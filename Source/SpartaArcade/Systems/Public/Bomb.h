#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Bomb.generated.h"

class UExplosionComponent;

DECLARE_DELEGATE(FOnBombExploded);

UCLASS()
class SPARTAARCADE_API ABomb : public AActor
{
	GENERATED_BODY()

public:
	ABomb();

	void Activate(int32 InExplosionRange, UDataTable* InBombStatTable);

	FOnBombExploded OnBombExploded;

protected:
	virtual void BeginPlay() override;

private:
	void Explode();

	// UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayExplosionEffect();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UExplosionComponent> ExplosionComp;

	int32 ExplosionRange = 2;
	float TileSize = 100.f;

	FTimerHandle FuseTimerHandle;
};