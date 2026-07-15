#include "SpartaArcadeCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
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
#include "Engine/DataTable.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"
#include "SpartaArcadePlayerController.h"
#include "UI/Public/SpartaHUDWidget.h"
#include "Engine/DamageEvents.h"
#include "AbilitySystemComponent.h"
#include "BomberAttributeSet.h"
#include "BomberGameplayTags.h"
#include "UI/Public/SpartaMenuFlowWidget.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/GameStateBase.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateColor.h"


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
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

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

	// Tick 낭비 방지를 위해 bCanEverTick 비활성화
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 컴포넌트 기반 아키텍처 적용
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	MaxInitializedComponentsCount = 100;
	InitializedComponentsCount = 0;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UBomberAttributeSet>(TEXT("AttributeSet"));

	// 데디케이티드 서버 네트워크 동기화용 캐릭터 복제 활성화
	bReplicates = true;

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

	NicknameWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("NicknameWidgetComponent"));
	NicknameWidgetComponent->SetupAttachment(RootComponent);
	NicknameWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen); // 항상 화면을 향하도록 Screen 스페이스 설정
	NicknameWidgetComponent->SetDrawSize(FVector2D(200.f, 50.f));
	NicknameWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 130.f)); // 캐릭터 머리 위 높이로 적당히 배치
}

void ASpartaArcadeCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeCharacterComponents();
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
	UpdateNickname(); 
}

void ASpartaArcadeCharacter::OnRep_PlayerState()                                                                                                                                                                                  
{                                                                                                                                                                                                                                 
	Super::OnRep_PlayerState();                                                                                                                                                                                                   
	if (IsValid(AbilitySystemComponent))                                                                                                                                                                                          
	{                                                                                                                                                                                                                             
		AbilitySystemComponent->InitAbilityActorInfo(this, this);                                                                                                                                                                 
	}                                                                                                                                                                                                                             
	UpdateNickname();
}     

// 하트 체력 감소, 실드 차단 및 체력 0 도달 시 기절 상태 진입 로직 구현
float ASpartaArcadeCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	// [디버그 로그 추가] - 데미지 수신이 되는지 확인!
	// 과도한 Warning 로그 방지를 위해 Verbose 레벨로 변경
	UE_LOG(LogTemp, Verbose, TEXT("ASpartaArcadeCharacter::TakeDamage 호출됨! 피해량: %f, 원인 제공자: %s"), DamageAmount, DamageCauser ? *DamageCauser->GetName() : TEXT("None"));
	// CombatComponent에 데미지 처리 위임
	if (CombatComponent)
	{
		if (CombatComponent->CanTakeDamage())
		{
			CombatComponent->ApplyDamage();

			if (GetCapsuleComponent())
			{
				// 피해를 입는 즉시 캡슐의 Visibility 채널을 Ignore(무시)로 전환하여 0.2초간 무적 상태로 만듭니다.
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
	// 이미 초기화가 완료되어 SpartaPlayerState가 할당되어 있다면 중복 실행을 차단합니다.
	if (IsValid(SpartaPlayerState))
	{
		return;
	}

	if (!IsValid(GetPlayerState()) || !IsValid(CombatComponent))
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
	SpartaPlayerState = GetPlayerState<ASpartaPlayerState>();
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
	}

	if (IsLocallyControlled())
	{
		if (ASpartaArcadePlayerController* PC = Cast<ASpartaArcadePlayerController>(GetController()))
		{
			if (USpartaHUDWidget* HUDWidget = Cast<USpartaHUDWidget>(PC->HUDUIWidgetInstance))
			{
				HUDWidget->InitializeHUD(SpartaPlayerState, AttributeSet, CombatComponent, nullptr);
				SpartaPlayerState->BroadcastCurrentState();
				CombatComponent->BroadcastCurrentState();
			}
		}
	}
	UpdateNickname();
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
		PlayAnimMontage(PlaceBombMontage, 1.0f);
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
	// CombatComponent에 방어막 획득 위임
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
	if (IsValid(AbilitySystemComponent) && IsValid(UseShieldAbilityClass))                                                                                                                                                      
	{                                                                                                                                                                                                                           
		AbilitySystemComponent->TryActivateAbilityByClass(UseShieldAbilityClass);                                                                                                                                           
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
			PlayAnimMontage(KickBombMontage, 1.0f);
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
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(0.2f);
	}

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
	
	// 팀원의 패배 UI 호출 보존
	ShowMatchResultUI(EMatchResult::Defeat);

	// 이동 및 충돌을 완전히 무력화하여 사망 중 조작/충돌 이상을 방지
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
	}

	// 애니메이션이 출력될 동안 대기 후 소멸 타이머 등록
	GetWorld()->GetTimerManager().SetTimer(
		DestroyTimerHandle,
		this,
		&ASpartaArcadeCharacter::EliminateDestroy,
		DestroyDelay,
		false
	);
}

