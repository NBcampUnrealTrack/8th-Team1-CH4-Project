#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpartaUIDefs.h"
#include "SpartaMenuFlowWidget.generated.h"

class UWidgetSwitcher;
class UButton;
class UTextBlock;
class UScrollBox;

UCLASS()
class SPARTAARCADE_API USpartaMenuFlowWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // --- UMG 위젯 바인딩 ---
    UPROPERTY(meta = (BindWidget))
    UWidgetSwitcher* MenuWidgetSwitcher;

    // 1) 메인 메뉴 관련 위젯
    UPROPERTY(meta = (BindWidget))
    UButton* JoinButton;

    UPROPERTY(meta = (BindWidget))
    UButton* SettingsButton;

    UPROPERTY(meta = (BindWidget))
    UButton* QuitButton;

    // 2) 일시정지 메뉴 관련 위젯
    UPROPERTY(meta = (BindWidget))
    UButton* ResumeButton;

    UPROPERTY(meta = (BindWidget))
    UButton* ExitToLobbyButton;

    // 3) 게임 시작 카운트다운 관련 위젯
    UPROPERTY(meta = (BindWidget))
    UTextBlock* MatchStartCountdownText;

    // 4) 게임 종료(결과) 화면 관련 위젯
    UPROPERTY(meta = (BindWidget))
    UTextBlock* ResultTitleText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* MyRankText;

    UPROPERTY(meta = (BindWidget))
    UScrollBox* LeaderboardScrollBox;

    UPROPERTY(meta = (BindWidget))
    UButton* LobbyReturnButton;

    // 리더보드 한 항목을 그릴 위젯 클래스
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MenuFlow | Settings")
    TSubclassOf<UUserWidget> LeaderboardEntryWidgetClass;

public:
    // --- 화면 상태 스위칭 함수 ---
    UFUNCTION(BlueprintCallable, Category = "UI | Flow")
    void ShowMainMenu();

    UFUNCTION(BlueprintCallable, Category = "UI | Flow")
    void ShowPauseMenu();

    UFUNCTION(BlueprintCallable, Category = "UI | Flow")
    void ShowStartCountdown(int32 RemainingSeconds);

    UFUNCTION(BlueprintCallable, Category = "UI | Flow")
    void ShowMatchResult(EMatchResult Result, int32 MyRank, const TArray<FMatchPlayerResult>& PlayerResults);

protected:
    // --- 메인 메뉴 클릭 이벤트 ---
    UFUNCTION()
    void OnJoinClicked();

    UFUNCTION()
    void OnSettingsClicked();

    UFUNCTION()
    void OnQuitClicked();

    // --- 일시정지 메뉴 클릭 이벤트 ---
    UFUNCTION()
    void OnResumeClicked();

    UFUNCTION()
    void OnExitToLobbyClicked();

    // --- 결과 화면 클릭 이벤트 ---
    UFUNCTION()
    void OnLobbyReturnClicked();

private:
    // WidgetSwitcher 인덱스 상수 정의
    static constexpr int32 Index_MainMenu = 0;
    static constexpr int32 Index_PauseMenu = 1;
    static constexpr int32 Index_StartCountdown = 2;
    static constexpr int32 Index_ResultScreen = 3;
};
