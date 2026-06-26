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

	// 기본 속성
	CharacterType = ESpartaArcadeCharacterType::Speed;
	Hearts = 3;
	MaxHearts = 3;
	SpeedLevel = 3;
	BaseMovementSpeed = 300.f;
	MaxBombs = 3;
	CurrentActiveBombs = 0;
	BombRange = 2;
	bIsShielded = false;
	FirstAidKits = 0;
	bIsStunned = false;
	bIsInvulnerable = false;
	TeamID = -1; // 기본은 개인전

	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed + (SpeedLevel * 75.0f);

	// 데디케이티드 서버 네트워크 동기화용 캐릭터 복제 활성화
	bReplicates = true;
}

void ASpartaArcadeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 선택된 캐릭터 타입에 따른 시작 스탯 특화
	switch (CharacterType)
	{
	case ESpartaArcadeCharacterType::Explosive:
		BombRange = 5;
		MaxBombs = 3;
		SpeedLevel = 3;
		break;
	case ESpartaArcadeCharacterType::Speed:
		BombRange = 2;
		MaxBombs = 2;
		SpeedLevel = 4;
		break;
	case ESpartaArcadeCharacterType::BombCount:
		BombRange = 2;
		MaxBombs = 5;
		SpeedLevel = 2;
		break;
	}

	Hearts = MaxHearts;
	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed + (SpeedLevel * 75.0f);
}

void ASpartaArcadeCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

// 하트 체력 감소, 실드 차단 및 체력 0 도달 시 기절 상태 진입 로직 구현
float ASpartaArcadeCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsInvulnerable || bIsStunned)
	{
		return 0.f;
	}

	if (bIsShielded)
	{
		bIsShielded = false;
		return 0.f;
	}

	// 매 피격마다 정확히 1개의 하트만 감소
	Hearts = FMath::Clamp(Hearts - 1, 0, MaxHearts);

	if (Hearts <= 0)
	{
		bIsStunned = true;
		GetCharacterMovement()->DisableMovement();
		
		// 솔로(TeamID == -1)면 5초, 팀전(TeamID != -1)이면 10초 뒤 자동으로 부활(자가 치유)하는 타이머 세팅
		float StunDuration = (TeamID == -1) ? 5.0f : 10.0f;
		GetWorld()->GetTimerManager().SetTimer(StunTimerHandle, this, &ASpartaArcadeCharacter::HandleStunTimeout, StunDuration, false);
		
		UE_LOG(LogTemp, Warning, TEXT("%s 기절 상태 진입! (%f초 뒤 자동 회복 예정)"), *GetName(), StunDuration);
	}

	return 1.f;
}

// 클래식 봄버맨 타일 일치를 위해 캐릭터의 현재 발밑 좌표를 100단위 그리드로 보정하여 스폰
void ASpartaArcadeCharacter::PlaceBomb()
{
	if (bIsStunned || CurrentActiveBombs >= MaxBombs || !BombClass)
	{
		return;
	}

	FVector MyLocation = GetActorLocation();
	
	// X, Y 좌표를 가장 가까운 100단위 격자 중심으로 정렬
	float RoundedX = FMath::RoundToFloat(MyLocation.X / 100.f) * 100.f;
	float RoundedY = FMath::RoundToFloat(MyLocation.Y / 100.f) * 100.f;
	
	// 캐릭터의 발밑 높이로 설정
	float SpawnZ = MyLocation.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 50.f;
	FVector SpawnLocation = FVector(RoundedX, RoundedY, SpawnZ);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASpartaArcadeBomb* NewBomb = GetWorld()->SpawnActor<ASpartaArcadeBomb>(BombClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (NewBomb)
	{
		NewBomb->InitializeBomb(this, BombRange);
		CurrentActiveBombs++;
	}
}

// 최대 스탯 상한선(Cap)을 포함하는 아이템 효과 함수
void ASpartaArcadeCharacter::AddSpeedBoost()
{
	SpeedLevel = FMath::Clamp(SpeedLevel + 1, 0, 5); // 이속 최대 5단계
	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed + (SpeedLevel * 75.0f);
}

void ASpartaArcadeCharacter::AddExtraBomb()
{
	MaxBombs = FMath::Clamp(MaxBombs + 1, 0, 8); // 폭탄 최대 8개
}

void ASpartaArcadeCharacter::IncreaseExplosionRange()
{
	BombRange = FMath::Clamp(BombRange + 1, 0, 5); // 사거리 최대 5칸
}

void ASpartaArcadeCharacter::AddFirstAidKit()
{
	FirstAidKits++;
}

void ASpartaArcadeCharacter::AddShield()
{
	bIsShielded = true;
}

void ASpartaArcadeCharacter::OnBombExploded()
{
	CurrentActiveBombs = FMath::Max(0, CurrentActiveBombs - 1);
}

// 구급 상자를 소모하여 일반 상태에선 자가 치료(하트 회복), 기절 상태에선 자력 부활 처리
void ASpartaArcadeCharacter::UseFirstAidKit()
{
	if (FirstAidKits <= 0)
	{
		return;
	}

	if (bIsStunned)
	{
		FirstAidKits--;
		ReviveCharacter(1); // 하트 1개로 기절 자력 해제
	}
	else if (Hearts < MaxHearts)
	{
		FirstAidKits--;
		Hearts = FMath::Clamp(Hearts + 1, 0, MaxHearts);
	}
}

// 기절 상태 해제 및 무브먼트 복구, 1초 무적 판정 타이머 설정
void ASpartaArcadeCharacter::ReviveCharacter(int32 HealthToRestore)
{
	bIsStunned = false;
	Hearts = FMath::Clamp(HealthToRestore, 0, MaxHearts);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

	// 1초간 무적 상태
	bIsInvulnerable = true;
	GetWorld()->GetTimerManager().SetTimer(InvulnerableTimerHandle, this, &ASpartaArcadeCharacter::ResetInvulnerability, 1.0f, false);
}

void ASpartaArcadeCharacter::ResetInvulnerability()
{
	bIsInvulnerable = false;
}

// 기절 복귀 시간 초과 시 하트 1개로 자동 부활(자가 회복) 처리
void ASpartaArcadeCharacter::HandleStunTimeout()
{
	if (bIsStunned)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s 기절 복귀 시간 초과, 자동 부활"), *GetName());
		ReviveCharacter(1); // 하트 1개로 자동 회복 및 부활
	}
}

