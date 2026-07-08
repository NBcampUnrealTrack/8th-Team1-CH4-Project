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

	// 컴포넌트 기반 아키텍처 적용
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	BombPlacerComponent = CreateDefaultSubobject<UBombPlacerComponent>(TEXT("BombPlacer"));

	BaseMovementSpeed = 300.f;
	FirstAidKits = 0;
	TeamID = -1; // 기본은 개인전

	// 무브먼트 스피드 제어는 StatComponent 내 OnRep_MoveSpeed 에서 수행
	// GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed + (SpeedLevel * 75.0f);

	// 데디케이티드 서버 네트워크 동기화용 캐릭터 복제 활성화
	bReplicates = true;
}

void ASpartaArcadeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 컴포넌트 기반 초기화 및 델리게이트 바인딩
	FName RowName = FName(TEXT("Default"));
	switch (CharacterType)
	{
	case ESpartaArcadeCharacterType::Explosive:
		RowName = FName(TEXT("Row_Explosion"));
		break;
	case ESpartaArcadeCharacterType::Speed:
		RowName = FName(TEXT("Row_Speed"));
		break;
	case ESpartaArcadeCharacterType::BombCount:
		RowName = FName(TEXT("Row_BombCount"));
		break;
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
		if (CombatStatTable)
		{
			CombatComponent->InitializeFromDataTable(CombatStatTable);
		}
		
		CombatComponent->OnStun.AddDynamic(this, &ASpartaArcadeCharacter::HandleOnStun);
		CombatComponent->OnRevived.AddDynamic(this, &ASpartaArcadeCharacter::HandleOnRevived);
		CombatComponent->OnSelfRevive.AddDynamic(this, &ASpartaArcadeCharacter::HandleOnSelfRevive);
		CombatComponent->OnEliminated.AddDynamic(this, &ASpartaArcadeCharacter::HandleOnEliminated);
	}
}

void ASpartaArcadeCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

// 하트 체력 감소, 실드 차단 및 체력 0 도달 시 기절 상태 진입 로직 구현
float ASpartaArcadeCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	// [디버그 로그 추가] - 데미지 수신이 되는지 확인!
	UE_LOG(LogTemp, Warning, TEXT("ASpartaArcadeCharacter::TakeDamage 호출됨! 피해량: %f, 원인 제공자: %s"), DamageAmount, DamageCauser ? *DamageCauser->GetName() : TEXT("None"));
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
	FirstAidKits++;
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
	if (FirstAidKits <= 0)
	{
		return;
	}

	if (CombatComponent)
	{
		if (CombatComponent->GetPlayerState() == EBomberPlayerState::Stunned)
		{
			FirstAidKits--;
			CombatComponent->SelfRevive();
		}
		else if (CombatComponent->GetHearts() < CombatComponent->GetMaxHearts())
		{
			FirstAidKits--;
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
		if (OtherChar->CombatComponent->GetPlayerState() == EBomberPlayerState::Stunned)
		{
			// 아군 구조 판정
			if (TeamID != -1 && TeamID == OtherChar->TeamID)
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
	bool bIsStunnedLocal = CombatComponent ? (CombatComponent->GetPlayerState() == EBomberPlayerState::Stunned) : false;
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
		Bomb->Kick(KickDir);
		UE_LOG(LogTemp, Log, TEXT("%s 가 폭탄을 %s 방향으로 찼습니다!"), *GetName(), *KickDir.ToString());break;
	}
}

// 네트워크 변수 동기화 규칙 정의 구현
void ASpartaArcadeCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	

	DOREPLIFETIME(ASpartaArcadeCharacter, FirstAidKits);
	DOREPLIFETIME(ASpartaArcadeCharacter, TeamID);
}

// 컴포넌트 기반 Getter 함수 구현 추가
float ASpartaArcadeCharacter::GetHP() const
{
	return CombatComponent ? (float)CombatComponent->GetHearts() : 0.f;
}

float ASpartaArcadeCharacter::GetMaxHP() const
{
	return CombatComponent ? (float)CombatComponent->GetMaxHearts() : 0.f;
}

bool ASpartaArcadeCharacter::IsShielded() const
{
	return CombatComponent ? CombatComponent->IsShielded() : false;
}

bool ASpartaArcadeCharacter::IsStunned() const
{
	return CombatComponent ? (CombatComponent->GetPlayerState() == EBomberPlayerState::Stunned) : false;
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

void ASpartaArcadeCharacter::RestoreCollisionResponse()
{
	if (GetCapsuleComponent())
	{
		// Visibility 채널을 다시 Block(차단)으로 돌려놓아 피해를 입을 수 있게 만듭니다.

		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

		UE_LOG(LogTemp, Warning, TEXT("%s 의 폭풍 콜리전 채널이 Block(활성화) 상태로 복구되었습니다."), *GetName());

	}
}