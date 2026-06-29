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

	// 기획 규칙에 따라 절차적으로 맵을 생성
	UFUNCTION(BlueprintCallable, Category = "Map Generation")
	void GenerateMap();

	// 격자 좌표가 플레이어 스폰 안전 구역(4개 모서리 및 인접 3개 타일)인지 판별하는 헬퍼 함수
	bool IsSafeZone(int32 X, int32 Y) const;
};
