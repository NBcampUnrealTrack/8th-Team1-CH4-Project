// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyGameModeBase.h"
#include "Lobby/LobbyGameStateBase.h"
#include "Lobby/LobbyPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "EOSGameInstanceSubsystem.h"
#include "SessionService.h"
#include "SpartaUIDefs.h"
#include "GameFlow/TravelGameInstanceSubsystem.h"
#include "Algo/RandomShuffle.h"

ALobbyGameModeBase::ALobbyGameModeBase()
{
	GameStateClass = ALobbyGameStateBase::StaticClass();
	PlayerStateClass = ALobbyPlayerState::StaticClass();

	bUseSeamlessTravel = true;
}

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

void ALobbyGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	InitializeLobbyGameState();
}

void ALobbyGameModeBase::InitializeLobbyGameState()
{
	ALobbyGameStateBase* LobbyGameState = GetGameState<ALobbyGameStateBase>();
	UEOSGameInstanceSubsystem* EOSGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UEOSGameInstanceSubsystem>();
	if (IsValid(LobbyGameState) && IsValid(EOSGameInstanceSubsystem))
	{
		FSessionInfo CurrentSessionInfo = EOSGameInstanceSubsystem->GetSessionService()->GetCurrentSessionInfo();
		LobbyGameState->CurrentPlayerCount = CurrentSessionInfo.CurrentPlayers;
		LobbyGameState->MaxPlayerCount = CurrentSessionInfo.MaxPlayers;
		LobbyGameState->GameModeType = static_cast<EGameModeType>(CurrentSessionInfo.GameModeType);
		LobbyGameState->OnRep_RoomInfoChanged();
	}
}

bool ALobbyGameModeBase::IsCanStartMatch() const
{
	ALobbyGameStateBase* LobbyGameState = GetGameState<ALobbyGameStateBase>();
	if (IsValid(LobbyGameState))
	{
		if(LobbyGameState->CurrentPlayerCount < LobbyGameState->MinPlayerCount)
		{
			return false;
		}

		for (APlayerState* PlayerState : LobbyGameState->PlayerStates)
		{
			if (PlayerState == nullptr)
			{
				continue;
			}
			if (ALobbyPlayerState* LobbyPlayerState = Cast<ALobbyPlayerState>(PlayerState))
			{
				if (!LobbyPlayerState->GetIsReady())
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

void ALobbyGameModeBase::StartInGameMatch()
{
	if(IsCanStartMatch() == false)
	{
		return;
	}

	// 모든 조건이 충족되면 타이머 시작
	ALobbyGameStateBase* LobbyGameState = GetGameState<ALobbyGameStateBase>();
	if (IsValid(LobbyGameState))
	{
		LobbyGameState->StartCountdownTime = StartCountdownTimeRemaining;
		LobbyGameState->OnRep_StartCountdownTime();
		GetWorldTimerManager().SetTimer(StartCountdownTimerHandle, this, &ALobbyGameModeBase::UpdateMatchStartCountdown, 1.0f, true);
	}
}

void ALobbyGameModeBase::UpdateMatchStartCountdown()
{
	ALobbyGameStateBase* LobbyGameState = GetGameState<ALobbyGameStateBase>();
	if (IsValid(LobbyGameState))
	{
		LobbyGameState->StartCountdownTime--;
		LobbyGameState->OnRep_StartCountdownTime();
		if (LobbyGameState->StartCountdownTime <= 0)
		{
			GetWorldTimerManager().ClearTimer(StartCountdownTimerHandle);
			AutoAssignTeams(LobbyGameState->TeamCount);
			GetGameInstance()->GetSubsystem<UTravelGameInstanceSubsystem>()->TravelToInGameMap();
		}
	}
}

void ALobbyGameModeBase::AutoAssignTeams(int32 TeamCount)
{
	ALobbyGameStateBase* LobbyGameState = GetGameState<ALobbyGameStateBase>();
	if (IsValid(LobbyGameState))
	{
		if (LobbyGameState->GameModeType == EGameModeType::Solo || !LobbyGameState->bAutoBalanceTeam)
		{
			return;
		}

		if(TeamCount <= 0)
		{
			TeamCount = 2;
		}

		TArray<ALobbyPlayerState*> Players;
		for (APlayerState* PlayerState : LobbyGameState->PlayerArray)
		{
			if (ALobbyPlayerState* LobbyPlayerState = Cast<ALobbyPlayerState>(PlayerState))
			{
				Players.Add(LobbyPlayerState);
			}
		}
		Algo::RandomShuffle(Players);

		for (int32 i = 0; i < Players.Num(); ++i)
		{
			int32 TeamID = (i % TeamCount) + 1;
			Players[i]->SetTeamID(TeamID);
		}
	}
}