// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyGameStateBase.h"
#include "Lobby/LobbyPlayerState.h"
#include "SpartaUIDefs.h"
#include "UI/Public/SpartaLobbyWidget.h"
#include "Net/UnrealNetwork.h"

ALobbyGameStateBase::ALobbyGameStateBase()
	: LobbyUIWidget(nullptr)
	, HostPlayerState(nullptr)
	, MaxPlayerCount(4)
	, MinPlayerCount(2)
	, CurrentPlayerCount(0)
	, GameModeType(EGameModeType::Solo)
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
}

void ALobbyGameStateBase::OnRep_RoomInfoChanged()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	RefreshLobbyUI();
}

void ALobbyGameStateBase::OnRep_StartCountdownTime()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (IsValid(LobbyUIWidget))
	{
		LobbyUIWidget->UpdateCountdown(StartCountdownTime);
	}
}

void ALobbyGameStateBase::RefreshLobbyUI()
{
	if (GetNetMode() == NM_DedicatedServer || !IsValid(LobbyUIWidget))
	{
		return;
	}

	TArray<FString> PlayerNames;
	TArray<bool> ReadyStates;
	bool bAllReady = true;

	// 모든 플레이어의 상태를 갱신
	for (APlayerState* PlayerState : PlayerStates)
	{
		if(PlayerState == nullptr)
		{
			continue;
		}

		PlayerNames.Add(PlayerState->GetPlayerName());

		if (ALobbyPlayerState* LobbyPlayerState = Cast<ALobbyPlayerState>(PlayerState))
		{
			ReadyStates.Add(LobbyPlayerState->bIsReady);

			if (!LobbyPlayerState->bIsReady)
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

	// 호스트인 경우 Start 버튼을 활성화
	bool bIsHost = false;
	if (APlayerController* LocalController = GetWorld()->GetFirstPlayerController())
	{
		if (ALobbyPlayerState* LocalLobbyPlayerState = Cast<ALobbyPlayerState>(LocalController->PlayerState))
		{
			bIsHost = (LocalLobbyPlayerState == HostPlayerState);

			if(bIsHost)
			{
				LobbyUIWidget->SetStartButtonVisibility(true, bAllReady && PlayerStates.Num() >= MinPlayerCount);
			}
			else
			{
				LobbyUIWidget->SetStartButtonVisibility(false, false);
			}
		}
	}
	LobbyUIWidget->UpdatePlayerList(PlayerNames, ReadyStates);
}

void ALobbyGameStateBase::SetLobbyUIWidget(USpartaLobbyWidget* NewLobbyUIWidget)
{
	if(IsValid(NewLobbyUIWidget))
	{
		LobbyUIWidget = NewLobbyUIWidget;
	}
}