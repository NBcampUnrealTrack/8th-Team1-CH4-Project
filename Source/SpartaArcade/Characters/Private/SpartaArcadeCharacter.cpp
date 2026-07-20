#include "SpartaArcadeCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Level/Public/SpartaArcadeMapBuilder.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "SpartaArcadeBomb.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "CombatComponent.h"
#include "DeathDropComponent.h"
#include "Engine/DataTable.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"
#include "SpartaArcadePlayerController.h"
#include "UI/Public/SpartaHUDWidget.h"
#include "Engine/DamageEvents.h"
#include "AbilitySystemComponent.h"
#include "BomberAttributeSet.h"
#include "BomberGameplayTags.h"
#include "InGame/SpartaGameMode.h"
#include "InGame/SpartaGameState.h"
#include "UI/Public/SpartaMenuFlowWidget.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/GameStateBase.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateColor.h"
#include "WorldToScreenWidget.h"

ASpartaArcadeCharacter::ASpartaArcadeCharacter()
{
	// 캡슐 콜리전 크기 초기화
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// 폭발 판정(SweepAndApplyDamage)이 ECC_Visibility 채널로 스윕하므로,
	// 프리셋 설정과 무관하게 캡슐이 이 채널을 확실히 Block 하도록 명시적으로 고정
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// 컨트롤러 회전값 사용 해제
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 무브먼트 컴포넌트 기본값 설정 (이동 방향으로 캐릭터 회전)
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = false;
	GetCharacterMovement()->bSnapToPlaneAtStart = false;
	GetCharacterMovement()->GravityScale = 1.0f;
	GetCharacterMovement()->MaxStepHeight = 45.0f;
	GetCharacterMovement()->bAlwaysCheckFloor = true;
	if (GetMesh())
	{
		GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -96.f));
		GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	}

	// 스프링암 생성 및 부착
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	// 카메라 생성 및 부착
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	// 타일 지형 효과 및 카메라 상태 제어를 위해 틱 활성화
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// 컴포넌트 기반 아키텍처 적용
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	// 사망 시 아이템 드롭 컴포넌트 생성
	DeathDropComponent = CreateDefaultSubobject<UDeathDropComponent>(TEXT("DeathDropComponent"));

	MaxInitializedComponentsCount = 100;
	InitializedComponentsCount = 0;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UBomberAttributeSet>(TEXT("AttributeSet"));

	// 데디케이티드 서버 네트워크 동기화용 캐릭터 복제 활성화
	bReplicates = true;
	SetNetUpdateFrequency(66.0f);
	SetMinNetUpdateFrequency(33.0f);

	static ConstructorHelpers::FObjectFinder<UDataTable> CharacterStatTableFinder(TEXT("/Game/DataFile/DT_CharacterStat.DT_CharacterStat"));
	if (CharacterStatTableFinder.Succeeded())
	{
		CharacterStatTable = CharacterStatTableFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UDataTable> CombatStatTableFinder(TEXT("/Game/DataFile/DT_CombatStat.DT_CombatStat"));
	if (CombatStatTableFinder.Succeeded())
	{
		CombatStatTable = CombatStatTableFinder.Object;
	}
}

void ASpartaArcadeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 블루프린트 덮어쓰기 설정을 방어하기 위해 런타임에 틱 강제 활성화
	SetActorTickEnabled(true);

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bConstrainToPlane = false;
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	// 맵 빌더 인스턴스 검색 및 캐싱
	CachedMapBuilder = Cast<ASpartaArcadeMapBuilder>(UGameplayStatics::GetActorOfClass(GetWorld(), ASpartaArcadeMapBuilder::StaticClass()));

	// [성능 최적화] SceneCaptureComponent2D 사전 캐싱으로 매 틱 FindComponentByClass 탐색 제거
	CachedSceneCaptureComponent = FindComponentByClass<USceneCaptureComponent2D>();

	// 기본 무브먼트 매개변수 백업
	if (GetCharacterMovement())
	{
		DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
		DefaultGroundFriction = GetCharacterMovement()->GroundFriction;
		DefaultBrakingDeceleration = GetCharacterMovement()->BrakingDecelerationWalking;
	}

	InitializeHUD();
}

void ASpartaArcadeCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		if (HasAuthority())
		{
			if (IsValid(PlaceBombAbilityClass))
			{
				AbilitySystemComponent->GiveAbility(
					FGameplayAbilitySpec(PlaceBombAbilityClass, 1, INDEX_NONE, this));
			}

			if (IsValid(UseFirstAidKitAbilityClass))
			{
				AbilitySystemComponent->GiveAbility(
					FGameplayAbilitySpec(UseFirstAidKitAbilityClass, 1, INDEX_NONE, this));
			}
			
			if (IsValid(UseShieldAbilityClass))
			{
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UseShieldAbilityClass, 1, INDEX_NONE, this));
			}
		}
	}
	// 서버 측에서 빙의 시 컴포넌트 및 HUD 초기화 진행
	InitializeCharacterComponents();
	InitializeHUD();
	UpdateNickname(); 
}

void ASpartaArcadeCharacter::OnRep_PlayerState()                                                                                                                                                                                  
{                                                                                                                                                                                                                                 
	Super::OnRep_PlayerState();                         
	SpartaPlayerState = GetPlayerState<ASpartaPlayerState>();

	if (IsValid(AbilitySystemComponent))                                                                                                                                                                                          
	{                                                                                                                                                                                                                             
		AbilitySystemComponent->InitAbilityActorInfo(this, this);                                                                                                                                                                 
	}                                                                                                                                                                                                                             
	// 클라이언트 측에서 PlayerState 수신 시 컴포넌트 및 HUD 연동 초기화 재수행
	InitializeCharacterComponents();
	InitializeHUD();
	UpdateNickname();
}     

	// 하트 체력 감소, 실드 차단 및 체력 0 도달 시 기절 상태 진입 로직 구현
float ASpartaArcadeCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	UE_LOG(LogTemp, Verbose, TEXT("ASpartaArcadeCharacter::TakeDamage 호출됨! 피해량: %f, 원인 제공자: %s"), DamageAmount, DamageCauser ? *DamageCauser->GetName() : TEXT("None"));
	// CombatComponent에 데미지 처리 위임
	if (CombatComponent)
	{
		if (CombatComponent->CanTakeDamage())
		{
			CombatComponent->ApplyDamage();

			// 블루프린트에 구현된 피격 시각 연출(깜빡임 등)을 실행합니다.
			// OnHitFlash는 폭탄/장애물 피격 모두에 적용됩니다.
			MulticastHitFlash(1.0f);
			//OnHitFlash(1.0f);

			// 무적 시간은 CombatComponent::ApplyDamage() 내부에서 1초(InvincibleDuration)로 관리됩니다.
			// 아래 ECC_Visibility 0.2초 차단은 폭발 스윕 중복 피격 방어용으로 병존시킵니다.
			if (GetCapsuleComponent())
			{
				// 피해를 입는 즉시 캡슐의 Visibility 채널을 Ignore(무시)로 전환
				GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

				// 0.2초 타이머 작동 후 RestoreCollisionResponse 실행하여 원복
				GetWorld()->GetTimerManager().SetTimer(
					CollisionRestoreTimerHandle,
					this,
					&ASpartaArcadeCharacter::RestoreCollisionResponse,
					0.2f,
					false
				);
			}

			return 1.f;
		}
	}
	return 0.f;
}

