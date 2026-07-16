// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Lobby/LobbyPlayerState.h"
#include "Lobby/LobbyGameModeBase.h"
#include "Lobby/LobbyGameStateBase.h"
#include "UI/Public/SpartaLobbyWidget.h"
#include "EOSGameInstanceSubsystem.h"
#include "SessionService.h"
#include "GameFlow/TravelGameInstanceSubsystem.h"
#include "SpartaMenuFlowWidget.h"

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}

	if (IsValid(MainMenuWidgetClass) == true)
	{;
		MainMenuWidgetInstance = CreateWidget<USpartaMenuFlowWidget>(this, MainMenuWidgetClass);
		if (IsValid(MainMenuWidgetInstance) == true)
		{
			MainMenuWidgetInstance->AddToViewport();
			MainMenuWidgetInstance->ShowLobbyMenu();
			FInputModeUIOnly Mode;
			Mode.SetWidgetToFocus(MainMenuWidgetInstance->GetCachedWidget());
			SetInputMode(Mode);

			bShowMouseCursor = true;
		}
	}

	UEOSGameInstanceSubsystem* EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSGameInstanceSubsystem>();
	if(IsValid(EOSSubsystem) == true)
	{
		USessionService* SessionService = EOSSubsystem->GetSessionService();
		if(IsValid(SessionService) == true)
		{
			SessionService->OnDestroySessionCompleteEvent.AddUObject(this, &ALobbyPlayerController::HandleDestroySessionComplete);
		}
	}
}

// 캐릭터 변경 요청을 서버로 전송하는 함수
void ALobbyPlayerController::ServerSelectCharacter_Implementation(ESpartaArcadeCharacterType NewType)
{
	ALobbyPlayerState* LobbyPlayerState = GetPlayerState<ALobbyPlayerState>();
	if (IsValid(LobbyPlayerState) == true)
	{
		if(LobbyPlayerState->GetIsReady() == false && LobbyPlayerState->GetSelectedCharacterType() != NewType)
		{
			LobbyPlayerState->SetSelectedCharacterType(NewType);
			LobbyPlayerState->OnRep_LobbyStateChanged();
		}
	}
}

bool ALobbyPlayerController::ServerSelectCharacter_Validate(ESpartaArcadeCharacterType NewType)
{
	return true;
}

// 플레이어 준비 상태를 토글하는 요청을 서버로 전송하는 함수
void ALobbyPlayerController::ServerToggleReady_Implementation()
{
	ALobbyPlayerState* LobbyPlayerState = GetPlayerState<ALobbyPlayerState>();
	if (IsValid(LobbyPlayerState) == true)
	{
		LobbyPlayerState->SetIsReady(!LobbyPlayerState->GetIsReady());
		LobbyPlayerState->OnRep_LobbyStateChanged();
	}
}

bool ALobbyPlayerController::ServerToggleReady_Validate()
{
	return true;
}

// 매치 시작 요청을 서버로 전송하는 함수
void ALobbyPlayerController::ServerStartMatch_Implementation()
{
	ALobbyGameModeBase* LobbyGameMode = GetWorld()->GetAuthGameMode<ALobbyGameModeBase>();
	if (IsValid(LobbyGameMode) == true)
	{
		LobbyGameMode->StartInGameMatch();
	}
}

bool ALobbyPlayerController::ServerStartMatch_Validate()
{
	return true;
}

void ALobbyPlayerController::LeaveLobby()
{
	if (IsLocalController() == false)
	{
		return;
	}

	GetGameInstance()->GetSubsystem<UEOSGameInstanceSubsystem>()->GetSessionService()->DestroySession();
}

void ALobbyPlayerController::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UTravelGameInstanceSubsystem* TravelSubsystem = GetGameInstance()->GetSubsystem<UTravelGameInstanceSubsystem>();
		if (IsValid(TravelSubsystem))
		{
			TravelSubsystem->TravelToTitleMap();
		}
	}
}