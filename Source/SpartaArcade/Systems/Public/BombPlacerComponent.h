#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BombPlacerComponent.generated.h"

class ABomb;
class UStatComponent;

UCLASS(ClassGroup=(Bomber), meta=(BlueprintSpawnableComponent))
class SPARTAARCADE_API UBombPlacerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBombPlacerComponent();

	// UFUNCTION(Server, Reliable)
	UFUNCTION(BlueprintCallable)
	void ServerPlaceBomb();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category="Data")
	TSubclassOf<ABomb> BombClass;

	UPROPERTY(EditDefaultsOnly, Category="Data")
	TObjectPtr<UDataTable> BombStatTable;

private:
	bool CanPlaceBomb() const;
	void OnBombExploded();

	// UPROPERTY(Replicated)
	UPROPERTY()
	int32 CurrentPlacedBombs = 0;

	UPROPERTY()
	TObjectPtr<UStatComponent> CachedStatComponent;

public:
	// virtual void GetLifetimeReplicatedProps(
	//     TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};