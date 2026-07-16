#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DeathDropComponent.generated.h"

class AItemActor;
class UDataTable;
class ASpartaArcadeMapBuilder;

UCLASS(ClassGroup=(Bomber), meta=(BlueprintSpawnableComponent))
class SPARTAARCADE_API UDeathDropComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDeathDropComponent();
	
	UFUNCTION(BlueprintCallable, Category = "Death Drop")
	void DropDeathItems(FVector DropLocation);

protected:
	// 스폰할 ItemActor 클래스 (BP_ItemActor를 배정)
	UPROPERTY(EditDefaultsOnly, Category = "Death Drop")
	TSubclassOf<AItemActor> ItemActorClass;

	// 아이템 타입 → DataTable Row Name 매핑
	// Key: "BombCount", "BombRange", "MoveSpeed", "MedKit" 등 DT_Item 의 Row 이름
	UPROPERTY(EditDefaultsOnly, Category = "Death Drop")
	TObjectPtr<UDataTable> ItemDataTable;

	// 4개 아이템을 살짝 흩뿌릴 반경 (단위: cm)
	UPROPERTY(EditDefaultsOnly, Category = "Death Drop", meta = (ClampMin = "0.0"))
	float SpreadRadius = 60.f;

private:
	// 지정 위치에 단일 아이템을 스폰하는 헬퍼
	void SpawnItemAt(FName RowName, FVector Location);
	
	bool FindNearestWalkableTile(
		ASpartaArcadeMapBuilder* MapBuilder,
		const FVector& Origin,
		TSet<FIntPoint>& OutUsed,
		FVector& OutLocation) const;

	// 같은 드롭 호출 내에서 이미 할당된 타일을 추적(동일 콴에 중복 스폰 방지)
	TSet<FIntPoint> UsedTileSet;
};
