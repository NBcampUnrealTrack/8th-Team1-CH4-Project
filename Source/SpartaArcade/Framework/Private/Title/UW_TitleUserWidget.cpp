// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/UW_TitleUserWidget.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Title/TitlePlayerController.h"

UUW_TitleUserWidget::UUW_TitleUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UUW_TitleUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (IsValid(PlayButton) == true)
	{
		PlayButton->OnClicked.AddDynamic(this, &UUW_TitleUserWidget::OnPlayButtonClicked);
	}
	if (IsValid(ExitButton) == true)
	{
		ExitButton->OnClicked.AddDynamic(this, &UUW_TitleUserWidget::OnExitButtonClicked);
	}
}

void UUW_TitleUserWidget::OnPlayButtonClicked()
{
	ATitlePlayerController* TitlePlayerController = Cast<ATitlePlayerController>(GetOwningPlayer());
	if(IsValid(TitlePlayerController) == true)
	{
		if (IsValid(ServerIPEditableText) == true)
		{
			FString ServerIP = ServerIPEditableText->GetText().ToString();
			if (ServerIP.IsEmpty() == false)
			{
				FString PlayerName = TEXT("Player");	
				FString PlayerNameInput = PlayerNameEditableText->GetText().ToString();
				if (PlayerNameInput.IsEmpty() == false)
				{
					PlayerName = PlayerNameInput;
				}
				TitlePlayerController->JoinServer(ServerIP, PlayerName);
			}
		}
	}
}

void UUW_TitleUserWidget::OnExitButtonClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}