// 캐릭터 오버랩 시 기절 상태인 다른 플레이어를 판별하여 구조(아군) 혹은 처치(적군) 처리 수행
void ASpartaArcadeCharacter::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	ASpartaArcadeCharacter* OtherChar = Cast<ASpartaArcadeCharacter>(OtherActor);
	if (OtherChar && OtherChar->IsStunned())
	{
		// 아군 구조 판정
		if (TeamID != -1 && TeamID == OtherChar->TeamID)
		{
			OtherChar->ReviveCharacter(1);
			UE_LOG(LogTemp, Log, TEXT("%s 가 아군 %s 를 구출했습니다!"), *GetName(), *OtherChar->GetName());
		}
		else
		{
			// 적군 처치 판정
			OtherChar->Destroy();
			UE_LOG(LogTemp, Log, TEXT("%s 가 기절한 적 %s 를 처치했습니다!"), *GetName(), *OtherChar->GetName());
		}
	}
}

// 캐릭터 정면에 인접한 폭탄이 있다면 격자 축 정렬 방향 보내는 기능
void ASpartaArcadeCharacter::KickBomb()
{
	if (bIsStunned)
	{
		return;
	}

	FVector StartLoc = GetActorLocation();
	TArray<FHitResult> OutHits;
	FCollisionShape DetectionSphere = FCollisionShape::MakeSphere(120.f); // 전방 120 유닛 범위 검사
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->SweepMultiByChannel(OutHits, StartLoc, StartLoc + FVector(0.f, 0.f, 1.f), FQuat::Identity, ECC_Visibility, DetectionSphere, Params))
	{
		for (const FHitResult& Hit : OutHits)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && HitActor->IsA(ASpartaArcadeBomb::StaticClass()))
			{
				ASpartaArcadeBomb* Bomb = Cast<ASpartaArcadeBomb>(HitActor);
				if (Bomb && !Bomb->IsRolling())
				{
					FVector KickDir = GetActorForwardVector();
					KickDir.Z = 0.f;
					KickDir.Normalize();

					// 캐릭터 조준 방향에 맞춰 십자 축(상/하/좌/우) 직진 방향으로 보정 스냅
					if (FMath::Abs(KickDir.X) > FMath::Abs(KickDir.Y))
					{
						KickDir = FVector(FMath::Sign(KickDir.X), 0.f, 0.f);
					}
					else
					{
						KickDir = FVector(0.f, FMath::Sign(KickDir.Y), 0.f);
					}

					Bomb->Kick(KickDir);
					UE_LOG(LogTemp, Log, TEXT("%s 가 폭탄을 %s 방향으로 찼습니다!"), *GetName(), *KickDir.ToString());
					break;
				}
			}
		}
	}
}

// 네트워크 변수 동기화 규칙 정의 구현
void ASpartaArcadeCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpartaArcadeCharacter, Hearts);
	DOREPLIFETIME(ASpartaArcadeCharacter, MaxHearts);
	DOREPLIFETIME(ASpartaArcadeCharacter, SpeedLevel);
	DOREPLIFETIME(ASpartaArcadeCharacter, MaxBombs);
	DOREPLIFETIME(ASpartaArcadeCharacter, CurrentActiveBombs);
	DOREPLIFETIME(ASpartaArcadeCharacter, BombRange);
	DOREPLIFETIME(ASpartaArcadeCharacter, bIsShielded);
	DOREPLIFETIME(ASpartaArcadeCharacter, FirstAidKits);
	DOREPLIFETIME(ASpartaArcadeCharacter, bIsStunned);
	DOREPLIFETIME(ASpartaArcadeCharacter, bIsInvulnerable);
	DOREPLIFETIME(ASpartaArcadeCharacter, TeamID);
}
