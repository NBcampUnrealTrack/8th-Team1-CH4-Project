#include "SpartaArcadeMapGenerator.h"
#include "Engine/World.h"

ASpartaArcadeMapGenerator::ASpartaArcadeMapGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

	GridWidth = 30;
	GridHeight = 30;
	TileSize = 100.f;
	BlockSpawnChance = 0.45f; // 45% 확률
}

void ASpartaArcadeMapGenerator::BeginPlay()
{
	Super::BeginPlay();

	// 시작 시 자동으로 맵 생성 (데디케이티드 서버 환경에서 서버가 권한을 가짐)
	if (HasAuthority())
	{
		GenerateMap();
	}
}

void ASpartaArcadeMapGenerator::GenerateMap()
{
	if (!GetWorld()) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FVector StartLoc = GetActorLocation();

	for (int32 X = 0; X < GridWidth; ++X)
	{
		for (int32 Y = 0; Y < GridHeight; ++Y)
		{
			// 대상 타일의 스폰 위치 계산
			FVector TileLocation = StartLoc + FVector(X * TileSize, Y * TileSize, 0.f);
			FRotator TileRotation = FRotator::ZeroRotator;

			// 1. 모든 그리드 셀 하단에 항상 바닥(Floor) 스폰
			if (FloorClass)
			{
				GetWorld()->SpawnActor<AActor>(FloorClass, TileLocation, TileRotation, SpawnParams);
			}

			// 2. 고정 벽인지 체크 (외곽 경계선 또는 짝수 좌표 고정벽)
			bool bIsFixedWall = false;
			if (X == 0 || X == GridWidth - 1 || Y == 0 || Y == GridHeight - 1)
			{
				bIsFixedWall = true;
			}
			else if (X % 2 == 0 && Y % 2 == 0)
			{
				bIsFixedWall = true;
			}

			if (bIsFixedWall)
			{
				if (FixedWallClass)
				{
					GetWorld()->SpawnActor<AActor>(FixedWallClass, TileLocation, TileRotation, SpawnParams);
				}
				continue; // 고정 벽이므로 블록 스폰 과정 건너뜀
			}

			// 3. 플레이어 안전 스폰 구역 내 좌표라면 블록 생성을 건너뜀
			if (IsSafeZone(X, Y))
			{
				continue;
			}

			// 4. 남은 공간에 블록 스폰 확률(45%)에 따라 무작위로 파괴 가능한 블록 스폰
			if (DestructibleBlockClass && FMath::FRand() <= BlockSpawnChance)
			{
				GetWorld()->SpawnActor<AActor>(DestructibleBlockClass, TileLocation, TileRotation, SpawnParams);
			}
		}
	}
}

bool ASpartaArcadeMapGenerator::IsSafeZone(int32 X, int32 Y) const
{
	// 좌측 상단 모서리 안전 구역
	if (X >= 1 && X <= 2 && Y >= 1 && Y <= 2)
	{
		return true;
	}

	// 우측 상단 모서리 안전 구역
	if (X >= GridWidth - 3 && X <= GridWidth - 2 && Y >= 1 && Y <= 2)
	{
		return true;
	}

	// 좌측 하단 모서리 안전 구역
	if (X >= 1 && X <= 2 && Y >= GridHeight - 3 && Y <= GridHeight - 2)
	{
		return true;
	}

	// 우측 하단 모서리 안전 구역
	if (X >= GridWidth - 3 && X <= GridWidth - 2 && Y >= GridHeight - 3 && Y <= GridHeight - 2)
	{
		return true;
	}

	return false;
}
