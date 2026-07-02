// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Lobby/LobbyPlayerState.h"
#include "Lobby/LobbyGameModeBase.h"
#include "Lobby/LobbyGameStateBase.h"
#include "UI/Public/SpartaLobbyWidget.h"

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}

	if (IsValid(LobbyUIWidgetClass) == true)
	{
		LobbyUIWidgetInstance = CreateWidget<UUserWidget>(this, LobbyUIWidgetClass);
		if (IsValid(LobbyUIWidgetInstance) == true)
		{
			LobbyUIWidgetInstance->AddToViewport();

			FInputModeUIOnly Mode;
			Mode.SetWidgetToFocus(LobbyUIWidgetInstance->GetCachedWidget());
			SetInputMode(Mode);

			bShowMouseCursor = true;

			if(ALobbyGameStateBase* LobbyGameState = GetWorld()->GetGameState<ALobbyGameStateBase>())
			{
				LobbyGameState->SetLobbyUIWidget(Cast<USpartaLobbyWidget>(LobbyUIWidgetInstance));
				LobbyGameState->RefreshLobbyUI();
			}
		}
	}
}

// 캐릭터 변경 요청을 서버로 전송하는 함수
void ALobbyPlayerController::ServerSelectCharacter_Implementation(ELobbyCharacterType NewType)
{
	ALobbyPlayerState* LobbyPlayerState = GetPlayerState<ALobbyPlayerState>();
	if (IsValid(LobbyPlayerState) == true)
	{
		if(LobbyPlayerState->bIsReady == false && LobbyPlayerState->SelectedCharacterType != NewType)
		{
			LobbyPlayerState->SelectedCharacterType = NewType;
			LobbyPlayerState->OnRep_LobbyStateChanged();
		}
	}
}

bool ALobbyPlayerController::ServerSelectCharacter_Validate(ELobbyCharacterType NewType)
{
	return true;
}

// 플레이어 준비 상태를 토글하는 요청을 서버로 전송하는 함수
void ALobbyPlayerController::ServerToggleReady_Implementation()
{
	ALobbyPlayerState* LobbyPlayerState = GetPlayerState<ALobbyPlayerState>();
	if (IsValid(LobbyPlayerState) == true)
	{
		LobbyPlayerState->bIsReady = !LobbyPlayerState->bIsReady;
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