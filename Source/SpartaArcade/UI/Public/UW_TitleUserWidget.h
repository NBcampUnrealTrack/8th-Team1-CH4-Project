#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_TitleUserWidget.generated.h"

class UButton;
class UEditableText;
class UEOSGameInstanceSubsystem;
class UCheckBox;
class UVerticalBox;
class USpartaButton;
class UWidget;

UCLASS()
class SPARTAARCADE_API UUW_TitleUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUW_TitleUserWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeConstruct() override;

	// UFUNCTION()
	// void OnPlayButtonClicked();

	UFUNCTION()
	void OnExitButtonClicked();

	UFUNCTION()
	void OnSettingSessionClicked();

	UFUNCTION()
	void OnCreateSessionButtonClicked();

	UFUNCTION()
	void OnSearchSessionButtonClicked();

	// UFUNCTION()
	// void OnLoginButtonClicked();

	UFUNCTION()
	void OnSoloModeButtonClicked();

	UFUNCTION()
	void OnTeamModeButtonClicked();

	UFUNCTION()
	void OnMaxPlayerTextCommitted(const FText& Text, ETextCommit::Type CommitType);

	void HandleSearchSessionComplete(bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResults);

	void UpdateSessionList(const TArray<FOnlineSessionSearchResult>& SearchResults);

private:
	UEOSGameInstanceSubsystem* EOSGameInstanceSubsystem;

	// UPROPERTY(BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	// TObjectPtr<USpartaButton> PlayButton;

	UPROPERTY( BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<USpartaButton> ExitButton;

	UPROPERTY(BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<USpartaButton> SettingSessionButton;

	UPROPERTY(BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<USpartaButton> SearchSessionButton;

	// UPROPERTY(BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	// TObjectPtr<USpartaButton> LoginButton;

	// UPROPERTY(BlueprintReadOnly, Category = "LobbyLevelUI", Meta = (AllowPrivateAccess, BindWidget))
	// TObjectPtr<UEditableText> ServerIPEditableText;
	//
	// UPROPERTY(BlueprintReadOnly, Category = "LobbyLevelUI", Meta = (AllowPrivateAccess, BindWidget))
	// TObjectPtr<UEditableText> PlayerNameEditableText;

	UPROPERTY(BlueprintReadOnly, Category = "CreateSession", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UEditableText> SessionNameEditableText;

	UPROPERTY(BlueprintReadOnly, Category = "CreateSession", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UCheckBox> IsPrivateCheckBox;

	UPROPERTY(BlueprintReadOnly, Category = "CreateSession", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UEditableText> MaxPlayerEditableText;

	UPROPERTY(BlueprintReadOnly, Category = "CreateSession", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<USpartaButton> SoloModeButton;

	UPROPERTY(BlueprintReadOnly, Category = "CreateSession", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<USpartaButton> TeamModeButton;

	UPROPERTY(BlueprintReadOnly, Category = "CreateSession", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<USpartaButton> CreateSessionButton;

	UPROPERTY(BlueprintReadOnly, Category = "CreateSession", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UVerticalBox> SessionListVerticalBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SessionList", Meta = (AllowPrivateAccess = true))
	TSubclassOf<UUserWidget> SessionEntryWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UWidget> MakeRoomOverlay;

	UPROPERTY(BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UWidget> SessionOverlay;
};
