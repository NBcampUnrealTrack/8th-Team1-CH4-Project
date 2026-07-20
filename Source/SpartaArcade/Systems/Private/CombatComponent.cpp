#include "CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"
#include "Framework/Public/InGame/SpartaGameState.h"
#include "UI/Public/SpartaUIDefs.h"
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

    FCombatStatRow FallbackRow;
    FallbackRow.StartHearts = 3;
    FallbackRow.StunDuration = 3.f;
    FallbackRow.InvincibleDuration = 1.f;
    FallbackRow.SelfReviveHearts = 1;

    FCombatStatRow* Row = &FallbackRow;

    if (IsValid(CombatStatTable))
    {
        TArray<FCombatStatRow*> Rows;
        CombatStatTable->GetAllRows<FCombatStatRow>(TEXT("InitializeFromDataTable"), Rows);
        if (Rows.Num() > 0 && Rows[0] != nullptr)
        {
            Row = Rows[0];
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatComponent: CombatStatTable에 Row가 없어 하드코딩된 기본값 사용!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatComponent: CombatStatTable이 유효하지 않아 하드코딩된 기본값 사용!"));
    }

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
    if (!IsValid(SpartaPlayerState)) return;
    if (!IsValid(CachedASC)) return;

    if (!GetOwner()->HasAuthority()) return;

    if (bDamageThisFrame) return;
    if (CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Stunned)) return;
    if (CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Eliminated)) return;

    // GAS 복제 태그를 기반으로 무적 여부 판별
    const bool bIsInvincible = CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Invincible);
    UE_LOG(LogTemp, Warning, TEXT("UCombatComponent::ApplyDamage 진입! 현재 Hearts: %d, 무적 여부: %s"),
        SpartaPlayerState->GetHearts(), bIsInvincible ? TEXT("True") : TEXT("False"));

    if (bIsInvincible) return;

    bDamageThisFrame = true;
    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &UCombatComponent::ResetDamageFlag));

    UBomberAttributeSet* AttrSet = GetMutableAttributeSet();
    if (IsValid(AttrSet))
    {
        CachedASC->ApplyModToAttribute(UBomberAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -1.f);
    }
    OnHit.Broadcast();

    // 기존 피격 무적 GE가 있다면 먼저 제거
    if (ActiveInvincibleEffectHandle.IsValid())
    {
        CachedASC->RemoveActiveGameplayEffect(ActiveInvincibleEffectHandle);
        ActiveInvincibleEffectHandle.Invalidate();
    }

    // 피격 후 1초 무적 — InvincibleEffectClass GE 적용 및 해제 타이머
    if (IsValid(InvincibleEffectClass))
    {
        FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
        FGameplayEffectSpecHandle Spec = CachedASC->MakeOutgoingSpec(InvincibleEffectClass, 1.f, Context);
        if (Spec.IsValid())
        {
            Spec.Data->SetDuration(InvincibleDuration, false);
            ActiveInvincibleEffectHandle = CachedASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
        }
    }

    GetWorld()->GetTimerManager().SetTimer(
        InvincibleTimerHandle,
        this,
        &UCombatComponent::EndInvincible,
        InvincibleDuration,
        false
    );

    if (IsValid(AttrSet) && AttrSet->GetHealth() <= 0.f)
    {
        EnterStun();
    }
}

bool UCombatComponent::CanTakeDamage() const
{
    // GAS 태그로 무적·쉴드 상태도 함께 판별
    if (IsValid(CachedASC) && CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Invincible)) return false;
    if (IsValid(CachedASC) && CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Shielded)) return false;
    if (!IsValid(SpartaPlayerState)) return true;
    if (SpartaPlayerState->GetCurrentState() == EBomberPlayerState::Eliminated) return false;
    return !IsValid(CachedASC) || !CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Eliminated);
}

void UCombatComponent::GrantShield()
{
    if (!IsValid(CachedASC)) return;

    // 기존 활성화된 쉴드가 있다면 먼저 제거
    if (ActiveShieldEffectHandle.IsValid())
    {
        CachedASC->RemoveActiveGameplayEffect(ActiveShieldEffectHandle);
        ActiveShieldEffectHandle.Invalidate();
    }

    // GE_Shield를 적용하여 복제 가능한 무적 및 쉴드 태그 부여
    if (IsValid(ShieldEffectClass))
    {
        FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
        FGameplayEffectSpecHandle Spec = CachedASC->MakeOutgoingSpec(ShieldEffectClass, 1.f, Context);
        if (Spec.IsValid())
        {
            Spec.Data->SetDuration(3.f, false);
            ActiveShieldEffectHandle = CachedASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
        }
    }

    // 3초 후 안전하게 쉴드 이펙트를 해제하기 위해 타이머 설정
    GetWorld()->GetTimerManager().SetTimer(
        ShieldTagClearTimerHandle,
        this,
        &UCombatComponent::ClearShieldTags,
        3.f,
        false
    );

    // UI 동기화
    OnbHasShieldChanged.Broadcast(true);
}

