// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Characters/Public/SpartaArcadeCharacter.h"
#include "Lobby/LobbyGameStateBase.h"
#include "InGame/SpartaPlayerState.h"
ALobbyPlayerState::ALobbyPlayerState() 
	: bIsReady(false)
	, SelectedCharacterType(ESpartaArcadeCharacterType::Explosive)
	, TeamID(-1)
{
	bReplicates = true;
}

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALobbyPlayerState, bIsReady);
	DOREPLIFETIME(ALobbyPlayerState, SelectedCharacterType);
	DOREPLIFETIME(ALobbyPlayerState, TeamID);
}

void ALobbyPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	if (ASpartaPlayerState* SpartaPlayerState = Cast<ASpartaPlayerState>(PlayerState))
	{
		SpartaPlayerState->SetCharacterType(SelectedCharacterType);
		SpartaPlayerState->SetTeamID(TeamID);
	}
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