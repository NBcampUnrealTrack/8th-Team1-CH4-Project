#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Characters/Public/SpartaArcadeCharacter.h"
#include "SpartaLobbyWidget.generated.h"

class UScrollBox;
class UButton;
class UTextBlock;
class USpartaButton;
class UWidget;

UCLASS()
class SPARTAARCADE_API USpartaLobbyWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(meta = (BindWidget))
    UScrollBox* PlayerListScrollBox;

    UPROPERTY(meta = (BindWidget))
    USpartaButton* CharacterAButton;

    UPROPERTY(meta = (BindWidget))
    USpartaButton* CharacterBButton;

    UPROPERTY(meta = (BindWidget))
    USpartaButton* CharacterCButton;

    UPROPERTY(meta = (BindWidget))
    USpartaButton* ReadyButton;

    UPROPERTY(meta = (BindWidget))
    USpartaButton* StartButton;

    UPROPERTY(meta = (BindWidget))
    USpartaButton* QuitButton;

    // 팀 선택(Red, Blue) 및 팀 자동 분배(AutoBalance) 제어용 위젯 추가
    UPROPERTY(meta = (BindWidget))
    USpartaButton* RedTeamButton;

    UPROPERTY(meta = (BindWidget))
    USpartaButton* BlueTeamButton;

    UPROPERTY(meta = (BindWidget))
    USpartaButton* AutoBalanceToggleButton;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* CountdownTextBlock;

    UPROPERTY(meta = (BindWidget))
    UWidget* Countdown;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* PreviewStatRangeText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* PreviewStatSpeedText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* PreviewStatDescriptionText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* PreviewStatBombCountText;

    // 플레이어 이름 리스트 항목용 위젯 클래스
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby | Settings")
    TSubclassOf<UUserWidget> PlayerEntryWidgetClass;

public:
    // 외부 네트워크/로비 시스템으로부터의 수신 데이터 갱신
    // TeamIDs 정보를 받아 처리할 수 있도록 파라미터 확장
    UFUNCTION(BlueprintCallable, Category = "UI | Lobby")
    void UpdatePlayerList(const TArray<FString>& PlayerNames, const TArray<bool>& ReadyStates, const TArray<int32>& TeamIDs);

    UFUNCTION(BlueprintCallable, Category = "UI | Lobby")
    void UpdateCountdown(int32 RemainingSeconds);

    UFUNCTION(BlueprintCallable, Category = "UI | Lobby")
    void SetStartButtonVisibility(bool bIsHost, bool bCanStart);

    UFUNCTION(BlueprintCallable, Category = "UI | Lobby")
    void UpdateCharacterPreview(ESpartaArcadeCharacterType CharacterType);

    // TeamIDs 및 bAutoBalance 정보를 반영할 수 있도록 7개 파라미터 서명으로 수정
	void RefreshLobbyUI(const TArray<FString>& PlayerNames, const TArray<bool>& ReadyStates, const TArray<int32>& TeamIDs, bool bIsHost, bool bCanStart, bool bAutoBalance, int32 RemainingSeconds);

protected:
    // 네트워크/로비 파트로 요청 전달
    UFUNCTION()
    void OnCharacterAClicked();

    UFUNCTION()
    void OnCharacterBClicked();

    UFUNCTION()
    void OnCharacterCClicked();

    UFUNCTION()
    void OnReadyClicked();

    UFUNCTION()
    void OnStartClicked();

    UFUNCTION()
	void OnQuitClicked();

    // 팀 선택 및 자동 분배 토글 클릭 이벤트 핸들러 추가
    UFUNCTION()
    void OnRedTeamClicked();

    UFUNCTION()
    void OnBlueTeamClicked();

    UFUNCTION()
    void OnAutoBalanceToggleClicked();

private:
    // 로컬 선택 상태
    ESpartaArcadeCharacterType SelectedCharacterType = ESpartaArcadeCharacterType::Explosive;
    bool bIsReady = false;
};
