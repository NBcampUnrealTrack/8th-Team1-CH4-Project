#include "SpartaArcadeBomb.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "SpartaArcadeCharacter.h"
#include "SpartaArcadeBlock.h"
#include "BreakableBox.h"
#include "WorldPartition/HLOD/DestructibleHLODComponent.h"

ASpartaArcadeBomb::ASpartaArcadeBomb()
{
	// 폭탄 굴리기 시뮬레이션을 위해 틱 활성화
	PrimaryActorTick.bCanEverTick = true;

	// 데디케이티드 서버를 위한 복제 및 이동 동기화 설정
	bReplicates = true;
	SetReplicateMovement(true);

	//  루트 컴포넌트를 기존과 같이 MeshComponent로 원복하여 BlockAll을 기본으로 동작시킴
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
	MeshComponent->SetGenerateOverlapEvents(false);

	ExplosionDelay = 3.0f;
	FirePower = 1;
	GridSize = 100.0f; // 타일 한 칸의 100x100 유닛 규격
	ExplosionDamage = 50.0f;

	// 굴리기 초기 상태 변수들 설정
	bIsRolling = false;
	RollDirection = FVector::ZeroVector;
	RollSpeed = 800.f;
	bIsExploded = false;
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		MeshComponent->SetStaticMesh(SphereMeshAsset.Object);
	}
}

void ASpartaArcadeBomb::BeginPlay()
{
	Super::BeginPlay();
	
	// 폭발 시간 카운트다운 타이머 등록
	// 폭발은 서버에서만 동작
	if (HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(ExplosionTimerHandle, this, &ASpartaArcadeBomb::Explode, ExplosionDelay, false);
	}
	

	// 스폰 시점에 이 폭탄의 2D 평면 영역(반경 80유닛 이내)에 겹쳐 있는 캐릭터들을 완벽히 감지하여 충돌 무시 설정
	TArray<AActor*> FoundCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpartaArcadeCharacter::StaticClass(), FoundCharacters);
	
	FVector MyLoc = GetActorLocation();
	for (AActor* Actor : FoundCharacters)
	{
		ASpartaArcadeCharacter* Character = Cast<ASpartaArcadeCharacter>(Actor);
		if (Character)
		{
			// Z축 높이 값을 배제하고 오직 XY 평면 기준 2D 평면 거리만 판정하도록 Dist2D 사용
			float Dist2D = FVector::Dist2D(MyLoc, Character->GetActorLocation());
			// 캐릭터 캡슐 반경 42.f + 알파 마진을 고려해 스폰 시 80유닛 이내 겹친 자들을 감지
			if (Dist2D < 80.f)
			{
				IgnoredCharacters.Add(Character);
				// 물리 관통 솔버(Penetration Solver)의 튕겨내기 버그를 피하기 위해 캡슐 레벨에서 충돌을 완전히 무시
				Character->GetCapsuleComponent()->IgnoreActorWhenMoving(this, true);
			}
		}
	}
}

void ASpartaArcadeBomb::InitializeBomb(ASpartaArcadeCharacter* InInstigator, int32 InFirePower)
{
	InstigatorCharacter = InInstigator;
	FirePower = InFirePower;
}

void ASpartaArcadeBomb::ApplyExplosionDamage(AActor* Target)
{
	if (!Target || DamagedActors.Contains(Target))
	{
		return;
	}
	if (HasAuthority() == false)
	{
		return;
	}

	// 대미지 적용
	UGameplayStatics::ApplyDamage(
		Target,
		ExplosionDamage,
		InstigatorCharacter ? InstigatorCharacter->GetController() : nullptr,
		this,
		UDamageType::StaticClass()
	);

	// 맞은 목록에 등록
	DamagedActors.Add(Target);

	UE_LOG(LogTemp, Warning, TEXT("폭풍 대미지 적용 완료! 대상: %s, 남은 하트: %f"),
		*Target->GetName(),
		Cast<ASpartaArcadeCharacter>(Target) ? Cast<ASpartaArcadeCharacter>(Target)->GetHP() : 0.f);
}

// 단일 물리 스윕 트레이스 연산만 전담하여 수행하는 함수
bool ASpartaArcadeBomb::SweepAndApplyDamage(const FVector& Start, const FVector& End, float Radius)
{
	FHitResult HitResult;
	FCollisionShape ExplodeSphere = FCollisionShape::MakeSphere(Radius);
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);

	// 이번 폭발에서 이미 피해를 입은 대상을 물리 스윕 대상에서 제외
	// 캐릭터의 캡슐과 메쉬가 교차 스캔되며 다중 검출 및 다단 데미지가 들어가는 버그를 차단
	for (AActor* DamagedActor : DamagedActors)
	{
		if (IsValid(DamagedActor))
		{
			TraceParams.AddIgnoredActor(DamagedActor);
		}
	}

	// 단일 검출로 1개의 충돌체 접촉 시 즉시 연산 종료
	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		ExplodeSphere,
		TraceParams
	);

	if (!bHit || !HitResult.GetActor())
	{
		return false; // 부딪힌 지형이나 액터가 없으므로 불길 차단 없이 계속 진행
	}

	// 충돌 결과 분석 및 반응 처리는 HandleExplosionHit 함수로 분리 위임
	return HandleExplosionHit(HitResult.GetActor());
}

