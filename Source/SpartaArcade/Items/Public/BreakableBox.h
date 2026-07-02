#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Damageable.h"
#include "BreakableBox.generated.h"

class UStaticMeshComponent;
class UItemDropComponent;

UCLASS()
class SPARTAARCADE_API ABreakableBox : public AActor, public IDamageable
{
	GENERATED_BODY()

public:
	ABreakableBox();

	//IDamageable 구현 
	virtual void TakeExplosionDamage_Implementation() override;
	virtual bool CanTakeDamage_Implementation() const override;
	virtual bool BlocksExplosion_Implementation() const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> BoxMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UItemDropComponent> ItemDropComp;

private:
	bool bIsDestroyed = false;
};