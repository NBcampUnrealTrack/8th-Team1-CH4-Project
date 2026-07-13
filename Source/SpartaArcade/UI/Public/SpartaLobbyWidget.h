#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Characters/Public/SpartaArcadeCharacter.h"
#include "SpartaLobbyWidget.generated.h"

class UScrollBox;
class UButton;
class UTextBlock;
class USpartaButton;

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
    UTextBlock* CountdownTextBlock;

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
    UFUNCTION(BlueprintCallable, Category = "UI | Lobby")
    void UpdatePlayerList(const TArray<FString>& PlayerNames, const TArray<bool>& ReadyStates);

    UFUNCTION(BlueprintCallable, Category = "UI | Lobby")
    void UpdateCountdown(int32 RemainingSeconds);

    UFUNCTION(BlueprintCallable, Category = "UI | Lobby")
    void SetStartButtonVisibility(bool bIsHost, bool bCanStart);

    UFUNCTION(BlueprintCallable, Category = "UI | Lobby")
    void UpdateCharacterPreview(ESpartaArcadeCharacterType CharacterType);

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

private:
    // 로컬 선택 상태
    ESpartaArcadeCharacterType SelectedCharacterType = ESpartaArcadeCharacterType::Explosive;
    bool bIsReady = false;
};
