#include "SpartaHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

void USpartaHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 초기값 설정
    UpdateHP(100.0f, 100.0f);
    UpdateBombStats(1, 1);
    UpdateCharacterStats(1.0f, 1.0f, false);
    UpdateGameStateInfo(0, 0, 0);
}

void USpartaHUDWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

void USpartaHUDWidget::UpdateHP(float CurrentHP, float MaxHP)
{
    if (HPProgressBar)
    {
        float Percent = (MaxHP > 0.0f) ? (CurrentHP / MaxHP) : 0.0f;
        HPProgressBar->SetPercent(Percent);
    }

    if (HPText)
    {
        HPText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(CurrentHP), FMath::RoundToInt(MaxHP))));
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
        AlivePlayersText->SetText(FText::FromString(FString::Printf(TEXT("%d SURVIVORS"), AlivePlayers)));
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

// 대미지 표시 시 주석 비활성화
void USpartaHUDWidget::HandleOnHit(float Damage, const FVector& HitLocation)
{
    // 대미지 표시 시 주석 비활성화
    
    // // 피격 시 데미지 숫자 화면 표시 UI (피드백)
    // if (DamageTextWidgetClass)
    // {
    //     UUserWidget* DamageWidget = CreateWidget<UUserWidget>(GetWorld(), DamageTextWidgetClass);
    //     if (DamageWidget)
    //     {
    //         DamageWidget->AddToViewport();
    //         
    //         // 데미지 위젯 내부에 SetDamage 등의 함수가 있다면 연동
    //     }
    // }
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
