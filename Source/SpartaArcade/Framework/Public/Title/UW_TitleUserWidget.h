// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_TitleUserWidget.generated.h"

class UButton;
class UEditableText;
class UEOSGameInstanceSubsystem;
class UCheckBox;
class UVerticalBox;

UCLASS()
class SPARTAARCADE_API UUW_TitleUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUW_TitleUserWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnPlayButtonClicked();

	UFUNCTION()
	void OnExitButtonClicked();

	UFUNCTION()
	void OnSettingSessionClicked();

	UFUNCTION()
	void OnCreateSessionButtonClicked();

	UFUNCTION()
	void OnSearchSessionButtonClicked();

	UFUNCTION()
	void OnLoginButtonClicked();

	UFUNCTION()
	void OnSoloModeButtonClicked();

	UFUNCTION()
	void OnTeamModeButtonClicked();

	void HandleSearchSessionComplete(bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResults);

	void UpdateSessionList(const TArray<FOnlineSessionSearchResult>& SearchResults);

private:
	UEOSGameInstanceSubsystem* EOSGameInstanceSubsystem;

	UPROPERTY(BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> PlayButton;

	UPROPERTY( BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> ExitButton;

	UPROPERTY(BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> SettingSessionButton;

	UPROPERTY(BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> SearchSessionButton;

	UPROPERTY(BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> LoginButton;

	UPROPERTY(BlueprintReadOnly, Category = "LobbyLevelUI", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UEditableText> ServerIPEditableText;

	UPROPERTY(BlueprintReadOnly, Category = "LobbyLevelUI", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UEditableText> PlayerNameEditableText;

	UPROPERTY(BlueprintReadOnly, Category = "CreateSession", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UEditableText> SessionNameEditableText;

	UPROPERTY(BlueprintReadOnly, Category = "CreateSession", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UCheckBox> IsPrivateCheckBox;

	UPROPERTY(BlueprintReadOnly, Category = "CreateSession", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UEditableText> MaxPlayerEditableText;

	UPROPERTY(BlueprintReadOnly, Category = "CreateSession", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> SoloModeButton;

	UPROPERTY(BlueprintReadOnly, Category = "CreateSession", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> TeamModeButton;

	UPROPERTY(BlueprintReadOnly, Category = "CreateSession", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> CreateSessionButton;

	UPROPERTY(BlueprintReadOnly, Category = "CreateSession", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UVerticalBox> SessionListVerticalBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SessionList", Meta = (AllowPrivateAccess = true))
	TSubclassOf<UUserWidget> SessionEntryWidgetClass;
};
