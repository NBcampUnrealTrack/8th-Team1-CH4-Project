#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpartaUIDefs.h"
#include "SpartaHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;

UCLASS()
class SPARTAARCADE_API USpartaHUDWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(meta = (BindWidget))
    UProgressBar* HPProgressBar;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* HPText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* CurrentBombCountText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* MaxBombCountText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ExplosionRangeText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* MoveSpeedText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ShieldStatusText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* AlivePlayersText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* MatchTimeText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ZonePhaseText;

    // --- 피격 피드백 UI 에셋 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI | Feedback")
    TSubclassOf<UUserWidget> DamageTextWidgetClass;

public:
    UFUNCTION(BlueprintCallable, Category = "UI | Update")
    void UpdateHP(float CurrentHP, float MaxHP);

    UFUNCTION(BlueprintCallable, Category = "UI | Update")
    void UpdateBombStats(int32 CurrentBombs, int32 MaxBombs);

    UFUNCTION(BlueprintCallable, Category = "UI | Update")
    void UpdateCharacterStats(float ExplosionRange, float MoveSpeed, bool bHasShield);

    UFUNCTION(BlueprintCallable, Category = "UI | Update")
    void UpdateGameStateInfo(int32 AlivePlayers, int32 MatchSeconds, int32 ZonePhase);

    UFUNCTION()
    void HandleOnHit(float Damage, const FVector& HitLocation);

    UFUNCTION()
    void HandleOnItemPickup(EItemType ItemType, int32 NewCount);

    UFUNCTION()
    void HandleOnShieldBlock();

private:
    // 내부 헬퍼 함수
    FString FormatTime(int32 TotalSeconds) const;
};
