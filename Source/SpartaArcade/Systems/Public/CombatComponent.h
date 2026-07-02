#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BomberTypes.h"
#include "CombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatEvent);


UCLASS(ClassGroup=(Bomber), meta=(BlueprintSpawnableComponent))
class SPARTAARCADE_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatComponent();

    // 게임 시작 시 DataTable에서 수치 로드
    UFUNCTION(BlueprintCallable)
    void InitializeFromDataTable(UDataTable* InCombatStatTable);

    // ─── 외부에서 호출하는 핵심 함수 ──────────────
    // 폭발/장애물 등에 의한 피해 처리
    // TODO: 캐릭터 파트의 IDamageable 구현부에서 이 함수를 호출하도록 협의 필요
    UFUNCTION(BlueprintCallable)
    void ApplyDamage();

    // 현재 피해를 받을 수 있는 상태인지
    UFUNCTION(BlueprintPure)
    bool CanTakeDamage() const;

    // ─── 충돌 처리 (캐릭터 Overlap에서 호출) ─────
    UFUNCTION(BlueprintCallable)
    void OnOverlapWithEnemy(AActor* Enemy);

    UFUNCTION(BlueprintCallable)
    void OnOverlapWithAlly(AActor* Ally);

    // 구급상자 사용 시 호출
    UFUNCTION(BlueprintCallable)
    void SelfRevive();

    // 자기장 압사 - 기절 없이 즉시 탈락
    UFUNCTION(BlueprintCallable)
    void InstantEliminate();

    // 방어막 획득 시 호출 (아이템 시스템과 연동)
    UFUNCTION(BlueprintCallable)
    void GrantShield();

    UFUNCTION(BlueprintPure)
    EBomberPlayerState GetPlayerState() const { return CurrentState; }

    UFUNCTION(BlueprintPure)
    int32 GetHearts() const { return Hearts; }

    // 캐릭터 위임 연동을 위한 Getter 및 Heal 함수 추가
    UFUNCTION(BlueprintCallable)
    void Heal(int32 Amount);

    UFUNCTION(BlueprintPure)
    int32 GetMaxHearts() const { return StartHearts; }

    UFUNCTION(BlueprintPure)
    bool IsShielded() const { return bHasShield; }
    
    UPROPERTY(BlueprintAssignable)
    FOnCombatEvent OnHit;

    UPROPERTY(BlueprintAssignable)
    FOnCombatEvent OnStun;

    UPROPERTY(BlueprintAssignable)
    FOnCombatEvent OnEliminated;

    UPROPERTY(BlueprintAssignable)
    FOnCombatEvent OnRevived;

    UPROPERTY(BlueprintAssignable)
    FOnCombatEvent OnSelfRevive;

    UPROPERTY(BlueprintAssignable)
    FOnCombatEvent OnShieldBlock;

    // virtual void GetLifetimeReplicatedProps(
    //     TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category="Data")
    TObjectPtr<UDataTable> CombatStatTable;

    // 상태 변수 
    // UPROPERTY(ReplicatedUsing=OnRep_Hearts)
    UPROPERTY()
    int32 Hearts;

    // UPROPERTY(ReplicatedUsing=OnRep_CurrentState)
    UPROPERTY()
    EBomberPlayerState CurrentState = EBomberPlayerState::Alive;

    // UPROPERTY(Replicated)
    UPROPERTY()
    bool bInvincible = false;
    
    UPROPERTY()
    bool bHasShield = false;

private:
    void EnterStun();
    void Eliminate();
    void Revive();
    void EndInvincible();

    bool bDamageThisFrame = false;
    void ResetDamageFlag();

    UFUNCTION()
    void OnRep_Hearts();

    UFUNCTION()
    void OnRep_CurrentState();

    // 수치 (DataTable에서 로드)
    int32 StartHearts = 3;
    float StunDuration = 3.f;
    float InvincibleDuration = 1.f;
    int32 SelfReviveHearts = 1;

    FTimerHandle StunTimerHandle;
    FTimerHandle InvincibleTimerHandle;
};