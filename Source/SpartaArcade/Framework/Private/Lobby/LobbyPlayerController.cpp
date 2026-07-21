// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
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

	if (IsValid(LevelBGM) == true)
	{
		UGameplayStatics::SpawnSound2D(this, LevelBGM);
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

// 수동 팀 선택 RPC 구현 (준비 상태가 아닐 때만 1(Red) 또는 2(Blue) 팀 할당)
void ALobbyPlayerController::ServerSelectTeam_Implementation(int32 NewTeamID)
{
	ALobbyPlayerState* LobbyPlayerState = GetPlayerState<ALobbyPlayerState>();
	if (IsValid(LobbyPlayerState))
	{
		if (LobbyPlayerState->GetIsReady() == false)
		{
			LobbyPlayerState->SetTeamID(NewTeamID);
			LobbyPlayerState->OnRep_LobbyStateChanged();
		}
	}
}

bool ALobbyPlayerController::ServerSelectTeam_Validate(int32 NewTeamID)
{
	return true;
}

// 방장 팀 자동 배분 기능 On/Off RPC 구현 (방장인 경우에만 작동)
void ALobbyPlayerController::ServerSetAutoBalanceTeam_Implementation(bool bEnabled)
{
	ALobbyGameStateBase* LobbyGameState = GetWorld()->GetGameState<ALobbyGameStateBase>();
	if (IsValid(LobbyGameState))
	{
		if (LobbyGameState->HostPlayerState == PlayerState)
		{
			LobbyGameState->bAutoBalanceTeam = bEnabled;
			LobbyGameState->OnRep_RoomInfoChanged();
		}
	}
}

bool ALobbyPlayerController::ServerSetAutoBalanceTeam_Validate(bool bEnabled)
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