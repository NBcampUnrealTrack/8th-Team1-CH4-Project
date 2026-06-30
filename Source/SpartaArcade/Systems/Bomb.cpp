#include "Systems/Bomb.h"
#include "Systems/ExplosionComponent.h"
#include "Systems/BomberTypes.h"

ABomb::ABomb()
{
	PrimaryActorTick.bCanEverTick = false;

	// bReplicates = true;

	ExplosionComp = CreateDefaultSubobject<UExplosionComponent>(TEXT("ExplosionComp"));
}

void ABomb::BeginPlay()
{
	Super::BeginPlay();
}

void ABomb::Activate(int32 InExplosionRange, UDataTable* InBombStatTable)
{
	ExplosionRange = InExplosionRange;

	float FuseTime = 3.f;

	if (IsValid(InBombStatTable))
	{
		TArray<FBombStatRow*> Rows;
		InBombStatTable->GetAllRows<FBombStatRow>(TEXT("Activate"), Rows);

		if (Rows.Num() > 0)
		{
			FuseTime = Rows[0]->FuseTime;
			TileSize = Rows[0]->TileSize;
		}
	}

	GetWorldTimerManager().SetTimer(
		FuseTimerHandle, this, &ABomb::Explode, FuseTime, false);
}

void ABomb::Explode()
{
	ExplosionComp->StartExplosion(ExplosionRange, TileSize);

	MulticastPlayExplosionEffect();

	OnBombExploded.ExecuteIfBound();

	Destroy();
}

void ABomb::MulticastPlayExplosionEffect()
{
	// TODO: 이펙트/사운드는 캐릭터 파트에서 구현
}