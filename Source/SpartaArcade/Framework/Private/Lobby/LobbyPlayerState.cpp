// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Characters/Public/SpartaArcadeCharacter.h"
#include "Lobby/LobbyGameStateBase.h"
#include "InGame/SpartaPlayerState.h"
ALobbyPlayerState::ALobbyPlayerState() 
	: SelectedCharacterType(ESpartaArcadeCharacterType::Explosive)
	, bIsReady(false)
	, TeamID(0)
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

ESpartaArcadeCharacterType ALobbyPlayerState::GetSelectedCharacterType() const
{
	return SelectedCharacterType;
}

bool ALobbyPlayerState::GetIsReady() const
{
	return bIsReady;
}

int32 ALobbyPlayerState::GetTeamID() const
{
	return TeamID;
}

void ALobbyPlayerState::SetSelectedCharacterType(ESpartaArcadeCharacterType NewCharacterType)
{
	if (HasAuthority())
	{
		SelectedCharacterType = NewCharacterType;
		OnRep_LobbyStateChanged();
	}
}

void ALobbyPlayerState::SetIsReady(bool bNewReady)
{
	if (HasAuthority())
	{
		bIsReady = bNewReady;
		OnRep_LobbyStateChanged();
	}
}

void ALobbyPlayerState::SetTeamID(int32 NewTeamID)
{
	if (HasAuthority())
	{
		TeamID = NewTeamID;
		OnRep_LobbyStateChanged();
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
		LobbyGameState->NotifyLobbyUI();
	}
}