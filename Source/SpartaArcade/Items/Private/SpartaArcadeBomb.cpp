#include "SpartaArcadeBomb.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h" 
#include "Particles/ParticleSystemComponent.h"
#include "SpartaArcadeCharacter.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"
#include "Systems/Public/CombatComponent.h"
#include "BreakableBox.h"
#include "Damageable.h"
#include "WorldPartition/HLOD/DestructibleHLODComponent.h"

ASpartaArcadeBomb::ASpartaArcadeBomb()
{
	// 폭탄 굴리기 시뮬레이션을 위해 틱 활성화
	PrimaryActorTick.bCanEverTick = true;

	// 데디케이티드 서버를 위한 복제 및 이동 동기화 설정
	bReplicates = true;
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
	SetReplicateMovement(true);

	// 구체 콜리전을 생성하여 루트 컴포넌트로 지정 (물리 스윕 감지용)
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;
	CollisionComponent->SetSphereRadius(50.f);
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAll"));
	CollisionComponent->SetGenerateOverlapEvents(true);

	// MeshComponent를 자식 컴포넌트로 생성 및 첨부 (렌더링 및 자전 비주얼 전담)
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionProfileName(TEXT("NoCollision"));
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
	
	MeshComponent->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
}

void ASpartaArcadeBomb::BeginPlay()
{
	Super::BeginPlay();
	
	// 폭발 시간 카운트다운 타이머 등록
	// 폭발은 서버에서만 동작
	if (HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(ExplosionTimerHandle, this, &ASpartaArcadeBomb::Explode, ExplosionDelay, false);

		// 폭발 몇 초 전, 모든 클라이언트에서 도화선 이펙트가 재생되도록 예약
		const float FuseDelay = FMath::Max(0.f, ExplosionDelay - FuseLeadTime);
		GetWorld()->GetTimerManager().SetTimer(FuseTimerHandle, this, &ASpartaArcadeBomb::Multicast_PlayFuseEffect, FuseDelay, false);
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

	// 과도한 Warning 로그 방지를 위해 Verbose 레벨로 변경
	UE_LOG(LogTemp, Verbose, TEXT("대미지 적용 완료! 대상: %s, 남은 하트: %f"),
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
			// 폭발을 가로막는 장애물(BlocksExplosion이 true이거나 ABreakableBox인 대상)은 무시 목록에 넣지 않음으로써 다음 루프 스윕 시 가로막혀 불길이 관통하지 못하도록 처리
			if (DamagedActor->Implements<UDamageable>())
			{
				if (DamagedActor->IsA(ABreakableBox::StaticClass()) || IDamageable::Execute_BlocksExplosion(DamagedActor))
				{
					continue;
				}
			}
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
		// 과도한 Warning 로그 방지를 위해 Verbose 레벨로 변경
		UE_LOG(LogTemp, Verbose, TEXT("[Bomb] SweepAndApplyDamage: 스윕에 아무것도 안 걸림 (Start=%s, End=%s, Radius=%.1f)"),
			*Start.ToString(), *End.ToString(), Radius);
		return false; // 부딪힌 지형이나 액터가 없으므로 불길 차단 없이 계속 진행
	}

	// 과도한 Warning 로그 방지를 위해 Verbose 레벨로 변경
	UE_LOG(LogTemp, Verbose, TEXT("[Bomb] SweepAndApplyDamage: %s 에 충돌 감지됨"), *HitResult.GetActor()->GetName());

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

	// IDamageable 인터페이스를 사용하는 대상(상자, 캐릭터 등) 일괄 피격 처리
	if (HitActor->GetClass()->ImplementsInterface(UDamageable::StaticClass()))
	{
		// 과도한 Warning 로그 방지를 위해 Verbose 레벨로 변경
		UE_LOG(LogTemp, Verbose, TEXT("[Bomb] HandleExplosionHit: %s 는 IDamageable 구현함, CanTakeDamage=%s"),
			*HitActor->GetName(), IDamageable::Execute_CanTakeDamage(HitActor) ? TEXT("true") : TEXT("false"));

		if (IDamageable::Execute_CanTakeDamage(HitActor))
		{
			if (!DamagedActors.Contains(HitActor))
			{
				if (ASpartaArcadeCharacter* TargetChar = Cast<ASpartaArcadeCharacter>(HitActor))
				{
					ASpartaPlayerState* KillerPS = IsValid(InstigatorCharacter) ? InstigatorCharacter->GetPlayerState<ASpartaPlayerState>() : nullptr;
					if (UCombatComponent* CombatComp = TargetChar->GetCombatComponent())
					{
						CombatComp->SetLastAttacker(KillerPS, EDeathReason::Explosion);
					}
				}

				// 과도한 Warning 로그 방지를 위해 Verbose 레벨로 변경
				UE_LOG(LogTemp, Verbose, TEXT("[Bomb] HandleExplosionHit: %s 에 TakeExplosionDamage 호출"), *HitActor->GetName());
				IDamageable::Execute_TakeExplosionDamage(HitActor);
				DamagedActors.Add(HitActor);
			}
		}
		// 클래스가 ABreakableBox를 상속받은 대상이면 블루프린트 오버라이드 여부와 상관없이 확실하게 폭발을 차단(true)하도록 보완
		if (HitActor->IsA(ABreakableBox::StaticClass()))
		{
			return true;
		}
		// 인터페이스에 정의된 폭발 차단 여부에 따라 불길 관통/차단 처리
		return IDamageable::Execute_BlocksExplosion(HitActor);
	}

	// 과도한 Warning 로그 방지를 위해 Verbose 레벨로 변경
	UE_LOG(LogTemp, Verbose, TEXT("[Bomb] HandleExplosionHit: %s 는 IDamageable을 구현하지 않음 (벽/지형으로 간주하고 차단)"), *HitActor->GetName());

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
	GetWorld()->GetTimerManager().ClearTimer(FuseTimerHandle);

	FVector StartLoc = GetActorLocation();
	ExplosionLocations.Add(StartLoc);

	// 중심부 대미지 판정 범위를 좁혀 옆 칸의 상자가 미리 파괴되지 않도록 방지
	SweepAndApplyDamage(StartLoc, StartLoc + FVector(0.f, 0.f, 1.f), GridSize * 0.2f);
	
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

		// 단일 스윕 반경을 GridSize * 0.3f로 다듬어 오차로 인한 인접 물체 중복 피격을 방지
		if (SweepAndApplyDamage(StartLoc + Direction * ((i - 1) * GridSize), TargetLoc, GridSize * 0.3f))
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

	// 서버와 클라이언트 모두가 렌더링을 위해 실행
	if (bIsRolling && MeshComponent)
	{
		// 구르는 속도(RollSpeed)에 비례하여 자전 속도(초당 회전 각도) 결정
		// RollSpeed가 800일 때 초당 약 560도 자전하도록 설정
		float RotationSpeed = RollSpeed * 0.7f; 
		float YawDelta = RotationSpeed * DeltaTime;

		// 세워진 상태 그대로 좌측(반시계 방향)으로 자전하도록 Yaw 값만 추가 적용
		MeshComponent->AddRelativeRotation(FRotator(0.f, YawDelta, 0.f));
	}

	// 오직 서버 권한을 가진 경우에만 수행
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

void ASpartaArcadeBomb::Multicast_PlayFuseEffect_Implementation()
{
	if (FuseVFX && MeshComponent)
	{
		// 폭탄이 발에 차여 굴러가는 중이어도 같이 움직이도록 메시에 붙여서 재생
		UNiagaraFunctionLibrary::SpawnSystemAttached(FuseVFX, MeshComponent, NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
	}
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
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionVFX, Location, FRotator::ZeroRotator, FVector(1.f), true, true);
		}
		if (ExplosionCascadeVFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionCascadeVFX, Location, FRotator::ZeroRotator, FVector(1.f), true);
		}
	}
}

void ASpartaArcadeBomb::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpartaArcadeBomb, bIsRolling);
	DOREPLIFETIME(ASpartaArcadeBomb, RollDirection);
}