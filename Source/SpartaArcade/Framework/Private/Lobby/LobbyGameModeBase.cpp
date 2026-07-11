// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyGameModeBase.h"
#include "Lobby/LobbyGameStateBase.h"
#include "Lobby/LobbyPlayerState.h"
#include "Kismet/GameplayStatics.h"

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

void ALobbyGameModeBase::OnPlayerReadyStateChanged()
{
	ALobbyGameStateBase* LobbyGameState = GetGameState<ALobbyGameStateBase>();
	if (IsValid(LobbyGameState))
	{
		LobbyGameState->RefreshLobbyUI();
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
				if (!LobbyPlayerState->bIsReady)
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

			UWorld* World = GetWorld();
			if (IsValid(World))
			{
				if (InGameMap.IsNull() == false)
				{
					FString MapPath = InGameMap.GetLongPackageName();

					World->ServerTravel(MapPath + TEXT("?listen"));
				}
			}
		}
	}
}