// 상태 전이 함수들

void UCombatComponent::EnterStun()
{
    if (!IsValid(SpartaPlayerState)) return;

    bool bHasAliveTeammate = false;
    UWorld* World = GetWorld();
    if (World)
    {
        ASpartaGameState* SpartaGS = World->GetGameState<ASpartaGameState>();
        if (SpartaGS && SpartaGS->GetGameModeType() != EGameModeType::Solo)
        {
            int32 MyTeamID = SpartaPlayerState->GetTeamID();
            for (APlayerState* PS : SpartaGS->PlayerArray)
            {
                ASpartaPlayerState* OtherPS = Cast<ASpartaPlayerState>(PS);
                if (OtherPS && OtherPS != SpartaPlayerState)
                {
                    if (OtherPS->GetTeamID() == MyTeamID && OtherPS->GetCurrentState() == EBomberPlayerState::Alive)
                    {
                        bHasAliveTeammate = true;
                        break;
                    }
                }
            }
        }
    }

    if (bHasAliveTeammate)
    {
        StunDuration = 10.f; // 아군이 생존해 있는 경우 기절(그로기) 대기 시간을 10초로 연장
    }
    else
    {
        Eliminate();
        return;
    }

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
            Spec.Data->SetDuration(StunDuration, false);
            ActiveStunEffectHandle = CachedASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

            if (FOnActiveGameplayEffectRemoved_Info* RemovedDelegate =
                CachedASC->OnGameplayEffectRemoved_InfoDelegate(ActiveStunEffectHandle))
            {
                RemovedDelegate->AddUObject(this, &UCombatComponent::OnAnyGameplayEffectRemoved);
            }
        }
    }

    // [버그 수정] StunTimerHandle 만료 시 Eliminate()가 호출되도록 콜백 함수 바인딩 추가
    GetWorld()->GetTimerManager().SetTimer(
        StunTimerHandle,
        this,
        &UCombatComponent::Eliminate,
        StunDuration,
        false);
}

void UCombatComponent::SetLastAttacker(ASpartaPlayerState* Attacker, EDeathReason Reason)
{
    LastAttackerPlayerState = Attacker;
    LastDeathReason = Reason;
}

void UCombatComponent::Eliminate()
{
    if (!IsValid(SpartaPlayerState)) return;
    GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

    SpartaPlayerState->SetCurrentState(EBomberPlayerState::Eliminated);

    if (IsValid(CachedASC))
    {
        CachedASC->AddLooseGameplayTag(BomberGameplayTags::State_Eliminated);
    }

    // GameMode에 Eliminate 이벤트 전달 (가해자 및 사유 포함)
    if (GetOwner()->HasAuthority() && IsValid(SpartaPlayerState))
    {
        OnEliminatedEvent.Broadcast(SpartaPlayerState, LastAttackerPlayerState, LastDeathReason);
    }

    OnEliminated.Broadcast();
}

void UCombatComponent::Revive()
{
    if (!IsValid(SpartaPlayerState)) return;
    if (!IsValid(CachedASC)) return;
    GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

    SpartaPlayerState->SetHearts(SpartaPlayerState->GetStartHearts());
    SpartaPlayerState->SetCurrentState(EBomberPlayerState::Alive);

    if (UBomberAttributeSet* AttrSet = GetMutableAttributeSet())
    {
        CachedASC->ApplyModToAttribute(UBomberAttributeSet::GetHealthAttribute(), EGameplayModOp::Override, AttrSet->GetMaxHealth());
    }

    CachedASC->RemoveLooseGameplayTag(BomberGameplayTags::State_Stunned);

    // 상태 변경 후 GE_Stun 제거
    if (ActiveStunEffectHandle.IsValid())
    {
        CachedASC->RemoveActiveGameplayEffect(ActiveStunEffectHandle);
    }

    OnRevived.Broadcast();

    // 기존 피격 무적 GE 제거
    if (ActiveInvincibleEffectHandle.IsValid())
    {
        CachedASC->RemoveActiveGameplayEffect(ActiveInvincibleEffectHandle);
        ActiveInvincibleEffectHandle.Invalidate();
    }

    // 부활 후 1초 무적 — InvincibleEffectClass GE 적용 및 C++ 타이머 작동
    if (IsValid(InvincibleEffectClass))
    {
        FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
        FGameplayEffectSpecHandle Spec = CachedASC->MakeOutgoingSpec(InvincibleEffectClass, 1.f, Context);
        if (Spec.IsValid())
        {
            Spec.Data->SetDuration(InvincibleDuration, false);
            ActiveInvincibleEffectHandle = CachedASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
        }
    }

    GetWorld()->GetTimerManager().SetTimer(
        InvincibleTimerHandle,
        this,
        &UCombatComponent::EndInvincible,
        InvincibleDuration,
        false
    );
}