void ASpartaArcadeCharacter::InitializeCharacterComponents()
{
	// 이미 초기화가 완료된 경우 실행 차단
	if (bComponentsInitialized)
	{
		return;
	}

	SpartaPlayerState = GetPlayerState<ASpartaPlayerState>();
	if (!IsValid(SpartaPlayerState) || !IsValid(CombatComponent))
	{
		if(InitializedComponentsCount >= MaxInitializedComponentsCount)
		{
			return;
		}

		InitializedComponentsCount++;
		FTimerDelegate TimerDel;
		TimerDel.BindUObject(this, &ASpartaArcadeCharacter::InitializeCharacterComponents);
		GetWorldTimerManager().SetTimerForNextTick(TimerDel);
		return;
	}

	// 컴포넌트 기반 초기화 및 델리게이트 바인딩
	FName RowName = FName(TEXT("Default"));
	UMaterialInstance* TargetMaterial = nullptr;

	if (IsValid(SpartaPlayerState))
	{
		switch (SpartaPlayerState->GetCharacterType())
		{
		case ESpartaArcadeCharacterType::Explosive:
			RowName = FName(TEXT("Explosion"));
			TargetMaterial = ExplosiveMaterial;
			break;
		case ESpartaArcadeCharacterType::Speed:
			RowName = FName(TEXT("Speed"));
			TargetMaterial = SpeedMaterial;
			break;
		case ESpartaArcadeCharacterType::BombCount:
			RowName = FName(TEXT("BombCount"));
			TargetMaterial = BombCountMaterial;
			break;
		}
	}
	if (TargetMaterial && GetMesh())
	{
		GetMesh()->SetMaterial(0, TargetMaterial);
	}

	if (IsValid(AttributeSet) && IsValid(CharacterStatTable))
	{
		AttributeSet->InitializeFromDataTable(CharacterStatTable, RowName);
	}

	if (CombatComponent)
	{
		CombatComponent->InitializePlayerState(GetPlayerState());

		if (CombatStatTable)
		{
			CombatComponent->InitializeFromDataTable(CombatStatTable);
		}

		CombatComponent->OnStun.AddDynamic(this, &ASpartaArcadeCharacter::HandleOnStun);
		CombatComponent->OnRevived.AddDynamic(this, &ASpartaArcadeCharacter::HandleOnRevived);
		CombatComponent->OnSelfRevive.AddDynamic(this, &ASpartaArcadeCharacter::HandleOnSelfRevive);
		CombatComponent->OnEliminated.AddDynamic(this, &ASpartaArcadeCharacter::HandleOnEliminated);

		if(HasAuthority())
		{
			ASpartaGameMode* SpartaGameMode = GetWorld()->GetAuthGameMode<ASpartaGameMode>();
			if (IsValid(SpartaGameMode))
			{
				CombatComponent->OnEliminatedEvent.AddUObject(SpartaGameMode, &ASpartaGameMode::HandlePlayerEliminated);
			}
		}
	}

	UpdateNickname();

	// 성공적으로 모든 초기화 및 HUD 바인딩 완료 시 플래그 설정
	bComponentsInitialized = true;
}

void ASpartaArcadeCharacter::InitializeHUD()
{
	if (IsLocallyControlled())
	{
		ASpartaArcadePlayerController* PC = Cast<ASpartaArcadePlayerController>(GetController());
		if (IsValid(PC) && IsValid(PC->HUDUIWidgetInstance) && IsValid(SpartaPlayerState) && IsValid(AttributeSet) && IsValid(CombatComponent))
		{
			USpartaHUDWidget* HUDWidget = Cast<USpartaHUDWidget>(PC->HUDUIWidgetInstance);
			if (IsValid(HUDWidget))
			{
				HUDWidget->InitializeHUD(SpartaPlayerState, AttributeSet, CombatComponent);
				SpartaPlayerState->BroadcastCurrentState();
				CombatComponent->BroadcastCurrentState();
			}
		}
	}
}

// 클래식 봄버맨 타일 일치를 위해 캐릭터의 현재 발밑 좌표를 100단위 그리드로 보정하여 스폰
void ASpartaArcadeCharacter::PlaceBomb()
{
	// 기절(Stunned) 혹은 사망(Eliminated) 상태일 때는 폭탄 설치 불가
	if (CombatComponent && CombatComponent->GetPlayerState() != EBomberPlayerState::Alive)
	{
		return;
	}

	// GA_PlaceBomb 어빌리티에 폭탄 설치 위임
	if (IsValid(AbilitySystemComponent) && IsValid(PlaceBombAbilityClass))
	{
		AbilitySystemComponent->TryActivateAbilityByClass(PlaceBombAbilityClass);
	}
}

void ASpartaArcadeCharacter::PlayPlaceBombAnim()
{
	// 실제 스폰 성공 시 어빌리티 단에서 이 함수를 호출하여 몽타주 재생
	if (PlaceBombMontage)
	{
		MulticastPlayPlaceBombAnim();
		//PlayAnimMontage(PlaceBombMontage, 1.0f);
	}
}

void ASpartaArcadeCharacter::AddFirstAidKit()
{
	if (SpartaPlayerState && SpartaPlayerState->GetFirstAidKits() < 1)
	{
		SpartaPlayerState->SetFirstAidKits(SpartaPlayerState->GetFirstAidKits() + 1);
	}
}

void ASpartaArcadeCharacter::AddShield()
{
	if (SpartaPlayerState && SpartaPlayerState->GetShields() < 1)                                                                                                                                                               
	{                                                                                                                                                                                                                           
		SpartaPlayerState->SetShields(SpartaPlayerState->GetShields() + 1);                                                                                                                                                 
	}   
}

void ASpartaArcadeCharacter::OnBombExploded()
{
	// 폭탄 카운트 처리는 GA_PlaceBomb 내부에서 델리게이트로 수행됨
}

