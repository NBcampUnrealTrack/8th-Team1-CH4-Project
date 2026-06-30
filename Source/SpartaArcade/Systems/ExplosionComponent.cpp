#include "Systems/ExplosionComponent.h"
#include "Systems/Damageable.h"
#include "Kismet/KismetSystemLibrary.h"

UExplosionComponent::UExplosionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UExplosionComponent::StartExplosion(int32 ExplosionRange, float TileSize)
{
    // if (!GetOwner()->HasAuthority()) return;

    TArray<FVector> Directions = {
        FVector(1.f, 0.f, 0.f),
        FVector(-1.f, 0.f, 0.f),
        FVector(0.f, 1.f, 0.f),
        FVector(0.f, -1.f, 0.f)
    };

    for (const FVector& Dir : Directions)
    {
        DetectInDirection(Dir, ExplosionRange, TileSize);
    }

    ProcessLocation(GetOwner()->GetActorLocation());
}

void UExplosionComponent::DetectInDirection(
    const FVector& Direction, int32 Range, float TileSize)
{
    const FVector OriginLocation = GetOwner()->GetActorLocation();

    for (int32 i = 1; i <= Range; i++)
    {
        FVector CheckLocation = OriginLocation + Direction * TileSize * i;

        bool bBlocked = ProcessLocation(CheckLocation);

        if (bBlocked)
        {
            break;
        }
    }
}

bool UExplosionComponent::ProcessLocation(const FVector& Location)
{
    TArray<AActor*> OverlappingActors;
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

    UKismetSystemLibrary::BoxOverlapActors(
        GetWorld(),
        Location,
        FVector(40.f, 40.f, 40.f),
        ObjectTypes,
        nullptr,
        TArray<AActor*>(),
        OverlappingActors
    );

    bool bShouldBlock = false;

    for (AActor* Actor : OverlappingActors)
    {
        if (!Actor->Implements<UDamageable>())
        {
            continue;
        }

        if (IDamageable::Execute_CanTakeDamage(Actor))
        {
            IDamageable::Execute_TakeExplosionDamage(Actor);
        }

        if (IDamageable::Execute_BlocksExplosion(Actor))
        {
            bShouldBlock = true;
        }
    }

    return bShouldBlock;
}