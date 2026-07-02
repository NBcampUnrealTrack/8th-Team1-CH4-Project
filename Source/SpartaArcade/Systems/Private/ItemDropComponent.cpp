#include "ItemDropComponent.h"
#include "ItemActor.h"

UItemDropComponent::UItemDropComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UItemDropComponent::TryDropItem()
{
    // if (!GetOwner()->HasAuthority()) return;

    FName SelectedRow = SelectRandomItemRow();

    if (SelectedRow != NAME_None)
    {
        SpawnItem(SelectedRow);
    }
}

FName UItemDropComponent::SelectRandomItemRow() const
{
    if (!IsValid(ItemDataTable))
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemDropComponent: ItemDataTable이 없어요!"));
        return NAME_None;
    }

    TArray<FName> RowNames = ItemDataTable->GetRowNames();

    int32 TotalWeight = 0;
    for (const FName& RowName : RowNames)
    {
        FItemDataRow* Row = ItemDataTable->FindRow<FItemDataRow>(
            RowName, TEXT("SelectRandomItemRow"));

        if (Row != nullptr)
        {
            TotalWeight += Row->DropWeight;
        }
    }

    if (TotalWeight <= 0) return NAME_None;

    int32 RandValue = FMath::RandRange(0, TotalWeight - 1);

    int32 Accumulated = 0;
    for (const FName& RowName : RowNames)
    {
        FItemDataRow* Row = ItemDataTable->FindRow<FItemDataRow>(
            RowName, TEXT("SelectRandomItemRow"));

        if (Row == nullptr) continue;

        Accumulated += Row->DropWeight;

        if (RandValue < Accumulated)
        {
            return RowName;
        }
    }

    return NAME_None;
}

void UItemDropComponent::SpawnItem(FName ItemRowName)
{
    if (!IsValid(ItemActorClass)) return;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AItemActor* SpawnedItem = GetWorld()->SpawnActor<AItemActor>(
        ItemActorClass,
        GetOwner()->GetActorLocation(),
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (IsValid(SpawnedItem))
    {
        SpawnedItem->InitializeItem(ItemRowName, ItemDataTable);
    }
}

void UItemDropComponent::DropKillReward(AActor* DefeatedActor)
{
    // if (!GetOwner()->HasAuthority()) return;

    if (!IsValid(DefeatedActor)) return;

    TryDropItem();
}