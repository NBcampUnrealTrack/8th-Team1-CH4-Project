#include "SpartaHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"
#include "Systems/Public/StatComponent.h"
#include "Systems/Public/CombatComponent.h"
#include "Systems/Public/BombPlacerComponent.h"

void USpartaHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    //초기 하트 체력 설정 (3/3) 및 기절 패널 비활성화
    UpdateHearts(3, 3);
    SetStunActive(false);
    
    UpdateBombStats(1, 1);
    UpdateCharacterStats(1.0f, 1.0f, false);
    UpdateGameStateInfo(0, 0, 0);
}

void USpartaHUDWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

// 기절 게이지 실시간 갱신을 위해 NativeTick 정의 추가
void USpartaHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (CombatComponent && SpartaPlayerState && SpartaPlayerState->GetCurrentState() == EBomberPlayerState::Stunned)
    {
        float Progress = CombatComponent->GetStunProgressPercent();
        UpdateStunProgress(Progress);
    }
}

void USpartaHUDWidget::UpdateHearts(int32 CurrentHearts, int32 MaxHearts)
{
    // 하트 개수만큼 UI 슬롯에 하트 유닛 스폰 및 리스트업
    if (HeartHorizontalBox && HeartUnitWidgetClass)
    {
        HeartHorizontalBox->ClearChildren();
        for (int32 i = 0; i < CurrentHearts; ++i)
        {
            UUserWidget* HeartWidget = CreateWidget<UUserWidget>(this, HeartUnitWidgetClass);
            if (HeartWidget)
            {
                HeartHorizontalBox->AddChildToHorizontalBox(HeartWidget);
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
    if (ExplosionRangeText)
    {
        ExplosionRangeText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), ExplosionRange)));
    }

    if (MoveSpeedText)
    {
        MoveSpeedText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), MoveSpeed)));
    }

    if (ShieldStatusText)
    {
        ShieldStatusText->SetText(FText::FromString(bHasShield ? TEXT("ACTIVE") : TEXT("NONE")));
    }
}

void USpartaHUDWidget::UpdateGameStateInfo(int32 AlivePlayers, int32 MatchSeconds, int32 ZonePhase)
{
    if (AlivePlayersText)
    {
        AlivePlayersText->SetText(FText::FromString(FString::Printf(TEXT("%d 명 생존"), AlivePlayers)));
    }

    if (MatchTimeText)
    {
        MatchTimeText->SetText(FText::FromString(FormatTime(MatchSeconds)));
    }

    if (ZonePhaseText)
    {
        ZonePhaseText->SetText(FText::FromString(FString::Printf(TEXT("PHASE %d"), ZonePhase)));
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

    if (ExplosionRangeText)
    {
        ExplosionRangeText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), ExplosionRange)));
    }

    if (MoveSpeedText)
    {
        MoveSpeedText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), MoveSpeed)));
    }
}

void USpartaHUDWidget::UpdateHasShield(bool bHasShield)
{
    if (ShieldStatusText)
    {
        ShieldStatusText->SetText(FText::FromString(bHasShield ? TEXT("ACTIVE") : TEXT("NONE")));
    }
}

void USpartaHUDWidget::InitializeHUD(ASpartaPlayerState* PlayerState, UStatComponent* StatComp, UCombatComponent* CombatComp, UBombPlacerComponent* BombPlacerComp)
{
    // 멤버 컴포넌트 캐싱 추가
    SpartaPlayerState = PlayerState;
    StatComponent = StatComp;
    CombatComponent = CombatComp;
    BombPlacerComponent = BombPlacerComp;

    if (PlayerState)
    {
		PlayerState->OnHeartsChanged.AddDynamic(this, &USpartaHUDWidget::UpdateHearts);
		PlayerState->OnStunStateChanged.AddDynamic(this, &USpartaHUDWidget::SetStunActive);
    }

    if(StatComp)
    {
        StatComp->OnStatsChanged.AddDynamic(this, &USpartaHUDWidget::UpdateStats);
	}

    if(CombatComp)
    {
        CombatComp->OnShieldBlock.AddDynamic(this, &USpartaHUDWidget::HandleOnShieldBlock);
        // OnHit 델리게이트 바인딩 추가
        CombatComp->OnHit.AddDynamic(this, &USpartaHUDWidget::HandleOnHit);
	}

    if (BombPlacerComp)
    {
        BombPlacerComp->OnCurrentPlacedBombsChanged.AddDynamic(this, &USpartaHUDWidget::UpdateCurrentBombs);
    }
}