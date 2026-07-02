#include "BreakableBox.h"
#include "Components/StaticMeshComponent.h"
#include "ItemDropComponent.h"

ABreakableBox::ABreakableBox()
{
	PrimaryActorTick.bCanEverTick = false;

	// bReplicates = true;

	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	SetRootComponent(BoxMesh);

	ItemDropComp = CreateDefaultSubobject<UItemDropComponent>(TEXT("ItemDropComp"));
}

void ABreakableBox::BeginPlay()
{
	Super::BeginPlay();
}

void ABreakableBox::TakeExplosionDamage_Implementation()
{
	if (bIsDestroyed) return;

	// if (!HasAuthority()) return;

	bIsDestroyed = true;

	if (IsValid(ItemDropComp))
	{
		ItemDropComp->TryDropItem();
	}

	Destroy();
}

bool ABreakableBox::CanTakeDamage_Implementation() const
{
	return !bIsDestroyed;
}

bool ABreakableBox::BlocksExplosion_Implementation() const
{
	return true;
}