#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpartaUIDefs.h"
#include "SpartaKillLogWidget.generated.h"

class UVerticalBox;
class USpartaKillLogItemWidget;

UCLASS()
class SPARTAARCADE_API USpartaKillLogWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    UPROPERTY(meta = (BindWidget))
    UVerticalBox* KillLogContainer;

    // 동적으로 생성할 킬 로그 개별 아이템 클래스 지정
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KillLog | Settings")
    TSubclassOf<USpartaKillLogItemWidget> KillLogItemClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KillLog | Settings")
    int32 MaxVisibleLogs = 5;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KillLog | Settings")
    float LogDisplayDuration = 5.0f;

public:
    // --- 게임 시스템/네트워크에서 호출하는 이벤트 리스너 ---
    UFUNCTION(BlueprintCallable, Category = "UI | KillLog")
    void AddKillLog(const FString& KillerName, const FString& VictimName, EDeathReason Reason);

private:
    // 시간 만료 후 로그를 지우는 콜백
    void RemoveOldestLog();
};
