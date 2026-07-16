#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpartaUIDefs.h"
#include "SpartaHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;
class ASpartaPlayerState;
class UStatComponent;
class UCombatComponent;
class UBombPlacerComponent;
class UBomberAttributeSet;

UCLASS()
class SPARTAARCADE_API USpartaHUDWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    
    // 기절 진행률 게이지 실시간 업데이트를 위한 NativeTick 오버라이드 추가
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    
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
    
    // 쉴드 텍스트가 위젯에서 삭제되더라도 컴파일 오류가 나지 않도록 OptionalWidget = true 속성을 부여합니다.
    UPROPERTY(meta = (BindWidget, OptionalWidget = true))
    UTextBlock* ShieldStatusText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* AlivePlayersText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* MatchTimeText;

    // --- 피격 피드백 UI 에셋 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI | Feedback")
    TSubclassOf<UUserWidget> DamageTextWidgetClass;

    // 스탯 바 UI 슬롯 템플릿과 3가지 스탯 바 컴포넌트 변수 선언 (OptionalWidget을 주어 블루프린트 매핑 예외 방지)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI | Stats")
    TSubclassOf<class USpartaArcadeStatSlot> StatSlotWidgetClass;
    
    UPROPERTY(meta = (BindWidget, OptionalWidget = true))
    class USpartaArcadeStatBar* RangeBar;

    UPROPERTY(meta = (BindWidget, OptionalWidget = true))
    class USpartaArcadeStatBar* SpeedBar;

    // 쉴드 및 구급상자 상태를 켜고 끌 단일 아이콘 이미지 컴포넌트 선언 (OptionalWidget)
    UPROPERTY(meta = (BindWidget, OptionalWidget = true))
    UImage* ShieldImg;

    UPROPERTY(meta = (BindWidget, OptionalWidget = true))
    UImage* MedImg;

    // 발차기 활성화 여부를 나타낼 아이콘 이미지 컴포넌트 변수 추가 (OptionalWidget)
    UPROPERTY(meta = (BindWidget, OptionalWidget = true))
    UImage* KickImg;

    UPROPERTY(meta = (BindWidget, OptionalWidget = true))
    class USpartaMinimapWidget* WBP_MinimapWidget; // 미니맵 UI 연동 변수 추가



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
    void UpdateGameStateInfo(int32 AlivePlayers, int32 MatchSeconds);

    // 쉴드 및 구급약 획득 여부에 따른 개별 아이콘 가시성 갱신 함수 선언
    UFUNCTION(BlueprintCallable, Category = "UI | Update")
    void UpdateShieldStatus(bool bHasShield);

    UFUNCTION(BlueprintCallable, Category = "UI | Update")
    void UpdateMedKitStatus(int32 MedKitCount);

    UFUNCTION(BlueprintCallable, Category = "UI | Update")
    void UpdateShieldItemStatus(int32 ShieldCount);

    // 발차기 활성화 가시성 제어 함수 선언
    UFUNCTION(BlueprintCallable, Category = "UI | Update")
    void UpdateKickStatus(bool bCanKick);
    
    // OnHit 델리게이트와 시그니처를 맞추기 위해 매개변수가 없는 HandleOnHit 추가
    UFUNCTION()
    void HandleOnHit();

    UFUNCTION()
    void HandleOnItemPickup(EItemType ItemType, int32 NewCount);

    UFUNCTION()
    void HandleOnShieldBlock();

    // 발차기 델리게이트 수신 핸들러 함수 선언
    UFUNCTION()
    void HandleOnKickUnlockedChanged(bool bIsUnlocked);

    UFUNCTION()
    void HandleOnEliminated();

    // UI 업데이트 함수 추가
    UFUNCTION(BlueprintCallable, Category = "UI | Update")
    void UpdateCurrentBombs(int32 CurrentBombs);

    UFUNCTION(BlueprintCallable, Category = "UI | Update")
	void UpdateStats(int32 BombCount, float ExplosionRange, float MoveSpeed);

    UFUNCTION(BlueprintCallable, Category = "UI | Update")
    void UpdateHasShield(bool bHasShield);

	void InitializeHUD(ASpartaPlayerState* PlayerState, UBomberAttributeSet* InAttributeSet, UCombatComponent* CombatComp, UBombPlacerComponent* BombPlacerComp);

private:
    // 매 틱마다 무거운 정보(생존 플레이어 루프, 자기장 탐색, 텍스트 리빌드)를 갱신하지 않도록 1초 주기 타이머 추가
    FTimerHandle GameStateTimerHandle;
    void UpdateGameStateTimer();

    // 내부 헬퍼 함수
    FString FormatTime(int32 TotalSeconds) const;

    // 실시간 게이지 업데이트 및 델리게이트 관리를 위해 각 컴포넌트 멤버 변수 캐싱 추가
    UPROPERTY()
    TObjectPtr<ASpartaPlayerState> SpartaPlayerState;

    UPROPERTY()
    TObjectPtr<UStatComponent> StatComponent;

    UPROPERTY()
    TObjectPtr<UBomberAttributeSet> BomberAttributeSet;

    UPROPERTY()
    TObjectPtr<UCombatComponent> CombatComponent;

    UPROPERTY()
    TObjectPtr<UBombPlacerComponent> BombPlacerComponent;

    int32 CachedAliveCount    = -1;
    int32 CachedMatchSeconds  = -1;
    float CachedStunProgress  = -1.0f;
    int32 CachedCurrentHearts = -1;
    int32 CachedMaxHearts     = -1;
};
