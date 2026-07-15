// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyGameStateBase.h"
#include "Lobby/LobbyPlayerState.h"
#include "SpartaUIDefs.h"
#include "UI/Public/SpartaLobbyWidget.h"
#include "Net/UnrealNetwork.h"

ALobbyGameStateBase::ALobbyGameStateBase()
	: HostPlayerState(nullptr)
	, MaxPlayerCount(4)
	, MinPlayerCount(2)
	, CurrentPlayerCount(0)
	, GameModeType(EGameModeType::Solo)
	, bAutoBalanceTeam(true)
	, TeamCount(2)
{
	bReplicates = true;
}

void ALobbyGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyGameStateBase, HostPlayerState);
	DOREPLIFETIME(ALobbyGameStateBase, MaxPlayerCount);
	DOREPLIFETIME(ALobbyGameStateBase, MinPlayerCount);
	DOREPLIFETIME(ALobbyGameStateBase, CurrentPlayerCount);
	DOREPLIFETIME(ALobbyGameStateBase, StartCountdownTime);
	DOREPLIFETIME(ALobbyGameStateBase, GameModeType);
	DOREPLIFETIME(ALobbyGameStateBase, PlayerStates);
	DOREPLIFETIME(ALobbyGameStateBase, bAutoBalanceTeam);
}

void ALobbyGameStateBase::OnRep_RoomInfoChanged()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

    NotifyLobbyUI();
}

void ALobbyGameStateBase::OnRep_StartCountdownTime()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	OnCountdownChanged.Broadcast(StartCountdownTime);
}

void ALobbyGameStateBase::NotifyLobbyUI()
{
    TArray<FString> PlayerNames;
    TArray<bool> ReadyStates;

    bool bAllReady = true;

    for (APlayerState* PlayerState : PlayerStates)
    {
        if (!PlayerState)
        {
            continue;
        }

        PlayerNames.Add(PlayerState->GetPlayerName());

        if (ALobbyPlayerState* LobbyPlayerState = Cast<ALobbyPlayerState>(PlayerState))
        {
            ReadyStates.Add(LobbyPlayerState->GetIsReady());

            if (!LobbyPlayerState->GetIsReady())
            {
                bAllReady = false;
            }
        }
        else
        {
            ReadyStates.Add(false);
            bAllReady = false;
        }
    }

    bool bIsHost = false;
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (ALobbyPlayerState* LocalPS = Cast<ALobbyPlayerState>(PC->PlayerState))
        {
            bIsHost = (LocalPS == HostPlayerState);
        }
    }

    const bool bCanStart = bAllReady && PlayerStates.Num() >= MinPlayerCount;
    OnLobbyInfoChanged.Broadcast(PlayerNames, ReadyStates,bIsHost, bCanStart, StartCountdownTime);
}