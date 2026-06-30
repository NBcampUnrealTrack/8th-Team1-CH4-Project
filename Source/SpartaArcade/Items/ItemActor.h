#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Systems/BomberTypes.h"
#include "ItemActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class SPARTAARCADE_API AItemActor : public AActor
{
	GENERATED_BODY()

public:
	AItemActor();

	void InitializeItem(FName InItemRowName, UDataTable* InItemDataTable);

protected:
	virtual void BeginPlay() override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ItemMesh;

private:
	FName ItemRowName;

	UPROPERTY()
	TObjectPtr<UDataTable> ItemDataTable;
};