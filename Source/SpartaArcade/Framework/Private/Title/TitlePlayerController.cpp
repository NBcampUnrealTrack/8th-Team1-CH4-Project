// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/TitlePlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

void ATitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}

	if (IsValid(UIWidgetClass) == true)
	{
		UIWidgetInstance = CreateWidget<UUserWidget>(this, UIWidgetClass);
		if (IsValid(UIWidgetInstance) == true)
		{
			UIWidgetInstance->AddToViewport();

			FInputModeUIOnly Mode;
			Mode.SetWidgetToFocus(UIWidgetInstance->GetCachedWidget());
			SetInputMode(Mode);

			bShowMouseCursor = true;
		}
	}
}

void ATitlePlayerController::JoinServer(const FString& InIPAddress, const FString& InPlayerName)
{
	FString ConnectionURL = FString::Printf(TEXT("%s?PlayerName=%s"), *InIPAddress, *InPlayerName);
	ClientTravel(ConnectionURL, ETravelType::TRAVEL_Absolute);
}