void ASpartaArcadeCharacter::PerformUseShield()
{
	if (!SpartaPlayerState || SpartaPlayerState->GetShields() < 1) return;
	if (!CombatComponent || CombatComponent->IsShielded()) return;
	
	SpartaPlayerState->SetShields(SpartaPlayerState->GetShields() - 1);
	CombatComponent->GrantShield();
	
	MulticastHitFlash(3.0f);
	//OnHitFlash(3.0f);
}

void ASpartaArcadeCharacter::UnlockKickBomb()
{
	if (!HasAuthority()) return;
	if (!IsValid(AbilitySystemComponent) || !IsValid(KickBombAbilityClass)) return;
	if (AbilitySystemComponent->FindAbilitySpecFromClass(KickBombAbilityClass) != nullptr) return;
	
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(KickBombAbilityClass,1, INDEX_NONE, this));
	
	// 발차기 활성화 시 PlayerState에도 해당 상태를 업데이트하여 UI와 동기화
	if (IsValid(SpartaPlayerState))
	{
		SpartaPlayerState->SetKickUnlocked(true);
	}
}

// 구급 상자 사용 어빌리티 트리거
void ASpartaArcadeCharacter::UseFirstAidKit()
{
	if (IsValid(AbilitySystemComponent) && IsValid(UseFirstAidKitAbilityClass))
	{
		AbilitySystemComponent->TryActivateAbilityByClass(UseFirstAidKitAbilityClass);
	}
}

void ASpartaArcadeCharacter::UseShield()
{
	bool bActivated = false;
	if (IsValid(AbilitySystemComponent) && IsValid(UseShieldAbilityClass))
	{
		bActivated = AbilitySystemComponent->TryActivateAbilityByClass(UseShieldAbilityClass);
		UE_LOG(LogTemp, Warning, TEXT("[Shield] TryActivateAbilityByClass 결과: %d"), bActivated);
	}

	// 어빌리티 클래스가 에디터에 미할당되었거나 활성화에 실패한 경우, 서버 권한으로 PerformUseShield()를 직접 실행하여 확실한 사용을 보장합니다.
	if (!bActivated && HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Shield] GAS 어빌리티 미작동 -> PerformUseShield() 직접 호출 집행"));
		PerformUseShield();
	}
}

// 구급 상자를 소모하여 일반 상태에선 자가 치료(하트 회복), 기절 상태에선 자력 부활 처리
void ASpartaArcadeCharacter::PerformUseFirstAidKit()
{
	// CombatComponent와 연동하여 구급상자 소모 로직 수행
	if (SpartaPlayerState->GetFirstAidKits() <= 0)
	{
		return;
	}

	if (CombatComponent && IsValid(SpartaPlayerState))
	{
		if (IsStunned())
		{
			SpartaPlayerState->SetFirstAidKits(SpartaPlayerState->GetFirstAidKits() - 1);
			CombatComponent->SelfRevive();
		}
		else if (SpartaPlayerState->GetHearts() < SpartaPlayerState->GetStartHearts())
		{
			SpartaPlayerState->SetFirstAidKits(SpartaPlayerState->GetFirstAidKits() - 1);
			CombatComponent->Heal(1);
		}
	}
}

// 캐릭터 오버랩 시 기절 상태인 다른 플레이어를 판별하여 구조(아군) 혹은 처치(적군) 처리 수행
void ASpartaArcadeCharacter::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	// CombatComponent 기반 구조로 중복 및 의존성 해결
	ASpartaArcadeCharacter* OtherChar = Cast<ASpartaArcadeCharacter>(OtherActor);
	if (OtherChar && OtherChar->CombatComponent)
	{
		UAbilitySystemComponent* OtherASC = OtherChar->GetAbilitySystemComponent();
		if (IsValid(OtherASC) && OtherASC->HasMatchingGameplayTag(BomberGameplayTags::State_Stunned))
		{
			// 아군 구조 판정
			if (OtherChar->SpartaPlayerState->GetTeamID() != -1 && SpartaPlayerState->GetTeamID() == OtherChar->SpartaPlayerState->GetTeamID())
			{
				OtherChar->CombatComponent->OnOverlapWithAlly(this);
				UE_LOG(LogTemp, Log, TEXT("%s 가 아군 %s 를 구출했습니다!"), *GetName(), *OtherChar->GetName());
			}
			else
			{
				// 적군 처치 판정
				OtherChar->CombatComponent->OnOverlapWithEnemy(this);
				UE_LOG(LogTemp, Log, TEXT("%s 가 기절한 적 %s 를 처치했습니다!"), *GetName(), *OtherChar->GetName());
			}
		}
	}
}

