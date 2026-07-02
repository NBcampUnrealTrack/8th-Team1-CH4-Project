#include "SpartaKillLogItemWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void USpartaKillLogItemWidget::SetupKillLog(const FString& KillerName, const FString& VictimName, EDeathReason Reason)
{
    if (KillerTextBlock)
    {
        // 환경에 따라 킬러명이 빈 문자열이면 환경 장애물 등의 텍스트로 대체
        FString DisplayKiller = KillerName.IsEmpty() ? TEXT("장애물") : KillerName;
        KillerTextBlock->SetText(FText::FromString(DisplayKiller));
    }

    if (VictimTextBlock)
    {
        VictimTextBlock->SetText(FText::FromString(VictimName));
    }

    if (DeathReasonImage)
    {
        UTexture2D* TargetIcon = nullptr;
        switch (Reason)
        {
        case EDeathReason::Explosion:
            TargetIcon = ExplosionIcon;
            break;
        case EDeathReason::SafeZone:
            TargetIcon = SafeZoneIcon;
            break;
        case EDeathReason::Obstacle:
            TargetIcon = ObstacleIcon;
            break;
        default:
            TargetIcon = nullptr;
            break;
        }

        if (TargetIcon)
        {
            DeathReasonImage->SetBrushFromTexture(TargetIcon);
            DeathReasonImage->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            DeathReasonImage->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}
