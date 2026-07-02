#include "BombPlacerComponent.h"
#include "StatComponent.h"
#include "Bomb.h"
// #include "Net/UnrealNetwork.h"

UBombPlacerComponent::UBombPlacerComponent()
{
	// SetIsReplicatedByDefault(true);

	PrimaryComponentTick.bCanEverTick = false;
}

// void UBombPlacerComponent::GetLifetimeReplicatedProps(
//     TArray<FLifetimeProperty>& OutLifetimeProps) const
// {
//     Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//     DOREPLIFETIME(UBombPlacerComponent, CurrentPlacedBombs);
// }

void UBombPlacerComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedStatComponent = GetOwner()->FindComponentByClass<UStatComponent>();
}

void UBombPlacerComponent::ServerPlaceBomb()
{
	if (!CanPlaceBomb()) return;
	if (!IsValid(BombClass)) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ABomb* NewBomb = GetWorld()->SpawnActor<ABomb>(
		BombClass,
		GetOwner()->GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (!IsValid(NewBomb)) return;

	NewBomb->OnBombExploded.BindUObject(this, &UBombPlacerComponent::OnBombExploded);

	int32 ExplosionRange = IsValid(CachedStatComponent)
		? CachedStatComponent->GetBombRange()
		: 2;

	NewBomb->Activate(ExplosionRange, BombStatTable);
	CurrentPlacedBombs++;
}

bool UBombPlacerComponent::CanPlaceBomb() const
{
	if (!IsValid(CachedStatComponent)) return false;

	return CurrentPlacedBombs < CachedStatComponent->GetBombCount();
}

void UBombPlacerComponent::OnBombExploded()
{
	CurrentPlacedBombs--;
}