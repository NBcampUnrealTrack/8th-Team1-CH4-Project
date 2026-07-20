#include "BreakableBox.h"
#include "Components/StaticMeshComponent.h"
#include "ItemDropComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SpartaArcadeMapBuilder.h"

ABreakableBox::ABreakableBox()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetNetUpdateFrequency(66.0f);
	SetMinNetUpdateFrequency(2.0f);

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

	if (!HasAuthority()) return;

	bIsDestroyed = true;

	// 맵빌더에 블록 파괴 통지하여 그리드 및 비주얼 동기화
	AActor* MapBuilderActor = UGameplayStatics::GetActorOfClass(GetWorld(), ASpartaArcadeMapBuilder::StaticClass());
	if (ASpartaArcadeMapBuilder* MapBuilder = Cast<ASpartaArcadeMapBuilder>(MapBuilderActor))
	{
		MapBuilder->NotifyTileDestroyedAtWorld(GetActorLocation());
	}

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