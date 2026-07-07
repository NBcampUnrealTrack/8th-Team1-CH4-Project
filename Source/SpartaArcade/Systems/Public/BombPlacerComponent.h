#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BombPlacerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentPlacedBombsChangedSignature, int32, CurrentPlacedBombs);

class ASpartaArcadeBomb;
class UStatComponent;

UCLASS(ClassGroup=(Bomber), meta=(BlueprintSpawnableComponent))
class SPARTAARCADE_API UBombPlacerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBombPlacerComponent();

	UFUNCTION(Server, Reliable)
	void ServerPlaceBomb();

public:
	UPROPERTY(BlueprintAssignable, Category = "Events | UI")
	FOnCurrentPlacedBombsChangedSignature OnCurrentPlacedBombsChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category="Data")
	TSubclassOf<ASpartaArcadeBomb> BombClass;

	UPROPERTY(EditDefaultsOnly, Category="Data")
	TObjectPtr<UDataTable> BombStatTable;

private:
	bool CanPlaceBomb() const;
	void OnBombExploded();

	UPROPERTY(ReplicatedUsing=OnRep_CurrentPlacedBombs)
	int32 CurrentPlacedBombs = 0;

	UPROPERTY()
	TObjectPtr<UStatComponent> CachedStatComponent;

public:
	virtual void GetLifetimeReplicatedProps(
	    TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void BroadcastCurrentState();

private:
	UFUNCTION()
	void OnRep_CurrentPlacedBombs();
};