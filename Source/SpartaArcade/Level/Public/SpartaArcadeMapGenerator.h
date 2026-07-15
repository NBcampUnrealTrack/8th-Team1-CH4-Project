#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/SubclassOf.h"
#include "SpartaArcadeMapGenerator.generated.h"

UCLASS()
class SPARTAARCADE_API ASpartaArcadeMapGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpartaArcadeMapGenerator();

	// 맵 생성 절차가 이미 수행 완료되었는지 기록하는 플래그 (선제 맵 빌드 타이밍 제어용)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Map Generation")
	bool bMapGenerated = false;

	// 기획 규칙에 따라 절차적으로 맵을 생성 (게임모드가 선제 완료할 수 있도록 public 공개)
	UFUNCTION(BlueprintCallable, Category = "Map Generation")
	void GenerateMap();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Settings")
	int32 GridWidth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Settings")
	int32 GridHeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Settings")
	float TileSize;

	// 스폰할 클래스 타입들
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Settings|Classes")
	TSubclassOf<AActor> FloorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Settings|Classes")
	TSubclassOf<AActor> FixedWallClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Settings|Classes")
	TSubclassOf<AActor> DestructibleBlockClass;

	// 파괴 가능한 블록의 스폰 확률
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BlockSpawnChance;

	// 격자 좌표가 플레이어 스폰 안전 구역(4개 모서리 및 인접 3개 타일)인지 판별하는 헬퍼 함수
	bool IsSafeZone(int32 X, int32 Y) const;

	void RepositionPlayerStarts();

	// Modified: 스폰 포인트 주변의 구조물/장애물 액터 강제 파괴 함수 추가
	void ClearStructuresAtSpawns();
};