FVector ASpartaArcadeCharacter::GetSnappedKickDirection() const
{
	FVector KickDir = GetActorForwardVector();
	KickDir.Z = 0.f;
	KickDir.Normalize();
	
	if (FMath::Abs(KickDir.X) > FMath::Abs(KickDir.Y))
	{
		return FVector(FMath::Sign(KickDir.X), 0.f, 0.f);
	}
	
	return FVector(0.f, FMath::Sign(KickDir.Y), 0.f);
}

// 폭탄 차기 어빌리티 트리거
void ASpartaArcadeCharacter::KickBomb()
{
	// 기절(Stunned) 혹은 사망(Eliminated) 상태일 때는 폭탄 차기 불가
	if (CombatComponent && CombatComponent->GetPlayerState() != EBomberPlayerState::Alive)
	{
		return;
	}

	if (IsValid(AbilitySystemComponent) && IsValid(KickBombAbilityClass))
	{
		AbilitySystemComponent->TryActivateAbilityByClass(KickBombAbilityClass);
	}
}

// 캐릭터 정면에 인접한 폭탄이 있다면 격자 축 정렬 방향 보내는 기능
void ASpartaArcadeCharacter::PerformKickBomb()
{
	FVector StartLoc = GetActorLocation();
	TArray<FHitResult> OutHits;
	FCollisionShape DetectionSphere = FCollisionShape::MakeSphere(120.f); // 전방 120 유닛 범위 검사
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// 스윕 검사에 실패했으면 즉시 중단
	bool bHasHits = GetWorld()->SweepMultiByChannel(OutHits, StartLoc, StartLoc + FVector(0.f, 0.f, 1.f),
		FQuat::Identity, ECC_Visibility, DetectionSphere, Params);
	if (!bHasHits) return;

	for (const FHitResult& Hit : OutHits)
	{
		ASpartaArcadeBomb* Bomb = Cast<ASpartaArcadeBomb>(Hit.GetActor());
		if (!Bomb || Bomb->IsRolling()) continue;

		// 캐릭터에서 폭탄까지의 2D 방향 벡터 계산
		FVector CharacterLoc = GetActorLocation();
		FVector BombLoc = Bomb->GetActorLocation();
		FVector DirToBomb = BombLoc - CharacterLoc;
		DirToBomb.Z = 0.f;
		DirToBomb.Normalize();

		// 캐릭터의 전방 방향 벡터 획득
		FVector ForwardVec = GetActorForwardVector();
		ForwardVec.Z = 0.f;
		ForwardVec.Normalize();

		float DotResult = FVector::DotProduct(ForwardVec, DirToBomb);

		// 전방 45도 범위를 벗어나면 차기 무시 (Early Continue)
		if (DotResult < 0.7f)
		{
			continue;
		}
		
		// 헬퍼 함수 호출
		FVector KickDir = GetSnappedKickDirection();
		if (KickBombMontage)
		{
			MulticastPlayKickAnim();
			//PlayAnimMontage(KickBombMontage, 1.0f);
		}
		Bomb->Kick(KickDir);
		UE_LOG(LogTemp, Log, TEXT("%s 가 폭탄을 %s 방향으로 찼습니다!"), *GetName(), *KickDir.ToString());
		break;
	}
}

// 컴포넌트 기반 Getter 함수 구현 추가
float ASpartaArcadeCharacter::GetHP() const
{
	return SpartaPlayerState ? (float)SpartaPlayerState->GetHearts() : 0.f;
}

// 최대 하트 설정값 반환
float ASpartaArcadeCharacter::GetMaxHP() const
{
	return SpartaPlayerState ? (float)SpartaPlayerState->GetStartHearts() : 0.f;
}

bool ASpartaArcadeCharacter::IsShielded() const
{
	return CombatComponent ? CombatComponent->IsShielded() : false;
}

bool ASpartaArcadeCharacter::IsStunned() const
{
	return IsValid(AbilitySystemComponent) && AbilitySystemComponent->HasMatchingGameplayTag(BomberGameplayTags::State_Stunned);
}