void ASpartaArcadeCharacter::EliminateDestroy()
{
	UE_LOG(LogTemp, Log, TEXT("%s 캐릭터 액터가 월드에서 완전히 제거(소멸)됩니다."), *GetName());
	Destroy();
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
	if (!IsValid(NicknameWidgetComponent))
	{
		return;
	}

	UUserWidget* UserWidget = NicknameWidgetComponent->GetUserWidgetObject();
	if (!UserWidget)
	{
		// 위젯 오브젝트가 아직 로드되지 않은 경우, 0.1초 뒤에 재시도
		FTimerHandle TempHandle;
		GetWorldTimerManager().SetTimer(TempHandle, this, &ASpartaArcadeCharacter::UpdateNickname, 0.1f, false);
		return;
	}

	APlayerState* PS = GetPlayerState();
	if (IsValid(PS))
	{
		FString PlayerName = PS->GetPlayerName();
		
		// 위젯 트리에서 첫 번째 TextBlock을 찾아 플레이어 닉네임 적용
		UTextBlock* TargetText = nullptr;
		UserWidget->WidgetTree->ForEachWidget([&TargetText](UWidget* Widget)
		{
			// 첫 번째 발견한 TextBlock만 선택되도록 수정
			if (!TargetText)
			{
				if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
				{
					TargetText = TextBlock;
				}
			}
		});

		if (TargetText)
		{
			TargetText->SetText(FText::FromString(PlayerName));

			// Team 1이면 빨간색, Team 2면 파란색으로 닉네임 색상 표시
			if (ASpartaPlayerState* SPS = Cast<ASpartaPlayerState>(PS))
			{
				int32 LocalTeamID = SPS->GetTeamID();
				if (LocalTeamID == 1)
				{
					TargetText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.25f, 0.25f)));
				}
				else if (LocalTeamID == 2)
				{
					TargetText->SetColorAndOpacity(FSlateColor(FLinearColor(0.25f, 0.5f, 1.0f)));
				}
				else
				{
					TargetText->SetColorAndOpacity(FSlateColor(FLinearColor::White)); // 기본 색상 (흰색)
				}
			}
		}
	}
	else
	{
		// PlayerState가 유효해질 때까지 재시도
		FTimerHandle TempHandle;
		GetWorldTimerManager().SetTimer(TempHandle, this, &ASpartaArcadeCharacter::UpdateNickname, 0.1f, false);
	}
}

void ASpartaArcadeCharacter::ShowMatchResultUI(EMatchResult Result)
{
	if (bMatchResultShown || !IsLocallyControlled())
	{
		return;
	}
	bMatchResultShown = true;

	for (TObjectIterator<USpartaMenuFlowWidget> It; It; ++It)
	{
		if (It->GetWorld() == GetWorld())
		{
			int32 AliveCount = 0;
			TArray<FMatchPlayerResult> MatchResults;

			if (AGameStateBase* GS = GetWorld()->GetGameState())
			{
				for (APlayerState* PS : GS->PlayerArray)
				{
					if (ASpartaPlayerState* SPS = Cast<ASpartaPlayerState>(PS))
					{
						if (SPS->GetCurrentState() != EBomberPlayerState::Eliminated)
						{
							AliveCount++;
						}

						FMatchPlayerResult Res;
						Res.PlayerName = SPS->GetPlayerName();
						Res.Rank = (SPS->GetCurrentState() == EBomberPlayerState::Eliminated) ? 4 : 1;
						MatchResults.Add(Res);
					}
				}
			}

			int32 FinalRank = (Result == EMatchResult::Defeat) ? FMath::Max(1, AliveCount) : 1;
			It->ShowMatchResult(Result, FinalRank, MatchResults);

			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				PC->SetShowMouseCursor(true);
				PC->SetInputMode(FInputModeUIOnly());
			}
			return;
		}
	}
}

// 매 프레임 캐릭터 씬 캡쳐 위치 강제 보정 및 최적화를 위해 Tick 구현
void ASpartaArcadeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 캐릭터에 부착된 SceneCaptureComponent2D 검색
	USceneCaptureComponent2D* SceneCapture = FindComponentByClass<USceneCaptureComponent2D>();

	if (SceneCapture)
	{
		if (IsLocallyControlled())
		{
			// 1. 로컬 플레이어 캐릭터의 경우 씬 캡처 활성화 보장
			if (!SceneCapture->IsActive())
			{
				SceneCapture->SetActive(true);
			}

			// 2. 상대 위치/회전을 로컬 캐릭터의 정수리 위 1500 uu 상공, 수직 하방으로 고정
			FVector TargetRelativeLoc = FVector(0.f, 0.f, 1500.f);
			FRotator TargetRelativeRot = FRotator(-90.f, 0.f, 0.f);
			SceneCapture->SetRelativeLocationAndRotation(TargetRelativeLoc, TargetRelativeRot);
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

