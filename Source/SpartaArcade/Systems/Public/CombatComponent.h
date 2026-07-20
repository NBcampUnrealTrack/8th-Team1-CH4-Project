#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BomberTypes.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "ActiveGameplayEffectHandle.h"
#include "SpartaUIDefs.h"
#include "CombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatEvent);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnEliminatedEvent, class ASpartaPlayerState* /*DeadPlayer*/, class ASpartaPlayerState* /*KillerPlayerState*/, EDeathReason /*Reason*/);

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
    
    // 폭발/장애물 등에 의한 피해 처리
    UFUNCTION(BlueprintCallable)
    void ApplyDamage();

    // 현재 피해를 받을 수 있는 상태인지
    UFUNCTION(BlueprintPure)
    bool CanTakeDamage() const;

    // 충돌 처리 (캐릭터 Overlap에서 호출) 
    UFUNCTION(BlueprintCallable)
    void OnOverlapWithEnemy(AActor* Enemy);

    UFUNCTION(BlueprintCallable)
    void OnOverlapWithAlly(AActor* Ally);

    // 구급상자 사용 시 호출
    UFUNCTION(BlueprintCallable)
    void SelfRevive();

    // 자기장 압사 
    UFUNCTION(BlueprintCallable)
    void InstantEliminate();

    UFUNCTION(BlueprintCallable)
    void GrantShield();

    UFUNCTION(BlueprintCallable)
    void Heal(int32 Amount);

    UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Combat Player State"))
    EBomberPlayerState GetPlayerState() const;

    UFUNCTION(BlueprintPure)
    bool IsShielded() const;

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

	FOnEliminatedEvent OnEliminatedEvent;


    virtual void GetLifetimeReplicatedProps(
         TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void InitializePlayerState(APlayerState* NewPlayerState);
    
    void BroadcastCurrentState();

    void SetLastAttacker(ASpartaPlayerState* Attacker, EDeathReason Reason);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category="Data")
    TObjectPtr<UDataTable> CombatStatTable;

    UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
    EBomberPlayerState CurrentState = EBomberPlayerState::Alive;

    UPROPERTY()
    TObjectPtr<ASpartaPlayerState> SpartaPlayerState;

    UPROPERTY()
    TObjectPtr<ASpartaPlayerState> LastAttackerPlayerState;

    EDeathReason LastDeathReason = EDeathReason::Explosion;

private:
    void EnterStun();
    void Eliminate();
    void Revive();
    void EndInvincible();

    // CachedASC로부터 BomberAttributeSet을 가져오는 헬퍼 (Init 계열 함수 호출을 위해 non-const로 반환)
    class UBomberAttributeSet* GetMutableAttributeSet() const;

    bool bDamageThisFrame = false;
    void ResetDamageFlag();

    void OnRep_HasShield();
    
    void OnAnyGameplayEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo);
    // GE_Shield 미설정 시 수동 태그 제거 함수
    void ClearShieldTags();

    float StunDuration = 3.f;
    float InvincibleDuration = 1.f;

    FTimerHandle StunTimerHandle;
    FTimerHandle ShieldTagClearTimerHandle;
    FTimerHandle InvincibleTimerHandle;

    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> CachedASC;

    UPROPERTY(EditDefaultsOnly, Category="GAS")
    TSubclassOf<UGameplayEffect> StunEffectClass;
    
    FActiveGameplayEffectHandle ActiveStunEffectHandle;
    
    UPROPERTY(EditDefaultsOnly, Category="GAS")
    TSubclassOf<UGameplayEffect> ShieldEffectClass;

    FActiveGameplayEffectHandle ActiveShieldEffectHandle;

    UPROPERTY(EditDefaultsOnly, Category="GAS")
    TSubclassOf<UGameplayEffect> InvincibleEffectClass;

    FActiveGameplayEffectHandle ActiveInvincibleEffectHandle;

};