// CombatComponent 이벤트에 대응하는 핸들러 함수 구현 추가
void ASpartaArcadeCharacter::HandleOnStun()
{
	GetCharacterMovement()->DisableMovement();

	// 기절 진입 시 현재 재생 중인 몽타주(폭탄 설치/차기 등)를 강제 정지하여 상태 기계 핑퐁 방지
	MulticastStopAnim(0.2f);

	UE_LOG(LogTemp, Warning, TEXT("%s 기절 상태 진입!"), *GetName());
}

void ASpartaArcadeCharacter::HandleOnRevived()
{
	// 이미 돌고 있던 사망 소멸 타이머를 확실하게 해제합니다.
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DestroyTimerHandle);
	}

	// 꺼져있던 캡슐 콜리전과 이동 상태를 복구합니다.
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	UE_LOG(LogTemp, Log, TEXT("%s 부활/구출 완료!"), *GetName());
}

void ASpartaArcadeCharacter::HandleOnSelfRevive()
{
	// 자력 부활 시에도 동일하게 사망 타이머 및 콜리전 복구를 수행합니다.
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DestroyTimerHandle);
	}

	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	UE_LOG(LogTemp, Log, TEXT("%s 자력 부활 완료!"), *GetName());
}

void ASpartaArcadeCharacter::HandleOnEliminated()
{
	UE_LOG(LogTemp, Log, TEXT("%s 게임에서 탈락(소멸)되었습니다. 사망 연출을 시작합니다."), *GetName());

	// 사망 몽타주가 지정된 경우 재생 (연출용 - Destroy 타이밍에는 영향을 주지 않음)
	if (DeathMontage)
	{
		MulticastPlayDeathAnim();
	}

	// 팀전은 Eliminate()가 이미 기절 시간(설정값)에 맞춰 호출되므로 추가 대기 없이 바로 Destroy.
	// 개인전은 Eliminate()가 즉시 호출되므로, 사망 모션을 잠깐 보여주기 위해 DestroyDelay만큼 대기.
	float WaitDuration = 0.f;
	if (ASpartaGameState* SpartaGS = GetWorld()->GetGameState<ASpartaGameState>())
	{
		if (SpartaGS->GetGameModeType() == EGameModeType::Solo)
		{
			WaitDuration = DestroyDelay;
		}
	}

	// 사망 위치에 아이템 드롭 (서버 전용 컴포넌트 내부에서 Authority 체크)
	if (IsValid(DeathDropComponent))
	{
		DeathDropComponent->DropDeathItems(GetActorLocation());
	}

	// 이동 및 충돌을 완전히 무력화하여 사망 중 조작/충돌 이상을 방지
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
	}

	// 사망 모션 대기 후 소멸 처리
	if (WaitDuration <= 0.f)
	{
		EliminateDestroy();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(
			DestroyTimerHandle,
			this,
			&ASpartaArcadeCharacter::EliminateDestroy,
			WaitDuration,
			false
		);
	}
}

void ASpartaArcadeCharacter::EliminateDestroy()
{
	// [버그 수정] Replicated Actor 소멸은 오직 서버(HasAuthority) 권한에서만 진행되어야 네트워크 상에서 정상 제거됨
	if (HasAuthority())
	{
		Destroy();
	}
}

void ASpartaArcadeCharacter::RestoreCollisionResponse()
{
	if (GetCapsuleComponent())
	{
		// Visibility 채널을 다시 Block(차단)으로 돌려놓아 피해를 입을 수 있게 만듭니다.
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		UE_LOG(LogTemp, Warning, TEXT("%s 의 폭풍 콜리전 채널이 Block(활성화) 상태로 복구되었습니다."), *GetName());
	}
}

// IDamageable 인터페이스 구현체 정의. 폭탄 폭발로 인한 피해를 캐릭터 데미지 시스템에 연동.
void ASpartaArcadeCharacter::TakeExplosionDamage_Implementation()
{
	FDamageEvent DamageEvent;
	// 폭풍 피해 1.f 적용 (TakeDamage가 내부적으로 생명 1개 차감, 실드 체크 등 복합 로직 수행)
	TakeDamage(1.f, DamageEvent, GetController(), this);
}

bool ASpartaArcadeCharacter::CanTakeDamage_Implementation() const
{
	return CombatComponent ? CombatComponent->CanTakeDamage() : true;
}

bool ASpartaArcadeCharacter::BlocksExplosion_Implementation() const
{
	return false;
}

