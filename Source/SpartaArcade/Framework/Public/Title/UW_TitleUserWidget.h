// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_TitleUserWidget.generated.h"

class UButton;
class UEditableText;
class UEOSGameInstanceSubsystem;

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
	void OnCreateSessionButtonClicked();

	UFUNCTION()
	void OnSearchSessionButtonClicked();

	UFUNCTION()
	void OnLoginButtonClicked();

private:
	UPROPERTY(BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> PlayButton;

	UPROPERTY( BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> ExitButton;

	UPROPERTY(BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> CreateSessionButton;

	UPROPERTY(BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> SearchSessionButton;

	UPROPERTY(BlueprintReadOnly, Category = "TitleWidget", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> LoginButton;

	UPROPERTY(BlueprintReadOnly, Category = "LobbyLevelUI", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UEditableText> ServerIPEditableText;

	UPROPERTY(BlueprintReadOnly, Category = "LobbyLevelUI", Meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UEditableText> PlayerNameEditableText;

	UEOSGameInstanceSubsystem* EOSGameInstanceSubsystem;
};
