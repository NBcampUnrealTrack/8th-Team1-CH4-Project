#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BomberTypes.h"
#include "CombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnbHasShieldChangedSignature, bool, bHasShield);

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

    // 캐릭터 위임 연동을 위한 Getter 및 Heal 함수 추가
    UFUNCTION(BlueprintCallable)
    void Heal(int32 Amount);

    UFUNCTION(BlueprintPure)
    bool IsShielded() const { return bHasShield; }

    // 기절 진행률 게이지 처리를 위한 퍼센트 반환 함수 추가
    UFUNCTION(BlueprintPure, Category = "Combat")
    float GetStunProgressPercent() const;
    
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

    UPROPERTY(BlueprintAssignable, Category = "Events | UI")
	FOnbHasShieldChangedSignature OnbHasShieldChanged;

    virtual void GetLifetimeReplicatedProps(
         TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void InitializePlayerState(APlayerState* NewPlayerState);
    
    void BroadcastCurrentState();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category="Data")
    TObjectPtr<UDataTable> CombatStatTable;

    // 상태 변수 
    UPROPERTY(Replicated)
    bool bInvincible = false;
    
    UPROPERTY(ReplicatedUsing=OnRep_HasShield)
    bool bHasShield = false;

    UPROPERTY()
    TObjectPtr<class ASpartaPlayerState> SpartaPlayerState;

private:
    void EnterStun();
    void Eliminate();
    void Revive();
    void EndInvincible();

    bool bDamageThisFrame = false;
    void ResetDamageFlag();

    UFUNCTION()
	void OnRep_HasShield();

    // 수치 (DataTable에서 로드)
    float StunDuration = 3.f;
    float InvincibleDuration = 1.f;
    
    FTimerHandle StunTimerHandle;
    FTimerHandle InvincibleTimerHandle;
};