UAbilitySystemComponent* ASpartaArcadeCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// 캐릭터 위에 닉네임을 상시 렌더링하고 동기화하는 함수
void ASpartaArcadeCharacter::UpdateNickname()
{
	if (GetNetMode() == NM_DedicatedServer || !NicknameWidgetClass || IsValid(NicknameWidget))
	{
		return;
	}

	APlayerController* LocalPlayerController = nullptr;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();

		if (PC && PC->IsLocalController() && PC->GetLocalPlayer())
		{
			LocalPlayerController = PC;
			break;
		}
	}

	ASpartaPlayerState* SPS = GetPlayerState<ASpartaPlayerState>();
	if (!IsValid(SPS) || !IsValid(LocalPlayerController))
	{
		FTimerDelegate TimerDel;
		TimerDel.BindUObject(this, &ASpartaArcadeCharacter::UpdateNickname);
		GetWorldTimerManager().SetTimerForNextTick(TimerDel);
		return;
	}

	if (LocalPlayerController)
	{
		UWorldToScreenWidget* NameWidget = CreateWidget<UWorldToScreenWidget>(LocalPlayerController, NicknameWidgetClass);
		if (NameWidget)
		{
			NameWidget->AttachedActor = this;
			NameWidget->HeightOffset = 130.f;
			NameWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			NameWidget->AddToViewport(-1);
			NicknameWidget = NameWidget;

			FString PlayerName = SPS->GetPlayerName();
			int32 TeamID = SPS->GetTeamID();
			NameWidget->SetNickname(PlayerName, TeamID, LocalPlayerController);
		}
	}
}

// 매 프레임 캐릭터 씬 캡쳐 위치 강제 보정 및 최적화를 위해 Tick 구현
void ASpartaArcadeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 지형 지물 효과를 이동 속도 및 상태에 반영
	ApplyTileEffectToMovement(DeltaSeconds);
	
	// [성능 최적화] 매 프레임 FindComponentByClass 호출 대신 사전 캐싱된 SceneCapture 사용 (유효성 검사 추가)
	if (!IsValid(CachedSceneCaptureComponent))
	{
		CachedSceneCaptureComponent = FindComponentByClass<USceneCaptureComponent2D>();
	}

	USceneCaptureComponent2D* SceneCapture = CachedSceneCaptureComponent;

	if (SceneCapture)
	{
		if (IsLocallyControlled())
		{
			// 1. 로컬 플레이어 캐릭터의 경우 씬 캡처 활성화 보장
			if (!SceneCapture->IsActive())
			{
				SceneCapture->SetActive(true);
			}

			FVector TargetRelativeLoc = FVector(0.f, 0.f, 1500.f);
			FRotator TargetRelativeRot = FRotator(-90.f, 0.f, 0.f);
			if (!SceneCapture->GetRelativeLocation().Equals(TargetRelativeLoc, 1.f) ||
				!SceneCapture->GetRelativeRotation().Equals(TargetRelativeRot, 1.f))
			{
				SceneCapture->SetRelativeLocationAndRotation(TargetRelativeLoc, TargetRelativeRot);
			}
		}
		else
		{
			// 3. 원격 클라이언트 복제본 캐릭터들의 씬 캡처는 렌더링 부하 절감 및 간섭 예방을 위해 비활성화
			if (SceneCapture->IsActive())
			{
				SceneCapture->SetActive(false);
			}
		}
	}
}

