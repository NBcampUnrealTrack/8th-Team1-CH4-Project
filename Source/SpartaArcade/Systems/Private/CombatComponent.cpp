#include "CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"

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

    if(IsValid(SpartaPlayerState))
    {
        SpartaPlayerState->SetStartHearts(Row->StartHearts);
        SpartaPlayerState->SetSelfReviveHearts(Row->SelfReviveHearts);
        SpartaPlayerState->SetHearts(Row->StartHearts);
        SpartaPlayerState->SetCurrentState(EBomberPlayerState::Alive);
	}

    StunDuration = Row->StunDuration;
    InvincibleDuration = Row->InvincibleDuration;
}

// 핵심 피격 처리 

void UCombatComponent::ApplyDamage()
{
    if (!GetOwner()->HasAuthority()) return;

    //무적 중 피격 무효
    if (bInvincible) return;

    //동시 피격 시 프레임당 1회만 차감
    if (bDamageThisFrame) return;

    //방어막 보유 중이면 하트 대신 방어막 소멸
    if (bHasShield)
    {
        bHasShield = false;
        OnShieldBlock.Broadcast();
        return;
    }

    // 기절 중 재피격은 무시 
    if (SpartaPlayerState->GetCurrentState() == EBomberPlayerState::Stunned) return;
    if (SpartaPlayerState->GetCurrentState() == EBomberPlayerState::Eliminated) return;

    bDamageThisFrame = true;

    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &UCombatComponent::ResetDamageFlag));

    SpartaPlayerState->SetHearts(SpartaPlayerState->GetHearts() - 1);
    OnHit.Broadcast();

    if (SpartaPlayerState->GetHearts() <= 0)
    {
        EnterStun();
    }
}

bool UCombatComponent::CanTakeDamage() const
{
    return SpartaPlayerState->GetCurrentState() != EBomberPlayerState::Eliminated;
}

void UCombatComponent::GrantShield()
{
    bHasShield = true;
}

//상태 전이 함수들 

void UCombatComponent::EnterStun()
{
    SpartaPlayerState->SetCurrentState(EBomberPlayerState::Stunned);
    OnStun.Broadcast();

    GetWorld()->GetTimerManager().SetTimer(
        StunTimerHandle, this, &UCombatComponent::Eliminate, StunDuration, false);
}

void UCombatComponent::Eliminate()
{
    GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

    SpartaPlayerState->SetCurrentState(EBomberPlayerState::Eliminated);
    OnEliminated.Broadcast();

    // GameMode에 CheckMatchEnd() 호출 연결 필요
    // 처치 보상 드롭 요청 (시스템3 ItemDropComponent::DropKillReward)
}

void UCombatComponent::Revive()
{
    GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

    SpartaPlayerState->SetHearts(SpartaPlayerState->GetStartHearts());
    SpartaPlayerState->SetCurrentState(EBomberPlayerState::Alive);
    bInvincible = true;

    OnRevived.Broadcast();

    GetWorld()->GetTimerManager().SetTimer(
        InvincibleTimerHandle, this, &UCombatComponent::EndInvincible,
        InvincibleDuration, false);
}

void UCombatComponent::SelfRevive()
{
    if (!GetOwner()->HasAuthority()) return;

    if (SpartaPlayerState->GetCurrentState() != EBomberPlayerState::Stunned) return;

    GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

    SpartaPlayerState->SetHearts(SpartaPlayerState->GetSelfReviveHearts());
    SpartaPlayerState->SetCurrentState(EBomberPlayerState::Alive);

    OnSelfRevive.Broadcast();
}

void UCombatComponent::InstantEliminate()
{
    //자기장 압사 - 기절 없이 즉시 탈락
    if (!GetOwner()->HasAuthority()) return;

    GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);

    SpartaPlayerState->SetCurrentState(EBomberPlayerState::Eliminated);
    OnEliminated.Broadcast();

    // GameMode에 CheckMatchEnd() 호출 연결 필요
}

void UCombatComponent::EndInvincible()
{
    bInvincible = false;
}

void UCombatComponent::ResetDamageFlag()
{
    bDamageThisFrame = false;
}

//충돌 처리 
void UCombatComponent::OnOverlapWithEnemy(AActor* Enemy)
{
    if (!GetOwner()->HasAuthority()) return;

    if (!IsValid(Enemy)) return;

    if (SpartaPlayerState->GetCurrentState() == EBomberPlayerState::Stunned)
    {
        Eliminate();
    }
}

void UCombatComponent::OnOverlapWithAlly(AActor* Ally)
{
    if (!GetOwner()->HasAuthority()) return;

    if (!IsValid(Ally)) return;

    if (SpartaPlayerState->GetCurrentState() == EBomberPlayerState::Stunned)
    {
        Revive();
    }
}

void UCombatComponent::OnRep_HasShield()
{
    // UI 갱신용 - 방어막 상태 변경 시 자동 호출됨
}

// 캐릭터 위임 연동을 위한 Heal 함수 구현 추가
void UCombatComponent::Heal(int32 Amount)
{
    if (SpartaPlayerState->GetCurrentState() != EBomberPlayerState::Alive) return;
    SpartaPlayerState->SetHearts(FMath::Clamp(SpartaPlayerState->GetHearts() + Amount, 0, SpartaPlayerState->GetStartHearts()));
    OnHit.Broadcast(); // UI 갱신을 위해 브로드캐스트
}