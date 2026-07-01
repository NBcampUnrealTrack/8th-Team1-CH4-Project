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
    
    //하트 개수 기반 체력 표시
    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* HeartHorizontalBox;

    //개별 하트를 렌더링하기 위한 단위 위젯 에셋 클래스
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI | HP")
    TSubclassOf<UUserWidget> HeartUnitWidgetClass;

    // 기절 상태(물방울) 화면 덮개 패널 추가
    UPROPERTY(meta = (BindWidget))
    UWidget* StunOverlayPanel;

    // 기절 탈출 진행률 바
    UPROPERTY(meta = (BindWidget))
    UProgressBar* StunProgressBar;

    //  기절 시간을 안내할 텍스트
    UPROPERTY(meta = (BindWidget))
    UTextBlock* StunWarningText;

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
    // 하트 개수 업데이트 함수 추가
    UFUNCTION(BlueprintCallable, Category = "UI | Update")
    void UpdateHearts(int32 CurrentHearts, int32 MaxHearts);

    // 기절 UI 활성화 제어 함수 추가
    UFUNCTION(BlueprintCallable, Category = "UI | Update")
    void SetStunActive(bool bIsActive);

    // 기절 탈출 게이지 갱신 함수 추가
    UFUNCTION(BlueprintCallable, Category = "UI | Update")
    void UpdateStunProgress(float Percent);

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
