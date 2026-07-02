// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "SpartaUIDefs.h"
#include "Lobby/LobbyGameStateBase.h"

ALobbyPlayerState::ALobbyPlayerState() 
	: bIsReady(false)
	, SelectedCharacterType(ELobbyCharacterType::CharacterA)
{
	bReplicates = true;
}

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALobbyPlayerState, bIsReady);
	DOREPLIFETIME(ALobbyPlayerState, SelectedCharacterType);
}

void ALobbyPlayerState::OnRep_LobbyStateChanged()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (ALobbyGameStateBase* LobbyGameState = GetWorld()->GetGameState<ALobbyGameStateBase>())
	{
		LobbyGameState->RefreshLobbyUI();
	}
}