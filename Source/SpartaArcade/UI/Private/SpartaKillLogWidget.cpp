#include "SpartaKillLogWidget.h"
#include "SpartaKillLogItemWidget.h"
#include "Components/VerticalBox.h"
#include "TimerManager.h"

void USpartaKillLogWidget::AddKillLog(const FString& KillerName, const FString& VictimName, EDeathReason Reason)
{
    if (!KillLogContainer || !KillLogItemClass)
    {
        return;
    }

    // 1. 개별 로그 아이템 생성 및 데이터 세팅
    USpartaKillLogItemWidget* LogItem = CreateWidget<USpartaKillLogItemWidget>(GetWorld(), KillLogItemClass);
    if (LogItem)
    {
        LogItem->SetupKillLog(KillerName, VictimName, Reason);
        
        // 2. 컨테이너에 추가
        KillLogContainer->AddChildToVerticalBox(LogItem);

        // 3. 최대 노출 개수 초과 시 가장 오래된 것 즉시 제거
        if (KillLogContainer->GetChildrenCount() > MaxVisibleLogs)
        {
            KillLogContainer->RemoveChildAt(0);
        }

        // 4. 일정 시간 후 페이드/소멸을 위한 타이머 설정
        FTimerHandle LogExpiryTimer;
        GetWorld()->GetTimerManager().SetTimer(
            LogExpiryTimer,
            this,
            &USpartaKillLogWidget::RemoveOldestLog,
            LogDisplayDuration,
            false
        );
    }
}

void USpartaKillLogWidget::RemoveOldestLog()
{
    if (KillLogContainer && KillLogContainer->GetChildrenCount() > 0)
    {
        // 가장 먼저 추가된(오래된) 자식 노드 제거
        KillLogContainer->RemoveChildAt(0);
    }
}