void UCombatComponent::SelfRevive()
{
    if (!GetOwner()->HasAuthority()) return;
    if (!IsValid(SpartaPlayerState)) return;
    if (!IsValid(CachedASC) || !CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Stunned)) return;

    // 상태를 먼저 바꾼다
    CachedASC->ApplyModToAttribute(UBomberAttributeSet::GetHealthAttribute(), EGameplayModOp::Override, static_cast<float>(SpartaPlayerState->GetSelfReviveHearts()));
    SpartaPlayerState->SetCurrentState(EBomberPlayerState::Alive);

    GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

    CachedASC->RemoveLooseGameplayTag(BomberGameplayTags::State_Stunned);

    // 상태 변경 후 GE_Stun 제거
    if (IsValid(CachedASC) && ActiveStunEffectHandle.IsValid())
    {
        CachedASC->RemoveActiveGameplayEffect(ActiveStunEffectHandle);
    }


    OnSelfRevive.Broadcast();
}

void UCombatComponent::InstantEliminate()
{
    if (!GetOwner()->HasAuthority()) return;
    if (!IsValid(SpartaPlayerState)) return;

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

    // 즉사 판정(자기장 압사 등) 시에도 GameMode가 탈락 및 매치 종료 처리를 연동할 수 있도록 이벤트 브로드캐스트 추가
    if (IsValid(SpartaPlayerState))
    {
        OnEliminatedEvent.Broadcast(SpartaPlayerState, LastAttackerPlayerState, LastDeathReason);
    }

    OnEliminated.Broadcast();
}

void UCombatComponent::EndInvincible()
{
    if (!IsValid(CachedASC)) return;

    // 피격 1초 무적 GE 명시적 제거 -> 무적 태그(State.Invincible) 동반 해제
    if (ActiveInvincibleEffectHandle.IsValid())
    {
        CachedASC->RemoveActiveGameplayEffect(ActiveInvincibleEffectHandle);
        ActiveInvincibleEffectHandle.Invalidate();
    }
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
    const bool bNowShielded = IsValid(CachedASC) && CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Shielded);
    if (OnbHasShieldChanged.IsBound())
    {
        OnbHasShieldChanged.Broadcast(bNowShielded);
    }
}



void UCombatComponent::ClearShieldTags()
{
    if (!IsValid(CachedASC)) return;
    
    if (ActiveShieldEffectHandle.IsValid())
    {
        CachedASC->RemoveActiveGameplayEffect(ActiveShieldEffectHandle);
        ActiveShieldEffectHandle.Invalidate();
    }
    
    OnbHasShieldChanged.Broadcast(false);
}

// 추가 — 헤더에 선언만 있고 정의가 없어서 링크 에러 나던 함수
void UCombatComponent::OnAnyGameplayEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo)
{
    // 이미 부활(SelfRevive/Revive 등)하여 살아난 상태라면, 이펙트 제거로 인한 사망 처리를 무시합니다.
    if (IsValid(SpartaPlayerState) && SpartaPlayerState->GetCurrentState() == EBomberPlayerState::Alive)
    {
        return;
    }

    if (!IsValid(CachedASC) || !CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Stunned)) return;

    // 아직 Stunned인데 GE가 사라졌다 = 자연 만료였다는 뜻
    Eliminate();
}

void UCombatComponent::Heal(int32 Amount)
{
    if (!IsValid(SpartaPlayerState)) return;
    if (SpartaPlayerState->GetCurrentState() != EBomberPlayerState::Alive) return;

    if (IsValid(CachedASC) &&
        (CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Stunned) ||
         CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Eliminated)))
    {
        return;
    }

    SpartaPlayerState->SetHearts(FMath::Clamp(SpartaPlayerState->GetHearts() + Amount, 0, SpartaPlayerState->GetStartHearts()));

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
            // 1. 서버 측 타이머 핸들 남은 시간 우선 확인
            float Remaining = GetWorld()->GetTimerManager().GetTimerRemaining(StunTimerHandle);
            if (Remaining > 0.f)
            {
                return FMath::Clamp((StunDuration - Remaining) / StunDuration, 0.f, 1.f);
            }

            if (ActiveStunEffectHandle.IsValid())
            {
                if (const FActiveGameplayEffect* ActiveGE = CachedASC->GetActiveGameplayEffect(ActiveStunEffectHandle))
                {
                    float TimeRemaining = ActiveGE->GetTimeRemaining(GetWorld()->GetTimeSeconds());
                    float TotalDuration = ActiveGE->GetDuration();
                    if (TotalDuration > 0.f && TimeRemaining >= 0.f)
                    {
                        return FMath::Clamp((TotalDuration - TimeRemaining) / TotalDuration, 0.f, 1.f);
                    }
                }
            }
        }
    }
    return 0.f;
}

void UCombatComponent::BroadcastCurrentState()
{
    OnRep_HasShield();
}

EBomberPlayerState UCombatComponent::GetPlayerState() const
{
    return SpartaPlayerState ? SpartaPlayerState->GetCurrentState() : CurrentState;
}

bool UCombatComponent::IsShielded() const
{
    return IsValid(CachedASC) && CachedASC->HasMatchingGameplayTag(BomberGameplayTags::State_Shielded);
}