void ASpartaArcadeCharacter::ApplyTileEffectToMovement(float DeltaSeconds)
{
	// 맵 빌더를 아직 캐싱하지 못한 경우 런타임에 실시간 탐색 및 캐싱 시도
	if (!IsValid(CachedMapBuilder))
	{
		CachedMapBuilder = Cast<ASpartaArcadeMapBuilder>(UGameplayStatics::GetActorOfClass(GetWorld(), ASpartaArcadeMapBuilder::StaticClass()));
	}

	if (!IsValid(CachedMapBuilder) || !GetCharacterMovement())
	{
		return;
	}

	// 현재 캐릭터가 서 있는 위치의 지형 타일 타입 및 좌표 조회
	FVector CurrentLocation = GetActorLocation();
	int32 TileX = 0, TileY = 0;
	bool bValidTile = CachedMapBuilder->WorldToTile(CurrentLocation, TileX, TileY);
	FIntPoint CurrentTileCoords(TileX, TileY);
	ESpartaArcadeTileType TileType = CachedMapBuilder->GetTileTypeAtWorldPosition(CurrentLocation);
	uint8 CurrentTileTypeRaw = static_cast<uint8>(TileType);

	// [성능 최적화] 동일 타일에 머물러 있고 컨베이어 타일이 아닌 경우, 매 프레임 속성 재할당 부하 차단
	if (bValidTile && CurrentTileCoords == LastTileLocation && CurrentTileTypeRaw == LastTileTypeRaw && TileType != ESpartaArcadeTileType::Conveyor)
	{
		return;
	}

	LastTileLocation = CurrentTileCoords;
	LastTileTypeRaw = CurrentTileTypeRaw;

	// 드코딩된 Blueprint 기본값 대신 현재 AttributeSet의 MoveSpeed 속성에 비례하는 기준 속도를 동적 계산
	float BaseWalkSpeed = DefaultMaxWalkSpeed;
	if (IsValid(AttributeSet))
	{
		const float AttrBaseSpeed = 200.f;
		const float AttrSpeedPerLevel = 100.f;
		BaseWalkSpeed = AttrBaseSpeed + (AttributeSet->GetMoveSpeed() * AttrSpeedPerLevel);
	}

	switch (TileType)
	{
	case ESpartaArcadeTileType::Ice:
		// 얼음 타일: 마찰력을 대폭 줄이고 제동 감속도를 낮추어 미끄러지도록 설정
		GetCharacterMovement()->GroundFriction = IceFriction;
		GetCharacterMovement()->BrakingDecelerationWalking = IceBrakingDeceleration;
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		break;

	case ESpartaArcadeTileType::MudWater:
		// 진흙 타일: 기본 속도를 절반(MudSpeedMultiplier)으로 감속
		GetCharacterMovement()->GroundFriction = DefaultGroundFriction;
		GetCharacterMovement()->BrakingDecelerationWalking = DefaultBrakingDeceleration;
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * MudSpeedMultiplier;
		break;

	case ESpartaArcadeTileType::Conveyor:
		// 컨베이어 타일: 이동 속도와 마찰은 기본으로 유지하고, 컨베이어의 추진 방향으로 강제 입력 추가
		GetCharacterMovement()->GroundFriction = DefaultGroundFriction;
		GetCharacterMovement()->BrakingDecelerationWalking = DefaultBrakingDeceleration;
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		
		// [임시 방향 벡터]: 현재는 가로(Y축) 방향으로 캐릭터를 서서히 밀어내도록 임시 구현
		// 실제 기획 또는 맵의 컨베이어 방향 데이터에 대응하여 방향 벡터를 조절할 수 있습니다.
		{
			FVector PushDirection = FVector(0.f, 1.f, 0.f); 
			GetCharacterMovement()->AddInputVector(PushDirection * ConveyorPushSpeed * DeltaSeconds, true);
		}
		break;

	default:
		// 일반 바닥 타일: 원래 백업해 두었던 기본 마찰력, 감속도, 이동 속도로 안전하게 복원
		GetCharacterMovement()->GroundFriction = DefaultGroundFriction;
		GetCharacterMovement()->BrakingDecelerationWalking = DefaultBrakingDeceleration;
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		break;
	}
}

//------------------------------
// 애니메이션 재생 멀티캐스트 함수

void ASpartaArcadeCharacter::MulticastPlayPlaceBombAnim_Implementation()
{
	if(IsRunningDedicatedServer())
	{
		return;
	}

	if(IsValid(PlaceBombMontage))
	{
		PlayAnimMontage(PlaceBombMontage, 1.0f);
	}
}

void ASpartaArcadeCharacter::MulticastPlayKickAnim_Implementation()
{
	if (IsRunningDedicatedServer())
	{
		return;
	}

	if (IsValid(KickBombMontage))
	{
		PlayAnimMontage(KickBombMontage, 1.0f);
	}
}

void ASpartaArcadeCharacter::MulticastPlayDeathAnim_Implementation()
{
	if (IsRunningDedicatedServer())
	{
		return;
	}
	if (IsValid(DeathMontage))
	{
		PlayAnimMontage(DeathMontage, 1.0f);
	}
}

void ASpartaArcadeCharacter::MulticastHitFlash_Implementation(float FlashDuration)
{
	if (IsRunningDedicatedServer())
	{
		return;
	}
	
	OnHitFlash(FlashDuration);
}

void ASpartaArcadeCharacter::MulticastStopAnim_Implementation(float InBlendOutTime)
{
	if (IsRunningDedicatedServer())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(0.2f);
	}
}