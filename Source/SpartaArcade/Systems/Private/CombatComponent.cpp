#include "CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"
#include "BomberGameplayTags.h"
#include "BomberAttributeSet.h"

UCombatComponent::UCombatComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UCombatComponent, bInvincible);
    DOREPLIFETIME(UCombatComponent, bHasShield);
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    // 추가 — Owner(캐릭터)의 ASC를 미리 찾아서 캐싱
    if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(GetOwner()))
    {
        CachedASC = ASCInterface->GetAbilitySystemComponent();
    }
}

void UCombatComponent::InitializePlayerState(APlayerState* NewPlayerState)
{
    if (!IsValid(NewPlayerState)) return;

    SpartaPlayerState = Cast<ASpartaPlayerState>(NewPlayerState);

    if (!IsValid(SpartaPlayerState))
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatComponent: PlayerState가 SpartaPlayerState가 아니에요!"));
        return;
    }
}

void UCombatComponent::InitializeFromDataTable(UDataTable* InCombatStatTable)
{
    CombatStatTable = InCombatStatTable;

    if (!IsValid(CombatStatTable))
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatComponent: CombatStatTable이 없어요!"));
        return;
    }

    TArray<FCombatStatRow*> Rows;
    CombatStatTable->GetAllRows<FCombatStatRow>(TEXT("InitializeFromDataTable"), Rows);

    if (Rows.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatComponent: DataTable에 Row가 없어요!"));
        return;
    }

    FCombatStatRow* Row = Rows[0];

    if (IsValid(SpartaPlayerState))
    {
        SpartaPlayerState->SetStartHearts(Row->StartHearts);
        SpartaPlayerState->SetSelfReviveHearts(Row->SelfReviveHearts);
        SpartaPlayerState->SetHearts(Row->StartHearts);
        SpartaPlayerState->SetCurrentState(EBomberPlayerState::Alive);
    }

    if (UBomberAttributeSet* AttrSet = GetMutableAttributeSet())
    {
        AttrSet->InitializeHealth(Row->StartHearts);
    }

    StunDuration = Row->StunDuration;
    InvincibleDuration = Row->InvincibleDuration;
}

UBomberAttributeSet* UCombatComponent::GetMutableAttributeSet() const
{
    if (!IsValid(CachedASC)) return nullptr;
    return const_cast<UBomberAttributeSet*>(CachedASC->GetSet<UBomberAttributeSet>());
}

// 핵심 피격 처리

void UCombatComponent::ApplyDamage()
{
    if (IsValid(SpartaPlayerState))
    {
        UE_LOG(LogTemp, Warning, TEXT("UCombatComponent::ApplyDamage 진입! 현재 Hearts: %d, 무적 여부: %s"),
            SpartaPlayerState->GetHearts(), bInvincible ? TEXT("True") : TEXT("False"));
    }

    if (!GetOwner()->HasAuthority()) return;

    if (bInvincible) return;
    if (bDamageThisFrame) return;

    if (bHasShield)
    {
        bHasShield = false;
        OnShieldBlock.Broadcast();
        OnRep_HasShield();

        // 추가 — GE_Shield도 같이 제거
        if (IsValid(CachedASC) && ActiveShieldEffectHandle.IsValid())
        {
            CachedASC->RemoveActiveGameplayEffect(ActiveShieldEffectHandle);
        }
        return;
    }

    if (IsValid(CachedASC) && CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Stunned)) return;
    if (IsValid(CachedASC) && CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Eliminated)) return;

    bDamageThisFrame = true;

    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &UCombatComponent::ResetDamageFlag));

    UBomberAttributeSet* AttrSet = GetMutableAttributeSet();
    if (IsValid(AttrSet))
    {
        CachedASC->ApplyModToAttribute(UBomberAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -1.f);
    }
    OnHit.Broadcast();

    if (IsValid(AttrSet) && AttrSet->GetHealth() <= 0.f)
    {
        EnterStun();
    }
}

bool UCombatComponent::CanTakeDamage() const
{
    return !IsValid(CachedASC) || !CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Eliminated);
}

void UCombatComponent::GrantShield()
{
    bHasShield = true;
    OnRep_HasShield();

    // 추가 — GE_Shield 적용 (Infinite 정책)
    if (IsValid(CachedASC) && IsValid(ShieldEffectClass))
    {
        FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
        FGameplayEffectSpecHandle Spec = CachedASC->MakeOutgoingSpec(ShieldEffectClass, 1.f, Context);

        if (Spec.IsValid())
        {
            ActiveShieldEffectHandle = CachedASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
        }
    }
}

// 상태 전이 함수들

void UCombatComponent::EnterStun()
{
    SpartaPlayerState->SetCurrentState(EBomberPlayerState::Stunned);
    OnStun.Broadcast();

    if (IsValid(CachedASC))
    {
        CachedASC->AddLooseGameplayTag(BomberGameplayTags::State_Stunned);
    }

    if (IsValid(CachedASC) && IsValid(StunEffectClass))
    {
        FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
        FGameplayEffectSpecHandle Spec = CachedASC->MakeOutgoingSpec(StunEffectClass, 1.f, Context);

        if (Spec.IsValid())
        {
            ActiveStunEffectHandle = CachedASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

            if (FOnActiveGameplayEffectRemoved_Info* RemovedDelegate =
                CachedASC->OnGameplayEffectRemoved_InfoDelegate(ActiveStunEffectHandle))
            {
                RemovedDelegate->AddUObject(this, &UCombatComponent::OnAnyGameplayEffectRemoved);
            }
        }
    }

    GetWorld()->GetTimerManager().SetTimer(
        StunTimerHandle, StunDuration, false);
}