bool ASpartaArcadeBomb::HandleExplosionHit(AActor* HitActor)
{
	if (!HitActor) return false;

	// 다른 폭탄 발견 시 즉각 유폭(Chain Explosion) 유발 및 연속 관통 허용
	if (ASpartaArcadeBomb* OtherBomb = Cast<ASpartaArcadeBomb>(HitActor))
	{
		OtherBomb->Explode();
		return false;
	}

	// 파괴 가능 상자 블록 발견 시 부수고 불길 차단 (1칸만 파괴)
	if (ASpartaArcadeBlock* Block = Cast<ASpartaArcadeBlock>(HitActor))
	{
		Block->DestroyBlock();
		return true;
	}

	// 캐릭터 피격 처리 (캐릭터는 불길을 차단하지 않고 관통 통과)
	if (ASpartaArcadeCharacter* Character = Cast<ASpartaArcadeCharacter>(HitActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("폭풍이 캐릭터를 최초 감지했습니다! 대상: %s, 줄 데미지: %f"), *Character->GetName(), ExplosionDamage);
		ApplyExplosionDamage(Character);
		return false;
	}
	
	// 다른 폭탄이 아닌 고정 벽 지형이면 즉시 차단
	return true;
}


void ASpartaArcadeBomb::Explode()
{
	if(HasAuthority() == false)
	{
		return;
	}

	// 이미 폭발 중이면 즉시 리턴하여 무한 루프 방지
	if (bIsExploded) return;
	bIsExploded = true;
	// 유폭 연쇄 호출 시 타이머 중복 트리거 방지를 위해 선제 소멸 처리
	GetWorld()->GetTimerManager().ClearTimer(ExplosionTimerHandle);

	FVector StartLoc = GetActorLocation();
	ExplosionLocations.Add(StartLoc);

	// 중심부 대미지 판정을 SweepAndApplyDamage 헬퍼로 통합 수행
	SweepAndApplyDamage(StartLoc, StartLoc + FVector(0.f, 0.f, 1.f), GridSize * 0.4f);
	
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

	// 폭발 완료 델리게이트 호출 추가
	OnBombExploded.ExecuteIfBound();

	// 아직 해제되지 않은 충돌 무시 관계가 남았다면 액터 파괴 전에 깔끔하게 원상 복귀
	for (ASpartaArcadeCharacter* Character : IgnoredCharacters)
	{
		if (IsValid(Character) && Character->GetCapsuleComponent())
		{
			Character->GetCapsuleComponent()->IgnoreActorWhenMoving(this, false);
		}
	}

	// 폭발 판정 종료 후 폭발 이펙트 재생 및 소멸
	Multicast_PlayExplosionEffects(ExplosionLocations);
	Destroy();
}

void ASpartaArcadeBomb::PerformExplosionDirection(const FVector& Direction)
{
	FVector StartLoc = GetActorLocation();

	for (int32 i = 1; i <= FirePower; ++i)
	{
		FVector TargetLoc = StartLoc + Direction * (i * GridSize);

		ExplosionLocations.Add(TargetLoc);

		// 단일 스윕 헬퍼 호출을 통해 최초 1회 감지 보장 및 벽 차단 판정 처리
		if (SweepAndApplyDamage(StartLoc + Direction * ((i - 1) * GridSize), TargetLoc, GridSize * 0.35f))
		{
			break;
		}
	}
}

// 틱 함수에서 굴러가는 이동을 처리하고 장애물 충돌 시 격자 타일에 맞게 스냅 재정렬 수행
void ASpartaArcadeBomb::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 2D 거리를 수동 검사하여 캐릭터가 안전하게 영역을 벗어났을 때 충돌을 켬 (Z축 및 튕김 방지)
	for (int32 i = IgnoredCharacters.Num() - 1; i >= 0; --i)
	{
		ASpartaArcadeCharacter* Character = IgnoredCharacters[i];
		if (IsValid(Character))
		{
			float Dist2D = FVector::Dist2D(GetActorLocation(), Character->GetActorLocation());
			// 캐릭터 캡슐 반경(42.f)과 폭탄 물리 반경을 합해 안전한 이탈 거리인 90.f 유닛 이상 멀어지면 충돌 무시 제거
			if (Dist2D >= 90.f)
			{
				Character->GetCapsuleComponent()->IgnoreActorWhenMoving(this, false);
				IgnoredCharacters.RemoveAt(i);
			}
		}
		else
		{
			IgnoredCharacters.RemoveAt(i);
		}
	}

	if (!HasAuthority() || !bIsRolling)
	{
		return;
	}
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

void ASpartaArcadeBomb::Kick(const FVector& Direction)
{
	if(HasAuthority() == false)
	{
		return;
	}

	bIsRolling = true;
	RollDirection = Direction;
	RollDirection.Z = 0.f;
	RollDirection.Normalize();
}

void ASpartaArcadeBomb::Multicast_PlayExplosionEffects_Implementation(const TArray<FVector>& Locations)
{
	// 폭발 사운드 에셋이 지정되어 있다면 처음 폭발 위치에서 사운드 재생
	if (ExplosionSound && Locations.Num() > 0)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSound, Locations[0]);
	}

	for (const FVector& Location : Locations)
	{
		if (ExplosionVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionVFX, Location);
		}
		if (ExplosionCascadeVFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionCascadeVFX, Location);
		}
	}
}