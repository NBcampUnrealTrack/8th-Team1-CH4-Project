#include "SpartaArcadeCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "SpartaArcadeBomb.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "StatComponent.h"
#include "CombatComponent.h"
#include "BombPlacerComponent.h"
#include "Engine/DataTable.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"
#include "SpartaArcadePlayerController.h"
#include "UI/Public/SpartaHUDWidget.h"

ASpartaArcadeCharacter::ASpartaArcadeCharacter()
{
	// 캡슐 콜리전 크기 초기화
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

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

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// 컴포넌트 기반 아키텍처 적용
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	BombPlacerComponent = CreateDefaultSubobject<UBombPlacerComponent>(TEXT("BombPlacer"));

	BaseMovementSpeed = 300.f;

	MaxInitializedComponentsCount = 100;
	InitializedComponentsCount = 0;

	// 무브먼트 스피드 제어는 StatComponent 내 OnRep_MoveSpeed 에서 수행
	// GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed + (SpeedLevel * 75.0f);

	// 데디케이티드 서버 네트워크 동기화용 캐릭터 복제 활성화
	bReplicates = true;
}

void ASpartaArcadeCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeCharacterComponents();
	
}

void ASpartaArcadeCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

// 하트 체력 감소, 실드 차단 및 체력 0 도달 시 기절 상태 진입 로직 구현
float ASpartaArcadeCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	// CombatComponent에 데미지 처리 위임
	if (CombatComponent)
	{
		if (CombatComponent->CanTakeDamage())
		{
			CombatComponent->ApplyDamage();
			return 1.f;
		}
	}
	return 0.f;
}

void ASpartaArcadeCharacter::InitializeCharacterComponents()
{
	if (!IsValid(GetPlayerState()) || !IsValid(StatComponent) || !IsValid(CombatComponent) || !IsValid(BombPlacerComponent))
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
	SpartaPlayerState = GetPlayerState<ASpartaPlayerState>();
	if (IsValid(SpartaPlayerState))
	{
		switch (SpartaPlayerState->GetCharacterType())
		{
		case ESpartaArcadeCharacterType::Explosive:
			RowName = FName(TEXT("Explosive"));
			break;
		case ESpartaArcadeCharacterType::Speed:
			RowName = FName(TEXT("Speed"));
			break;
		case ESpartaArcadeCharacterType::BombCount:
			RowName = FName(TEXT("BombCount"));
			break;
		}
	}

	if (StatComponent)
	{
		if (CharacterStatTable)
		{
			StatComponent->SetCharacterStatTable(CharacterStatTable);
		}
		StatComponent->InitializeFromDataTable(RowName);
		
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
				HUDWidget->InitializeHUD(SpartaPlayerState, StatComponent, CombatComponent, BombPlacerComponent);
				SpartaPlayerState->BroadcastCurrentState();
				StatComponent->BroadcastCurrentState();
				CombatComponent->BroadcastCurrentState();
				BombPlacerComponent->BroadcastCurrentState();
			}
		}
	}
}
// 클래식 봄버맨 타일 일치를 위해 캐릭터의 현재 발밑 좌표를 100단위 그리드로 보정하여 스폰
void ASpartaArcadeCharacter::PlaceBomb()
{
	// BombPlacerComponent에 폭탄 설치 위임
	if (BombPlacerComponent)
	{
		BombPlacerComponent->ServerPlaceBomb();
	}
}

// 최대 스탯 상한선(Cap)을 포함하는 아이템 효과 함수
void ASpartaArcadeCharacter::AddSpeedBoost()
{
	// StatComponent에 스탯 성장 위임
	if (StatComponent)
	{
		StatComponent->GrowStat(EBomberStatType::MoveSpeed);
	}
}

void ASpartaArcadeCharacter::AddExtraBomb()
{
	// StatComponent에 스탯 성장 위임
	if (StatComponent)
	{
		StatComponent->GrowStat(EBomberStatType::BombCount);
	}
}

void ASpartaArcadeCharacter::IncreaseExplosionRange()
{
	// StatComponent에 스탯 성장 위임
	if (StatComponent)
	{
		StatComponent->GrowStat(EBomberStatType::BombRange);
	}
}

void ASpartaArcadeCharacter::AddFirstAidKit()
{
	SpartaPlayerState->SetFirstAidKits(SpartaPlayerState->GetFirstAidKits() + 1);
}

void ASpartaArcadeCharacter::AddShield()
{
	// CombatComponent에 방어막 획득 위임
	if (CombatComponent)
	{
		CombatComponent->GrantShield();
	}
}

void ASpartaArcadeCharacter::OnBombExploded()
{
	// 폭탄 카운트 처리는 BombPlacerComponent 내부에서 델리게이트로 수행됨
}

// 구급 상자를 소모하여 일반 상태에선 자가 치료(하트 회복), 기절 상태에선 자력 부활 처리
void ASpartaArcadeCharacter::UseFirstAidKit()
{
	// CombatComponent와 연동하여 구급상자 소모 로직 수행
	if (SpartaPlayerState->GetFirstAidKits() <= 0)
	{
		return;
	}

	if (CombatComponent && IsValid(SpartaPlayerState))
	{
		if (SpartaPlayerState->GetCurrentState() == EBomberPlayerState::Stunned)
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
		if (OtherChar->SpartaPlayerState->GetCurrentState() == EBomberPlayerState::Stunned)
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

// 캐릭터 정면에 인접한 폭탄이 있다면 격자 축 정렬 방향 보내는 기능
void ASpartaArcadeCharacter::KickBomb()
{
	// CombatComponent 상태 기반 기절 판단 적용
	bool bIsStunnedLocal = CombatComponent ? (SpartaPlayerState->GetCurrentState() == EBomberPlayerState::Stunned) : false;
	if (bIsStunnedLocal) return;

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

		// 헬퍼 함수 호출
		FVector KickDir = GetSnappedKickDirection();
		Bomb->Kick(KickDir);
		UE_LOG(LogTemp, Log, TEXT("%s 가 폭탄을 %s 방향으로 찼습니다!"), *GetName(), *KickDir.ToString());break;
	}
}

// 컴포넌트 기반 Getter 함수 구현 추가
float ASpartaArcadeCharacter::GetHP() const
{
	return SpartaPlayerState ? (float)SpartaPlayerState->GetHearts() : 0.f;
}

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
	return SpartaPlayerState ? (SpartaPlayerState->GetCurrentState() == EBomberPlayerState::Stunned) : false;
}

// CombatComponent 이벤트에 대응하는 핸들러 함수 구현 추가
void ASpartaArcadeCharacter::HandleOnStun()
{
	GetCharacterMovement()->DisableMovement();
	UE_LOG(LogTemp, Warning, TEXT("%s 기절 상태 진입!"), *GetName());
}

void ASpartaArcadeCharacter::HandleOnRevived()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	UE_LOG(LogTemp, Log, TEXT("%s 부활/구출 완료!"), *GetName());
}

void ASpartaArcadeCharacter::HandleOnSelfRevive()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	UE_LOG(LogTemp, Log, TEXT("%s 자력 부활 완료!"), *GetName());
}

void ASpartaArcadeCharacter::HandleOnEliminated()
{
	UE_LOG(LogTemp, Log, TEXT("%s 게임에서 탈락(소멸)되었습니다."), *GetName());
	Destroy();
}
