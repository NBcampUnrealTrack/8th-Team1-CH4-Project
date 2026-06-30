#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Systems/BomberTypes.h"
#include "StatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnStatChanged,
    EBomberStatType, StatType,
    int32, NewValue);

UCLASS(ClassGroup=(Bomber), meta=(BlueprintSpawnableComponent))
class SPARTAARCADE_API UStatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStatComponent();

    UFUNCTION(BlueprintCallable)
    void InitializeFromDataTable(FName CharacterRowName);

    UFUNCTION(BlueprintCallable)
    void GrowStat(EBomberStatType StatType);

    UFUNCTION(BlueprintPure)
    int32 GetBombRange()  const { return BombRange; }

    UFUNCTION(BlueprintPure)
    int32 GetBombCount()  const { return BombCount; }

    UFUNCTION(BlueprintPure)
    int32 GetMoveSpeed()  const { return MoveSpeed; }

    UPROPERTY(BlueprintAssignable)
    FOnStatChanged OnStatChanged;
    
    // virtual void GetLifetimeReplicatedProps(
    //     TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category="Data")
    TObjectPtr<UDataTable> CharacterStatTable;
    
    // UPROPERTY(ReplicatedUsing=OnRep_BombRange)
    UPROPERTY()
    int32 BombRange = 2;

    // UPROPERTY(ReplicatedUsing=OnRep_BombCount)
    UPROPERTY()
    int32 BombCount = 3;

    // UPROPERTY(ReplicatedUsing=OnRep_MoveSpeed)
    UPROPERTY()
    int32 MoveSpeed = 3;

    int32 MaxBombRange = 5;
    int32 MaxBombCount = 8;
    int32 MaxMoveSpeed = 5;

private:
    UFUNCTION()
    void OnRep_BombRange();

    UFUNCTION()
    void OnRep_BombCount();

    UFUNCTION()
    void OnRep_MoveSpeed();
};