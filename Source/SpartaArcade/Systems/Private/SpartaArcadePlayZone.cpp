// Modified: 불필요한 영문 주석 제거 및 나선형 즉사 자기장 로직으로 전면 교체

#include "SpartaArcadePlayZone.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "SpartaArcadeCharacter.h"
#include "SpartaArcadeMapGenerator.h"
#include "SpartaArcadeBomb.h"
#include "SpartaArcadeBlock.h"
#include "SpartaArcadeItem.h"

ASpartaArcadePlayZone::ASpartaArcadePlayZone()
{
	PrimaryActorTick.bCanEverTick = false; // 프레임 단위 보간을 안 하므로 틱 비활성화

	// 데디케이티드 서버를 위한 자기장 동기화 활성화
	bReplicates = true;

	// 나선형 수축 룰 기본 세팅 적용
	WarningDuration = 2.0f;
	StepInterval = 0.3f;
	GridWidth = 30;
	GridHeight = 30;
	TileSize = 100.f;
	CurrentSpiralIndex = 0;
}

void ASpartaArcadePlayZone::BeginPlay()
{
	Super::BeginPlay();
}

void ASpartaArcadePlayZone::StartSpiralSuddenDeath()
{
	InitializeSpiralPath();
	CurrentSpiralIndex = 0;

	// 나선형 한 칸씩 경고 생성 루프 기동
	GetWorld()->GetTimerManager().SetTimer(SpiralStepTimerHandle, this, &ASpartaArcadePlayZone::AdvanceSpiralStep, StepInterval, true);
	UE_LOG(LogTemp, Warning, TEXT("자기장 시스템 가동 시작!"));
}

void ASpartaArcadePlayZone::InitializeSpiralPath()
{
	int32 Top = 0;
	int32 Bottom = GridHeight - 1;
	int32 Left = 0;
	int32 Right = GridWidth - 1;
	
	SpiralPath.Empty();
	
	// 외곽 테두리부터 중앙까지 시계방향 나선형 경로 좌표 계산
	while (Top <= Bottom && Left <= Right)
	{
		for (int32 i = Left; i <= Right; ++i)
		{
			SpiralPath.Add(FIntPoint(i, Top));
		}
		Top++;
		
		for (int32 i = Top; i <= Bottom; ++i)
		{
			SpiralPath.Add(FIntPoint(Right, i));
		}
		Right--;
		
		if (Top <= Bottom)
		{
			for (int32 i = Right; i >= Left; --i)
			{
				SpiralPath.Add(FIntPoint(i, Bottom));
			}
			Bottom--;
		}
		
		if (Left <= Right)
		{
			for (int32 i = Bottom; i >= Top; --i)
			{
				SpiralPath.Add(FIntPoint(Left, i));
			}
			Left++;
		}
	}
}

void ASpartaArcadePlayZone::AdvanceSpiralStep()
{
	if (!GetWorld() || CurrentSpiralIndex >= SpiralPath.Num())
	{
		GetWorld()->GetTimerManager().ClearTimer(SpiralStepTimerHandle);
		return;
	}

	FIntPoint Coord = SpiralPath[CurrentSpiralIndex];
	CurrentSpiralIndex++;

	FVector MapStartLoc = FVector::ZeroVector;
	AActor* MapGenActor = UGameplayStatics::GetActorOfClass(GetWorld(), ASpartaArcadeMapGenerator::StaticClass());
	if (MapGenActor)
	{
		MapStartLoc = MapGenActor->GetActorLocation();
	}

	// 100유닛 격자 크기만큼 오프셋 적용하여 실제 스폰 좌표 산출
	FVector TargetWorldPos = MapStartLoc + FVector(Coord.X * TileSize, Coord.Y * TileSize, 0.f);

	// 빨간색 경고 비주얼 타일 임시 스폰
	AActor* WarningActor = nullptr;
	if (WarningDecalClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		WarningActor = GetWorld()->SpawnActor<AActor>(WarningDecalClass, TargetWorldPos, FRotator::ZeroRotator, SpawnParams);
	}

	// 경고 유지 시간 경과 후 압사 블록을 낙하시키는 델리게이트 구동
	FTimerHandle DropTimerHandle;
	FTimerDelegate DropDelegate;
	DropDelegate.BindUObject(this, &ASpartaArcadePlayZone::DropDeathBlockAtTile, Coord, WarningActor);
	GetWorld()->GetTimerManager().SetTimer(DropTimerHandle, DropDelegate, WarningDuration, false);
}

void ASpartaArcadePlayZone::DropDeathBlockAtTile(FIntPoint GridCoord, AActor* WarningActor)
{
	if (!GetWorld()) return;

	// 경고용 이펙트/데칼 제거
	if (WarningActor)
	{
		WarningActor->Destroy();
	}

	FVector MapStartLoc = FVector::ZeroVector;
	AActor* MapGenActor = UGameplayStatics::GetActorOfClass(GetWorld(), ASpartaArcadeMapGenerator::StaticClass());
	if (MapGenActor)
	{
		MapStartLoc = MapGenActor->GetActorLocation();
	}

	FVector TargetWorldPos = MapStartLoc + FVector(GridCoord.X * TileSize, GridCoord.Y * TileSize, 0.f);

	// 1. 타격 범위 내 캐릭터 즉사(압사) 및 주변 폭탄/상자/아이템 소멸 판정
	TArray<FHitResult> HitResults;
	FCollisionShape DetectionBox = FCollisionShape::MakeBox(FVector(TileSize * 0.45f, TileSize * 0.45f, 200.f));
	FCollisionQueryParams Params;
	
	FVector SweepStart = TargetWorldPos + FVector(0.f, 0.f, 10.f);
	FVector SweepEnd = TargetWorldPos + FVector(0.f, 0.f, 100.f);

	if (GetWorld()->SweepMultiByChannel(HitResults, SweepStart, SweepEnd, FQuat::Identity, ECC_Visibility, DetectionBox, Params))
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && HitActor->IsA(ASpartaArcadeCharacter::StaticClass()))
			{
				ASpartaArcadeCharacter* Character = Cast<ASpartaArcadeCharacter>(HitActor);
				if (Character)
				{
					UE_LOG(LogTemp, Warning, TEXT("%s 가 (%d, %d) 좌표에서 즉사 블록에 깔려 사망했습니다!"), *Character->GetName(), GridCoord.X, GridCoord.Y);
					Character->Destroy(); // 즉사 처리
				}
			}
			else if (HitActor && (HitActor->IsA(ASpartaArcadeBomb::StaticClass()) || HitActor->IsA(ASpartaArcadeBlock::StaticClass()) || HitActor->IsA(ASpartaArcadeItem::StaticClass())))
			{
				HitActor->Destroy(); // 격자 내 다른 액터 청소
			}
		}
	}

	// 2. 파괴 불가 압사 블록 고정 배치
	if (DeathBlockClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		FVector BlockSpawnPos = TargetWorldPos;
		BlockSpawnPos.Z = MapStartLoc.Z + 50.f; // 지면에 스냅 고정

		GetWorld()->SpawnActor<AActor>(DeathBlockClass, BlockSpawnPos, FRotator::ZeroRotator, SpawnParams);
	}
}

void ASpartaArcadePlayZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}