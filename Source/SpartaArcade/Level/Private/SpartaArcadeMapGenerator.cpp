#include "SpartaArcadeMapGenerator.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

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
	// 중복 생성 방지 및 선제 빌드 상태 기록
	if (bMapGenerated) return;
	bMapGenerated = true;

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

			// 플레이어 스폰 위치와 고정벽이 겹치는 경우에만 고정벽 삭제(스폰 생략)
			bool bIsSpawnLocation = 
				(X == 1 && Y == 1) ||
				(X == GridWidth - 2 && Y == 1) ||
				(X == 1 && Y == GridHeight - 2) ||
				(X == GridWidth - 2 && Y == GridHeight - 2);

			if (bIsFixedWall && !bIsSpawnLocation)
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

	// 4인 플레이어 모서리 스폰을 위한 플레이어 스타트 재배치 함수 호출
	RepositionPlayerStarts();

	// 스폰 포인트 겹침 구조물 제거 함수 호출
	ClearStructuresAtSpawns();
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

// 4인 플레이어 모서리 스폰을 위한 플레이어 스타트 재배치 함수 구현
void ASpartaArcadeMapGenerator::RepositionPlayerStarts()
{
	if (!GetWorld()) return;

	// 월드의 모든 PlayerStart 검색
	TArray<AActor*> FoundPlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundPlayerStarts);

	// 4개 모서리 좌표 계산
	TArray<FVector> CornerLocations;
	FVector StartLoc = GetActorLocation();
	TArray<FIntPoint> Corners = {
		FIntPoint(1, 1),
		FIntPoint(GridWidth - 2, 1),
		FIntPoint(1, GridHeight - 2),
		FIntPoint(GridWidth - 2, GridHeight - 2)
	};

	// 옆 타일에 배치된 외곽 고정벽과의 캡슐 충돌(Adjust Position)로 인해 
	// 캐릭터가 벽 위로 얹어지는 물리 현상을 방어하기 위해 맵 안쪽 방향으로 25.f uu 미세 오프셋을 적용
	const float SafeOffset = 25.f; 
	const FVector CornerOffsets[4] = {
		FVector(SafeOffset, SafeOffset, 0.f),   // Corner0 (1, 1) -> 우하단 밀기
		FVector(-SafeOffset, SafeOffset, 0.f),  // Corner1 (W-2, 1) -> 좌하단 밀기
		FVector(SafeOffset, -SafeOffset, 0.f),  // Corner2 (1, H-2) -> 우상단 밀기
		FVector(-SafeOffset, -SafeOffset, 0.f)  // Corner3 (W-2, H-2) -> 좌상단 밀기
	};

	for (int32 i = 0; i < 4; ++i)
	{
		// Z축은 10.f 정도로 더 낮춰서 지면 완착 유도
		FVector Loc = StartLoc + FVector(Corners[i].X * TileSize, Corners[i].Y * TileSize, 10.f);
		Loc += CornerOffsets[i];
		CornerLocations.Add(Loc);
	}

	// 기존 PlayerStart 액터들을 사용하거나 부족하면 생성
	for (int32 i = 0; i < 4; ++i)
	{
		APlayerStart* TargetPlayerStart = nullptr;
		if (i < FoundPlayerStarts.Num())
		{
			TargetPlayerStart = Cast<APlayerStart>(FoundPlayerStarts[i]);
		}
		else
		{
			// 부족한 경우 동적 생성
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			TargetPlayerStart = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), CornerLocations[i], FRotator::ZeroRotator, SpawnParams);
		}

		if (TargetPlayerStart)
		{
			TargetPlayerStart->SetActorLocation(CornerLocations[i]);
			// 태그 설정 (예: Corner0, Corner1, Corner2, Corner3)
			TargetPlayerStart->PlayerStartTag = FName(*FString::Printf(TEXT("Corner%d"), i));
		}
	}

	// 4개를 초과하는 PlayerStart가 있다면 삭제 (혼선 방지)
	for (int32 i = 4; i < FoundPlayerStarts.Num(); ++i)
	{
		if (FoundPlayerStarts[i])
		{
			// 기존 코드 보존 규칙에 따라 Destroy 호출을 로그와 함께 실행
			UE_LOG(LogTemp, Warning, TEXT("[SpawnSystem] 초과된 PlayerStart 액터를 삭제합니다."));
			FoundPlayerStarts[i]->Destroy();
		}
	}
}

// 스폰 포인트 주변의 구조물/장애물 액터 강제 파괴 함수 구현
void ASpartaArcadeMapGenerator::ClearStructuresAtSpawns()
{
	if (!GetWorld()) return;

	// 타일 크기 1칸 전체(90%)를 덮어 겹치는 모든 구조물 액터를 완전히 제거
	const float ClearRadius = TileSize * 0.90f; 

	// 4개 모서리 스폰 위치 (RepositionPlayerStarts 에서 사용하는 좌표와 동일)
	TArray<FVector> CornerLocations;
	FVector StartLoc = GetActorLocation();
	TArray<FIntPoint> Corners = {
		FIntPoint(1, 1),
		FIntPoint(GridWidth - 2, 1),
		FIntPoint(1, GridHeight - 2),
		FIntPoint(GridWidth - 2, GridHeight - 2)
	};

	for (const FIntPoint& Corner : Corners)
	{
		FVector Loc = StartLoc + FVector(Corner.X * TileSize, Corner.Y * TileSize, 100.f);
		CornerLocations.Add(Loc);
	}

	for (const FVector& SpawnLoc : CornerLocations)
	{
		TArray<AActor*> OverlappingActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), OverlappingActors);

		for (AActor* Actor : OverlappingActors)
		{
			if (!IsValid(Actor) || Actor == this || Actor->IsA(APlayerStart::StaticClass())) continue;

			FVector ActorLoc = Actor->GetActorLocation();
			float Distance2D = FVector::Dist2D(SpawnLoc, ActorLoc);
			float DistanceZ = FMath::Abs(SpawnLoc.Z - ActorLoc.Z);

			if (Distance2D < ClearRadius && DistanceZ < 200.f)
			{
				FString ClassName = Actor->GetClass()->GetName();
				if (ClassName.Contains(TEXT("Wall")) || 
					ClassName.Contains(TEXT("Block")) || 
					ClassName.Contains(TEXT("Box")) || 
					ClassName.Contains(TEXT("Pillar")) ||
					ClassName.Contains(TEXT("Obstacle")))
				{
					UE_LOG(LogTemp, Warning, TEXT("[MapGenerator] 스폰 위치(%s)에 겹치는 구조물 액터 %s 를 파괴합니다."), *SpawnLoc.ToString(), *Actor->GetName());
					Actor->Destroy();
				}
			}
		}
	}
}

