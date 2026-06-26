#include "SpartaArcadeBomb.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "SpartaArcadeCharacter.h"
#include "SpartaArcadeBlock.h"

ASpartaArcadeBomb::ASpartaArcadeBomb()
{
	// 폭탄 굴리기 시뮬레이션을 위해 틱 활성화
	PrimaryActorTick.bCanEverTick = true;

	// 데디케이티드 서버를 위한 복제 및 이동 동기화 설정
	bReplicates = true;
	SetReplicateMovement(true);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));

	ExplosionDelay = 3.0f;
	FirePower = 1;
	GridSize = 100.0f; // 타일 한 칸의 100x100 유닛 규격
	ExplosionDamage = 50.0f;

	// 굴리기 초기 상태 변수들 설정
	bIsRolling = false;
	RollDirection = FVector::ZeroVector;
	RollSpeed = 800.f;
}

void ASpartaArcadeBomb::BeginPlay()
{
	Super::BeginPlay();
	
	// 폭발 시간 카운트다운 타이머 등록
	GetWorld()->GetTimerManager().SetTimer(ExplosionTimerHandle, this, &ASpartaArcadeBomb::Explode, ExplosionDelay, false);
}

void ASpartaArcadeBomb::InitializeBomb(ASpartaArcadeCharacter* InInstigator, int32 InFirePower)
{
	InstigatorCharacter = InInstigator;
	FirePower = InFirePower;
}

void ASpartaArcadeBomb::Explode()
{
	// 유폭 연쇄 호출 시 타이머 중복 트리거 방지를 위해 선제 소멸 처리
	GetWorld()->GetTimerManager().ClearTimer(ExplosionTimerHandle);

	FVector StartLoc = GetActorLocation();

	// 폭발 중심부 비주얼 파티클 재생
	if (ExplosionVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionVFX, StartLoc);
	}

	// 폭탄 자체의 반경 내에 있는 중심점 데미지 스윕 판정
	TArray<FHitResult> OutHits;
	FCollisionShape CenterSphere = FCollisionShape::MakeSphere(GridSize * 0.4f);
	FCollisionQueryParams CenterParams;
	CenterParams.AddIgnoredActor(this);
	
	if (GetWorld()->SweepMultiByChannel(OutHits, StartLoc, StartLoc + FVector(0.f,0.f,1.f), FQuat::Identity, ECC_Visibility, CenterSphere, CenterParams))
	{
		for (const FHitResult& Hit : OutHits)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && HitActor->IsA(ASpartaArcadeCharacter::StaticClass()))
			{
				UGameplayStatics::ApplyDamage(HitActor, ExplosionDamage, InstigatorCharacter ? InstigatorCharacter->GetController() : nullptr, this, UDamageType::StaticClass());
			}
		}
	}

	// 십자 4방향으로 폭폭 화염 투사
	PerformExplosionDirection(FVector(1.f, 0.f, 0.f));  // 북
	PerformExplosionDirection(FVector(-1.f, 0.f, 0.f)); // 남
	PerformExplosionDirection(FVector(0.f, 1.f, 0.f));  // 동
	PerformExplosionDirection(FVector(0.f, -1.f, 0.f)); // 서

	// 소유자 캐릭터의 액티브 폭탄 슬롯 반환
	if (InstigatorCharacter)
	{
		InstigatorCharacter->OnBombExploded();
	}

	Destroy();
}

void ASpartaArcadeBomb::PerformExplosionDirection(const FVector& Direction)
{
	FVector StartLoc = GetActorLocation();
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);
	if (InstigatorCharacter)
	{
		TraceParams.AddIgnoredActor(InstigatorCharacter);
	}

	FCollisionShape ExplodeSphere = FCollisionShape::MakeSphere(GridSize * 0.35f);

	for (int32 i = 1; i <= FirePower; ++i)
	{
		FVector TargetLoc = StartLoc + Direction * (i * GridSize);
		FHitResult HitResult;

		// 1칸 앞 세그먼트 스윕 검출 수행
		bool bHit = GetWorld()->SweepSingleByChannel(
			HitResult,
			StartLoc + Direction * ((i - 1) * GridSize),
			TargetLoc,
			FQuat::Identity,
			ECC_Visibility,
			ExplodeSphere,
			TraceParams
		);

		// 각 분기마다 폭풍 불길 비주얼 파티클 생성
		if (ExplosionVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionVFX, TargetLoc);
		}

		if (bHit)
		{
			AActor* HitActor = HitResult.GetActor();
			if (HitActor)
			{
				// 다른 폭탄 발견 시 즉각 유폭(Chain Explosion) 유발 및 연속 관통 허용
				ASpartaArcadeBomb* OtherBomb = Cast<ASpartaArcadeBomb>(HitActor);
				if (OtherBomb)
				{
					OtherBomb->Explode();
				}

				// 파괴 가능 상자 블록 발견 시 부수고 불길 차단 (1칸만 파괴)
				ASpartaArcadeBlock* Block = Cast<ASpartaArcadeBlock>(HitActor);
				if (Block)
				{
					Block->DestroyBlock();
					break;
				}

				// 캐릭터 피격 처리 (캐릭터는 불길을 차단하지 않고 관통 통과)
				ASpartaArcadeCharacter* Character = Cast<ASpartaArcadeCharacter>(HitActor);
				if (Character)
				{
					UGameplayStatics::ApplyDamage(Character, ExplosionDamage, InstigatorCharacter ? InstigatorCharacter->GetController() : nullptr, this, UDamageType::StaticClass());
				}
				else if (!OtherBomb) //  다른 폭탄이 아닌 고정 벽 지형이면 즉시 차단
				{
					break;
				}
			}
		}
	}
}

// 틱 함수에서 굴러가는 이동을 처리하고 장애물 충돌 시 격자 타일에 맞게 스냅 재정렬 수행
void ASpartaArcadeBomb::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsRolling)
	{
		FHitResult SweepHit;
		FVector NextLoc = GetActorLocation() + RollDirection * RollSpeed * DeltaTime;

		// 스윕 콜리전을 켠 상태로 위치 갱신
		bool bObstacleHit = SetActorLocation(NextLoc, true, &SweepHit);

		// 다른 플레이어나 상자, 벽을 마주해 멈출 경우
		if (!bObstacleHit || SweepHit.bBlockingHit)
		{
			bIsRolling = false;

			// 폭발이 타일 격자 축을 빗나가지 않도록 즉시 100단위 그리드로 보정 스냅
			float RoundedX = FMath::RoundToFloat(GetActorLocation().X / 100.f) * 100.f;
			float RoundedY = FMath::RoundToFloat(GetActorLocation().Y / 100.f) * 100.f;
			SetActorLocation(FVector(RoundedX, RoundedY, GetActorLocation().Z));
		}
	}
}

void ASpartaArcadeBomb::Kick(const FVector& Direction)
{
	bIsRolling = true;
	RollDirection = Direction;
	RollDirection.Z = 0.f;
	RollDirection.Normalize();
}
