#include "SpartaHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "SpartaArcadeStatBar.h"
#include "SpartaArcadeStatSlot.h"
#include "Components/HorizontalBox.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"
#include "SpartaKillLogWidget.h"
#include "Systems/Public/StatComponent.h"
#include "Systems/Public/CombatComponent.h"
#include "Systems/Public/BombPlacerComponent.h"
#include "Systems/Public/BomberAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Level/Public/SpartaArcadeZoneManager.h"
#include "Characters/Public/SpartaArcadeCharacter.h"
#include "GameFramework/GameStateBase.h"

void USpartaHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    //초기 하트 체력 설정 (3/3) 및 기절 패널 비활성화
    UpdateHearts(3, 3);
    SetStunActive(false);
    
    UpdateBombStats(1, 1);
    UpdateCharacterStats(1.0f, 1.0f, false);
    UpdateGameStateInfo(0, 0);

    // 틱 대신 1초 주기 타이머 실행
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(GameStateTimerHandle, this, &USpartaHUDWidget::UpdateGameStateTimer, 1.0f, true);
    }
}

void USpartaHUDWidget::NativeDestruct()
{
    Super::NativeDestruct();

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(GameStateTimerHandle);
    }
}

// 기절 게이지 실시간 갱신 (최적화: 기절 상태가 아닐 때는 바로 조기 리턴하여 틱 비용 극소화)
void USpartaHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    // 기절 UI 실시간 갱신 로직은 NativeTick 대신 성능 최적화를 위해 0.1초 타이머(UpdateStunProgressTimer) 방식으로 변경하여 처리하므로 틱 로직 제거
}

// 1초 주기로 경기 시간 및 생존자 수를 업데이트
void USpartaHUDWidget::UpdateGameStateTimer()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 1. 생존 플레이어 수 카운트
    int32 AliveCount = 0;
    if (AGameStateBase* GS = World->GetGameState())
    {
        for (APlayerState* PS : GS->PlayerArray)
        {
            if (ASpartaPlayerState* SPS = Cast<ASpartaPlayerState>(PS))
            {
                if (SPS->GetCurrentState() != EBomberPlayerState::Eliminated)
                {
                    AliveCount++;
                }
            }
        }
    }

    // 2. 자기장 매니저 탐색 및 남은 시간(초) 산출
    static TWeakObjectPtr<ASpartaArcadeZoneManager> CachedZoneManager = nullptr;
    if (!CachedZoneManager.IsValid())
    {
        CachedZoneManager = Cast<ASpartaArcadeZoneManager>(UGameplayStatics::GetActorOfClass(World, ASpartaArcadeZoneManager::StaticClass()));
    }

    int32 MatchSeconds = 0;
    if (CachedZoneManager.IsValid())
    {
        float Elapsed = CachedZoneManager->GetElapsed();
        float ActivationDelay = CachedZoneManager->ActivationDelay;
        float ShrinkDuration = CachedZoneManager->ShrinkDuration;

        if (Elapsed < ActivationDelay)
        {
            // 자기장 수축 전: 남은 작동 대기 시간 카운트다운
            MatchSeconds = FMath::Max(0, FMath::RoundToInt(ActivationDelay - Elapsed));
        }
        else
        {
            // 자기장 수축 중: 남은 수축 시간 카운트다운
            MatchSeconds = FMath::Max(0, FMath::RoundToInt((ActivationDelay + ShrinkDuration) - Elapsed));
        }
    }

    const bool bAliveChanged   = (AliveCount != CachedAliveCount);
    const bool bTimeChanged    = (MatchSeconds != CachedMatchSeconds);
    if (bAliveChanged || bTimeChanged)
    {
        CachedAliveCount   = AliveCount;
        CachedMatchSeconds = MatchSeconds;
        UpdateGameStateInfo(AliveCount, MatchSeconds);
    }
}

