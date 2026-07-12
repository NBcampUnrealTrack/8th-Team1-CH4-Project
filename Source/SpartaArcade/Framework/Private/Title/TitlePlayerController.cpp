// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/TitlePlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "EOSGameInstanceSubsystem.h"
#include "SessionService.h"
#include "GameFlow/TravelGameInstanceSubsystem.h"

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

	USessionService* SessionService = GetGameInstance()->GetSubsystem<UEOSGameInstanceSubsystem>()->GetSessionService();
	if (SessionService)
	{
		SessionService->OnCreateSessionCompleteEvent.AddUObject(this, &ATitlePlayerController::HandleCreateSessionComplete);
		SessionService->OnJoinSessionCompleteEvent.AddUObject(this, &ATitlePlayerController::HandleJoinSessionComplete);
	}
}

void ATitlePlayerController::JoinServer(const FString& InIPAddress, const FString& InPlayerName)
{
	FString ConnectionURL = FString::Printf(TEXT("%s?PlayerName=%s"), *InIPAddress, *InPlayerName);
	ClientTravel(ConnectionURL, ETravelType::TRAVEL_Absolute);
}

void ATitlePlayerController::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UTravelGameInstanceSubsystem* TravelSubsystem = GetGameInstance()->GetSubsystem<UTravelGameInstanceSubsystem>();
		if (IsValid(TravelSubsystem))
		{
			TravelSubsystem->TravelToLobbyMap();
		}
	}
}

void ATitlePlayerController::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result, const FString& Connect)
{
	UTravelGameInstanceSubsystem* TravelSubsystem = GetGameInstance()->GetSubsystem<UTravelGameInstanceSubsystem>();
	if(IsValid(TravelSubsystem))
	{
		TravelSubsystem->TravelToSession(Connect);
	}
}