#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BomberTypes.h"
#include "ItemDropComponent.generated.h"

class AItemActor;

UCLASS(ClassGroup=(Bomber), meta=(BlueprintSpawnableComponent))
class SPARTAARCADE_API UItemDropComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UItemDropComponent();

	// 박스 파괴시 호출
	UFUNCTION(BlueprintCallable)
	void TryDropItem();

	// 처치 시 상대방의 보유 아이템 중 일부를 드롭
	UFUNCTION(BlueprintCallable)
	void DropKillReward(AActor* DefeatedActor);

protected:
	// DataTable 레퍼런스
	UPROPERTY(EditDefaultsOnly, Category="Data")
	TObjectPtr<UDataTable> ItemDataTable;

	// 스폰할 아이템 액터 클래스
	UPROPERTY(EditDefaultsOnly, Category="Data")
	TSubclassOf<AItemActor> ItemActorClass;

private:
	// 가중치 기반으로 랜덤 아이템 Row 선택
	FName SelectRandomItemRow() const;

	// 선택된 Row로 실제 아이템 액터 스폰
	void SpawnItem(FName ItemRowName);
};