void UCombatComponent::Eliminate()
{
    SpartaPlayerState->SetCurrentState(EBomberPlayerState::Eliminated);

    if (IsValid(CachedASC))
    {
        CachedASC->AddLooseGameplayTag(BomberGameplayTags::State_Eliminated);
    }

    OnEliminated.Broadcast();

    // GameMode에 CheckMatchEnd() 호출 연결 필요
    // 처치 보상 드롭 요청 (시스템3 ItemDropComponent::DropKillReward)
}

void UCombatComponent::Revive()
{
    if (UBomberAttributeSet* AttrSet = GetMutableAttributeSet())
    {
        CachedASC->ApplyModToAttribute(UBomberAttributeSet::GetHealthAttribute(), EGameplayModOp::Override, AttrSet->GetMaxHealth());
    }
    SpartaPlayerState->SetCurrentState(EBomberPlayerState::Alive);
    bInvincible = true;

    GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

    // 상태 변경 후 GE_Stun 제거
    if (IsValid(CachedASC) && ActiveStunEffectHandle.IsValid())
    {
        CachedASC->RemoveActiveGameplayEffect(ActiveStunEffectHandle);
    }

    if (IsValid(CachedASC))
    {
        CachedASC->RemoveLooseGameplayTag(BomberGameplayTags::State_Stunned);
    }

    OnRevived.Broadcast();

    GetWorld()->GetTimerManager().SetTimer(
        InvincibleTimerHandle, this, &UCombatComponent::EndInvincible,
        InvincibleDuration, false);
}

void UCombatComponent::SelfRevive()
{
    if (!GetOwner()->HasAuthority()) return;
    if (!IsValid(CachedASC) || !CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Stunned)) return;

    // 상태를 먼저 바꾼다
    CachedASC->ApplyModToAttribute(UBomberAttributeSet::GetHealthAttribute(), EGameplayModOp::Override, static_cast<float>(SpartaPlayerState->GetSelfReviveHearts()));
    SpartaPlayerState->SetCurrentState(EBomberPlayerState::Alive);

    GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

    // 상태 변경 후 GE_Stun 제거
    if (IsValid(CachedASC) && ActiveStunEffectHandle.IsValid())
    {
        CachedASC->RemoveActiveGameplayEffect(ActiveStunEffectHandle);
    }

    CachedASC->RemoveLooseGameplayTag(BomberGameplayTags::State_Stunned);

    OnSelfRevive.Broadcast();
}

void UCombatComponent::InstantEliminate()
{
    if (!GetOwner()->HasAuthority()) return;

    GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

    if (IsValid(CachedASC) && ActiveStunEffectHandle.IsValid())
    {
        CachedASC->RemoveActiveGameplayEffect(ActiveStunEffectHandle);
    }

    SpartaPlayerState->SetCurrentState(EBomberPlayerState::Eliminated);

    if (IsValid(CachedASC))
    {
        CachedASC->AddLooseGameplayTag(BomberGameplayTags::State_Eliminated);
    }

    OnEliminated.Broadcast();
}

void UCombatComponent::EndInvincible()
{
    bInvincible = false;
}

void UCombatComponent::ResetDamageFlag()
{
    bDamageThisFrame = false;
}

// 충돌 처리
void UCombatComponent::OnOverlapWithEnemy(AActor* Enemy)
{
    if (!GetOwner()->HasAuthority()) return;
    if (!IsValid(Enemy)) return;

    if (IsValid(CachedASC) && CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Stunned))
    {
        Eliminate();
    }
}

void UCombatComponent::OnOverlapWithAlly(AActor* Ally)
{
    if (!GetOwner()->HasAuthority()) return;
    if (!IsValid(Ally)) return;

    if (IsValid(CachedASC) && CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Stunned))
    {
        Revive();
    }
}

void UCombatComponent::OnRep_HasShield()
{
    if (OnbHasShieldChanged.IsBound())
    {
        OnbHasShieldChanged.Broadcast(bHasShield);
    }
}

// 추가 — 헤더에 선언만 있고 정의가 없어서 링크 에러 나던 함수
void UCombatComponent::OnAnyGameplayEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo)
{
    if (!IsValid(CachedASC) || !CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Stunned)) return;

    // 아직 Stunned인데 GE가 사라졌다 = 자연 만료였다는 뜻
    Eliminate();
}

void UCombatComponent::Heal(int32 Amount)
{
    if (IsValid(CachedASC) &&
        (CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Stunned) ||
         CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Eliminated)))
    {
        return;
    }

    if (IsValid(CachedASC))
    {
        // 상한(MaxHealth) 클램프는 PreAttributeChange에서 처리됨
        CachedASC->ApplyModToAttribute(UBomberAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, static_cast<float>(Amount));
    }
    OnHit.Broadcast();
}

float UCombatComponent::GetStunProgressPercent() const
{
    if (IsValid(CachedASC) && CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Stunned))
    {
        if (GetWorld() && StunDuration > 0.f)
        {
            float Remaining = GetWorld()->GetTimerManager().GetTimerRemaining(StunTimerHandle);
            return FMath::Clamp((StunDuration - Remaining) / StunDuration, 0.f, 1.f);
        }
    }
    return 0.f;
}

void UCombatComponent::BroadcastCurrentState()
{
    OnRep_HasShield();
}