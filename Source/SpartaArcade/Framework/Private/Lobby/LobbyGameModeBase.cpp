// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyGameModeBase.h"
#include "Lobby/LobbyGameStateBase.h"
#include "Lobby/LobbyPlayerState.h"
#include "Kismet/GameplayStatics.h"

void ALobbyGameModeBase::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	ALobbyGameStateBase* LobbyGameState = GetGameState<ALobbyGameStateBase>();
	if(IsValid(LobbyGameState))
	{
		if (LobbyGameState->PlayerArray.Num() >= LobbyGameState->MaxPlayerCount)
		{
			ErrorMessage = TEXT("Lobby is full.");
			return;
		}
	}
}

FString ALobbyGameModeBase::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	FString Result = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	ALobbyGameStateBase* LobbyGameState = GetGameState<ALobbyGameStateBase>();
	if (IsValid(LobbyGameState))
	{
		FString PlayerName = UGameplayStatics::ParseOption(Options, TEXT("PlayerName"));
		if (NewPlayerController->PlayerState)
		{
			if (!PlayerName.IsEmpty())
			{
				NewPlayerController->PlayerState->SetPlayerName(PlayerName);
			}

			else
			{
				NewPlayerController->PlayerState->SetPlayerName(FString::Printf(TEXT("Player%d"), LobbyGameState->PlayerStates.Num() + 1));
			}
		}
	}
	return Result;
}

void ALobbyGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ALobbyGameStateBase* LobbyGameState = GetGameState<ALobbyGameStateBase>();
	if (IsValid(LobbyGameState))
	{
		if (LobbyGameState->HostPlayerState == nullptr && NewPlayer->PlayerState)
		{
			LobbyGameState->HostPlayerState = NewPlayer->PlayerState;
		}
		LobbyGameState->PlayerStates.Add(NewPlayer->PlayerState);
		LobbyGameState->CurrentPlayerCount = LobbyGameState->PlayerStates.Num();
		LobbyGameState->OnRep_RoomInfoChanged();
	}
}

void ALobbyGameModeBase::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	ALobbyGameStateBase* LobbyGameState = GetGameState<ALobbyGameStateBase>();
	if(IsValid(LobbyGameState))
	{
		if (LobbyGameState->HostPlayerState == Exiting->PlayerState)
		{
			LobbyGameState->HostPlayerState = nullptr;

			for(APlayerState* RemainingPlayerState : LobbyGameState->PlayerStates)
			{
				if (RemainingPlayerState && RemainingPlayerState != Exiting->PlayerState)
				{
					LobbyGameState->HostPlayerState = RemainingPlayerState;
					break;
				}
			}
		}
		LobbyGameState->PlayerStates.Remove(Exiting->PlayerState);
		LobbyGameState->CurrentPlayerCount = FMath::Max(0, LobbyGameState->PlayerStates.Num());
		LobbyGameState->OnRep_RoomInfoChanged();
	}
}

void ALobbyGameModeBase::OnPlayerReadyStateChanged()
{
	ALobbyGameStateBase* LobbyGameState = GetGameState<ALobbyGameStateBase>();
	if (IsValid(LobbyGameState))
	{
		LobbyGameState->RefreshLobbyUI();
	}
}

void ALobbyGameModeBase::StartInGameMatch()
{
	
}