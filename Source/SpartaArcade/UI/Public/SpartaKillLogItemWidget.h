#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpartaUIDefs.h"
#include "SpartaKillLogItemWidget.generated.h"

class UTextBlock;
class UImage;

UCLASS()
class SPARTAARCADE_API USpartaKillLogItemWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    UPROPERTY(meta = (BindWidget))
    UTextBlock* KillerTextBlock;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* VictimTextBlock;

    UPROPERTY(meta = (BindWidget))
    UImage* DeathReasonImage;

    // 아이콘 매핑용 텍스처 (에디터 설정용)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KillLog | Assets")
    UTexture2D* ExplosionIcon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KillLog | Assets")
    UTexture2D* SafeZoneIcon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KillLog | Assets")
    UTexture2D* ObstacleIcon;

public:
    UFUNCTION(BlueprintCallable, Category = "UI | KillLog")
    void SetupKillLog(const FString& KillerName, const FString& VictimName, EDeathReason Reason);
};