void USpartaHUDWidget::UpdateHearts(int32 CurrentHearts, int32 MaxHearts)
{
    if (CurrentHearts == CachedCurrentHearts && MaxHearts == CachedMaxHearts)
    {
        return;
    }
    CachedCurrentHearts = CurrentHearts;
    CachedMaxHearts     = MaxHearts;

    // [성능 최적화] 매번 ClearChildren() 및 CreateWidget() 동적 스폰 대신 기존 자식 위젯을 풀링/재활용하여 SetVisibility 제어
    if (HeartHorizontalBox && HeartUnitWidgetClass)
    {
        int32 ExistingCount = HeartHorizontalBox->GetChildrenCount();
        int32 TargetTotal = FMath::Max(CurrentHearts, MaxHearts);

        // 부족한 수만큼 위젯 추가 생성하여 슬롯에 등록
        for (int32 i = ExistingCount; i < TargetTotal; ++i)
        {
            UUserWidget* HeartWidget = CreateWidget<UUserWidget>(this, HeartUnitWidgetClass);
            if (HeartWidget)
            {
                HeartHorizontalBox->AddChildToHorizontalBox(HeartWidget);
            }
        }

        // 현재 체력 수량에 맞춰 가시성 활성/비활성화 토글
        int32 TotalChildCount = HeartHorizontalBox->GetChildrenCount();
        for (int32 i = 0; i < TotalChildCount; ++i)
        {
            if (UWidget* Child = HeartHorizontalBox->GetChildAt(i))
            {
                Child->SetVisibility(i < CurrentHearts ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
            }
        }
    }
}

void USpartaHUDWidget::SetStunActive(bool bIsActive)
{
    // 기절 오버레이 활성 상태 토글
    if (StunOverlayPanel)
    {
        StunOverlayPanel->SetVisibility(bIsActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    // 기절 상태 진입 시 이전 게이지 캐시 값을 초기화하여 UI 게이지가 첫 프레임부터 즉시 갱신되도록 처리
    CachedStunProgress = -1.f;

    // 기절 활성화 시 0.1초 주기로 게이지를 갱신하는 타이머 구동, 기절 종료 시 타이머 해제
    UWorld* World = GetWorld();
    if (World)
    {
        if (bIsActive)
        {
            World->GetTimerManager().SetTimer(
                StunUpdateTimerHandle,
                this,
                &USpartaHUDWidget::UpdateStunProgressTimer,
                0.1f,
                true
            );
        }
        else
        {
            World->GetTimerManager().ClearTimer(StunUpdateTimerHandle);
        }
    }
}

// 기절 게이지 0.1초 주기 타이머 콜백 구현
void USpartaHUDWidget::UpdateStunProgressTimer()
{
    if (CombatComponent)
    {
        const float Progress = CombatComponent->GetStunProgressPercent();
        if (!FMath::IsNearlyEqual(Progress, CachedStunProgress, 0.001f))
        {
            CachedStunProgress = Progress;
            UpdateStunProgress(Progress);
        }
    }
}

void USpartaHUDWidget::UpdateStunProgress(float Percent)
{
    //  탈출 게이지 충전률 갱신
    if (StunProgressBar)
    {
        StunProgressBar->SetPercent(Percent);
    }
}

void USpartaHUDWidget::UpdateBombStats(int32 CurrentBombs, int32 MaxBombs)
{
    if (CurrentBombCountText)
    {
        CurrentBombCountText->SetText(FText::AsNumber(CurrentBombs));
    }

    if (MaxBombCountText)
    {
        MaxBombCountText->SetText(FText::AsNumber(MaxBombs));
    }
}

void USpartaHUDWidget::UpdateCharacterStats(float ExplosionRange, float MoveSpeed, bool bHasShield)
{
    if (ShieldStatusText)
    {
        ShieldStatusText->SetText(FText::FromString(bHasShield ? TEXT("ACTIVE") : TEXT("NONE")));
    }
}

void USpartaHUDWidget::UpdateGameStateInfo(int32 AlivePlayers, int32 MatchSeconds)
{
    if (AlivePlayersText)
    {
        AlivePlayersText->SetText(FText::FromString(FString::Printf(TEXT("%d 명 생존"), AlivePlayers)));
    }

    if (MatchTimeText)
    {
        MatchTimeText->SetText(FText::FromString(FormatTime(MatchSeconds)));
    }
}

// OnHit 델리게이트 호출 시 대미지 위젯 화면 표시 로직 구현
void USpartaHUDWidget::HandleOnHit()
{
    if (DamageTextWidgetClass)
    {
        UUserWidget* DamageWidget = CreateWidget<UUserWidget>(GetWorld(), DamageTextWidgetClass);
        if (DamageWidget)
        {
            DamageWidget->AddToViewport();
        }
    }
}

void USpartaHUDWidget::HandleOnItemPickup(EItemType ItemType, int32 NewCount)
{
    // 아이템 습득 시 능력치 갱신 이벤트 대응 (로직 소유 안 함, 수치 적용은 캐릭터/스탯 시스템의 몫)
    // 여기서는 화면 갱신만 처리
    switch (ItemType)
    {
    case EItemType::BombCount:
        // 폭탄 개수 갱신은 StatSystem 이벤트를 통해 UpdateBombStats 로 반영되거나
        // 획득 연출 팝업 처리
        break;
    case EItemType::ExplosionRange:
    case EItemType::MoveSpeed:
    case EItemType::Shield:
        // 스탯 변동 위젯 갱신
        break;
    }
}

void USpartaHUDWidget::HandleOnShieldBlock()
{
    // 방어막 피격 연출 처리
    if (ShieldStatusText)
    {
        // 쉴드 깨짐 또는 방어 피드백 표시
        ShieldStatusText->SetText(FText::FromString(TEXT("BLOCKED!")));
    }
}

FString USpartaHUDWidget::FormatTime(int32 TotalSeconds) const
{
    int32 Minutes = TotalSeconds / 60;
    int32 Seconds = TotalSeconds % 60;
    return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}

// --------------------------------------------
// Bomb, Explosion, MoveSpeed, Shield 등 스탯 갱신 함수 구현
void USpartaHUDWidget::UpdateCurrentBombs(int32 CurrentBombs)
{
    if (CurrentBombCountText)
    {
        CurrentBombCountText->SetText(FText::AsNumber(CurrentBombs));
    }
}

void USpartaHUDWidget::UpdateStats(int32 BombCount, float ExplosionRange, float MoveSpeed)
{
    if (MaxBombCountText)
    {
        MaxBombCountText->SetText(FText::AsNumber(BombCount));
    }
    if (RangeBar)
    {
        RangeBar->UpdateStatBar(FMath::RoundToInt(ExplosionRange));
    }
    if (SpeedBar)
    {
        SpeedBar->UpdateStatBar(FMath::RoundToInt(MoveSpeed));
    }
}

void USpartaHUDWidget::UpdateHasShield(bool bHasShield)
{

}

void USpartaHUDWidget::InitializeHUD(ASpartaPlayerState* PlayerState, UBomberAttributeSet* InAttributeSet, UCombatComponent* CombatComp)
{
    if(bIsInitialized)
    {
        return;
	}
    bIsInitialized = true;

    // 멤버 컴포넌트 캐싱 추가
    SpartaPlayerState = PlayerState;
    BomberAttributeSet = InAttributeSet;
    CombatComponent = CombatComp;

    // 스탯 바 위젯 초기화 (최대치 개수로 칸 동적 생성)
    if (StatSlotWidgetClass)
    {
        if (RangeBar) RangeBar->InitializeBar(5, StatSlotWidgetClass);       // 최대 5칸
        if (SpeedBar) SpeedBar->InitializeBar(5, StatSlotWidgetClass);       // 최대 5칸
    }

	BindToTarget(SpartaPlayerState, BomberAttributeSet, CombatComponent);
}

void USpartaHUDWidget::DeinitializeHUD()
{
    if (SpartaPlayerState)
    {
        SpartaPlayerState->OnHeartsChanged.RemoveDynamic(this, &USpartaHUDWidget::UpdateHearts);
        SpartaPlayerState->OnStunStateChanged.RemoveDynamic(this, &USpartaHUDWidget::SetStunActive);
        SpartaPlayerState->OnFirstAidKitsChanged.RemoveDynamic(this, &USpartaHUDWidget::UpdateMedKitStatus);
        SpartaPlayerState->OnShieldsChanged.RemoveDynamic(this, &USpartaHUDWidget::UpdateShieldItemStatus);
        SpartaPlayerState->OnKickUnlockedChanged.RemoveDynamic(this, &USpartaHUDWidget::HandleOnKickUnlockedChanged);
    }
    if (BomberAttributeSet)
    {
        BomberAttributeSet->OnStatsChanged.RemoveDynamic(this, &USpartaHUDWidget::UpdateStats);

        if (CurrentPlacedBombsHandle.IsValid())
        {
            if (UAbilitySystemComponent* ASC = BomberAttributeSet->GetOwningAbilitySystemComponent())
            {
                ASC->GetGameplayAttributeValueChangeDelegate(UBomberAttributeSet::GetCurrentPlacedBombsAttribute())
                    .Remove(CurrentPlacedBombsHandle);
            }
            CurrentPlacedBombsHandle.Reset();
        }
    }
    if (CombatComponent)
    {
        CombatComponent->OnShieldBlock.RemoveDynamic(this, &USpartaHUDWidget::HandleOnShieldBlock);
        CombatComponent->OnHit.RemoveDynamic(this, &USpartaHUDWidget::HandleOnHit);
        CombatComponent->OnbHasShieldChanged.RemoveDynamic(this, &USpartaHUDWidget::UpdateShieldStatus);
        CombatComponent->OnEliminated.RemoveDynamic(this, &USpartaHUDWidget::HandleOnEliminated);
    }
    
	SpartaPlayerState = nullptr;
	BomberAttributeSet = nullptr;
	CombatComponent = nullptr;
}

void USpartaHUDWidget::BindToTarget(ASpartaPlayerState* PlayerState, UBomberAttributeSet* InAttributeSet, UCombatComponent* CombatComp)
{
	DeinitializeHUD();
    
    SpartaPlayerState = PlayerState;
    BomberAttributeSet = InAttributeSet;
    CombatComponent = CombatComp;

    if (IsValid(SpartaPlayerState))
    {
        SpartaPlayerState->OnHeartsChanged.AddDynamic(this, &USpartaHUDWidget::UpdateHearts);
        SpartaPlayerState->OnStunStateChanged.AddDynamic(this, &USpartaHUDWidget::SetStunActive);
        SpartaPlayerState->OnFirstAidKitsChanged.AddDynamic(this, &USpartaHUDWidget::UpdateMedKitStatus);
        SpartaPlayerState->OnShieldsChanged.AddDynamic(this, &USpartaHUDWidget::UpdateShieldItemStatus);
        SpartaPlayerState->OnKickUnlockedChanged.AddDynamic(this, &USpartaHUDWidget::HandleOnKickUnlockedChanged);
        // 초기 가시성 상태 세팅
		UpdateHearts(SpartaPlayerState->GetHearts(), SpartaPlayerState->GetStartHearts());
        UpdateMedKitStatus(SpartaPlayerState->GetFirstAidKits());
        UpdateShieldItemStatus(SpartaPlayerState->GetShields());
		UpdateKickStatus(SpartaPlayerState->IsKickUnlocked());
    }

    if(IsValid(BomberAttributeSet))
    {
        BomberAttributeSet->OnStatsChanged.AddDynamic(this, &USpartaHUDWidget::UpdateStats);
        // 시작 시점의 초기 스탯치 값으로 바 가시성 세팅 강제 트리거
        UpdateStats(
            FMath::RoundToInt(BomberAttributeSet->GetBombCount()),
            BomberAttributeSet->GetBombRange(),
            BomberAttributeSet->GetMoveSpeed()
        );
        // CurrentPlacedBombs가 변경될 때 HUD 폭탄 수량 실시간 갱신 바인딩 추가
        if (UAbilitySystemComponent* ASC = BomberAttributeSet->GetOwningAbilitySystemComponent())
        {
            CurrentPlacedBombsHandle = ASC->GetGameplayAttributeValueChangeDelegate(UBomberAttributeSet::GetCurrentPlacedBombsAttribute())
                .AddLambda([this](const FOnAttributeChangeData& Data)
                {
                    UpdateCurrentBombs(FMath::RoundToInt(Data.NewValue));
                });
        }
        UpdateCurrentBombs(FMath::RoundToInt(BomberAttributeSet->GetCurrentPlacedBombs()));
	}

    if (IsValid(CombatComponent))
    {
        CombatComponent->OnShieldBlock.AddDynamic(this, &USpartaHUDWidget::HandleOnShieldBlock);
        CombatComponent->OnHit.AddDynamic(this, &USpartaHUDWidget::HandleOnHit);
        CombatComponent->OnbHasShieldChanged.AddDynamic(this, &USpartaHUDWidget::UpdateShieldStatus);
        CombatComponent->OnEliminated.AddDynamic(this, &USpartaHUDWidget::HandleOnEliminated);

        UpdateShieldStatus(CombatComp->IsShielded());
    }
}

// 쉴드 활성/비활성(실제 방어 중인지) 상태는 텍스트로만 표시
void USpartaHUDWidget::UpdateShieldStatus(bool bHasShield)
{
	if (ShieldStatusText)
	{
		ShieldStatusText->SetText(FText::FromString(bHasShield ? TEXT("ACTIVE") : TEXT("NONE")));
	}
}

// 쉴드 아이템 보유 여부(획득했는지)는 아이콘으로 표시 — 구급상자(MedImg)와 동일한 패턴
void USpartaHUDWidget::UpdateShieldItemStatus(int32 ShieldCount)
{
	if (ShieldImg)
	{
		ShieldImg->SetVisibility(ShieldCount > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void USpartaHUDWidget::UpdateMedKitStatus(int32 MedKitCount)
{
	if (MedImg)
	{
		MedImg->SetVisibility(MedKitCount > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

// 발차기 활성화 여부에 따른 아이콘 가시성 제어 구현
void USpartaHUDWidget::UpdateKickStatus(bool bCanKick)
{
	if (KickImg)
	{
		KickImg->SetVisibility(bCanKick ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

// 발차기 델리게이트 이벤트 수신 시 처리
void USpartaHUDWidget::HandleOnKickUnlockedChanged(bool bIsUnlocked)
{
	UpdateKickStatus(bIsUnlocked);
}

void USpartaHUDWidget::HandleOnEliminated()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // PlayerArray를 즉시 순회하여 생존자 수 재산출
    int32 AliveCount = 0;
    if (AGameStateBase* GS = World->GetGameState())
    {
        for (APlayerState* PS : GS->PlayerArray)
        {
            if (ASpartaPlayerState* SPS = Cast<ASpartaPlayerState>(PS))
            {
                if (SPS->GetCurrentState() != EBomberPlayerState::Eliminated)
                {
                    AliveCount++;
                }
            }
        }
    }

    // 캐시 갱신 후 HUD 즉시 반영
    if (AliveCount != CachedAliveCount)
    {
        CachedAliveCount = AliveCount;
        UpdateGameStateInfo(AliveCount, CachedMatchSeconds >= 0 ? CachedMatchSeconds : 0);
    }
}

void USpartaHUDWidget::AddKillLog(const FString& KillerName, const FString& VictimName, EDeathReason Reason)
{
    if (IsValid(WBP_KillLogWidget))
    {
        WBP_KillLogWidget->AddKillLog(KillerName, VictimName, Reason);
    }
}