#include "StatComponent.h"
// #include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UStatComponent::UStatComponent()
{
    // SetIsReplicatedByDefault(true);

    PrimaryComponentTick.bCanEverTick = false;
}

// void UStatComponent::GetLifetimeReplicatedProps(
//     TArray<FLifetimeProperty>& OutLifetimeProps) const
// {
//     Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//
//     DOREPLIFETIME(UStatComponent, BombRange);
//     DOREPLIFETIME(UStatComponent, BombCount);
//     DOREPLIFETIME(UStatComponent, MoveSpeed);
// }

void UStatComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UStatComponent::InitializeFromDataTable(FName CharacterRowName)
{
    // if (!GetOwner()->HasAuthority()) return;

    if (!IsValid(CharacterStatTable))
    {
        UE_LOG(LogTemp, Warning, TEXT("StatComponent: CharacterStatTable이 없어요!"));
        return;
    }

    FCharacterStatRow* Row = CharacterStatTable->FindRow<FCharacterStatRow>(
        CharacterRowName, TEXT("InitializeFromDataTable"));

    if (Row == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("StatComponent: Row [%s]를 찾을 수 없어요!"),
            *CharacterRowName.ToString());
        return;
    }

    BombRange = Row->StartBombRange;
    BombCount = Row->StartBombCount;
    MoveSpeed = Row->StartMoveSpeed;

    MaxBombRange = Row->MaxBombRange;
    MaxBombCount = Row->MaxBombCount;
    MaxMoveSpeed = Row->MaxMoveSpeed;

    OnRep_MoveSpeed();
}

void UStatComponent::GrowStat(EBomberStatType StatType)
{
    // if (!GetOwner()->HasAuthority()) return;

    switch (StatType)
    {
    case EBomberStatType::BombRange:
        if (BombRange >= MaxBombRange) return;
        BombRange++;
        OnStatChanged.Broadcast(StatType, BombRange);
        break;

    case EBomberStatType::BombCount:
        if (BombCount >= MaxBombCount) return;
        BombCount++;
        OnStatChanged.Broadcast(StatType, BombCount);
        break;

    case EBomberStatType::MoveSpeed:
        if (MoveSpeed >= MaxMoveSpeed) return;
        MoveSpeed++;
        OnRep_MoveSpeed();
        OnStatChanged.Broadcast(StatType, MoveSpeed);
        break;
    }
}

void UStatComponent::OnRep_BombRange()
{
    OnStatChanged.Broadcast(EBomberStatType::BombRange, BombRange);
}

void UStatComponent::OnRep_BombCount()
{
    OnStatChanged.Broadcast(EBomberStatType::BombCount, BombCount);
}

void UStatComponent::OnRep_MoveSpeed()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!IsValid(OwnerCharacter)) return;

    UCharacterMovementComponent* MoveComp =
        OwnerCharacter->GetCharacterMovement();
    if (!IsValid(MoveComp)) return;

    const float BaseSpeed = 200.f;
    const float SpeedPerLevel = 100.f;
    MoveComp->MaxWalkSpeed = BaseSpeed + (MoveSpeed * SpeedPerLevel);

    OnStatChanged.Broadcast(EBomberStatType::MoveSpeed, MoveSpeed);
}