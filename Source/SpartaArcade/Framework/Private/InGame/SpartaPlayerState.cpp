// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/SpartaPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Systems/Public/BomberTypes.h"

void ASpartaPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpartaPlayerState, CharacterType);
	DOREPLIFETIME(ASpartaPlayerState, FirstAidKits);
	DOREPLIFETIME(ASpartaPlayerState, TeamID);
	DOREPLIFETIME(ASpartaPlayerState, Hearts);
	DOREPLIFETIME(ASpartaPlayerState, CurrentState);
}

void ASpartaPlayerState::SetCharacterType(ESpartaArcadeCharacterType NewType)
{
	if (HasAuthority())
	{
		CharacterType = NewType;
	}
}

ESpartaArcadeCharacterType ASpartaPlayerState::GetCharacterType() const
{
	return CharacterType;
}

void ASpartaPlayerState::SetFirstAidKits(int32 NewCount)
{
	if (HasAuthority())
	{
		FirstAidKits = NewCount;
	}
}

int32 ASpartaPlayerState::GetFirstAidKits() const
{
	return FirstAidKits;
}

void ASpartaPlayerState::SetTeamID(int32 NewTeamID)
{
	if(HasAuthority())
	{
		TeamID = NewTeamID;
	}
}

int32 ASpartaPlayerState::GetTeamID() const
{
	return TeamID;
}

void ASpartaPlayerState::SetHearts(int32 NewHearts)
{
	if (HasAuthority())
	{
		Hearts = NewHearts;
	}
}

int32 ASpartaPlayerState::GetHearts() const
{
	return Hearts;
}

void ASpartaPlayerState::SetCurrentState(EBomberPlayerState NewState)
{
	if(HasAuthority())
	{
		CurrentState = NewState;
	}
}

EBomberPlayerState ASpartaPlayerState::GetCurrentState() const
{
	return CurrentState;
}

void ASpartaPlayerState:: SetStartHearts(int32 NewStartHearts)
{
	StartHearts = NewStartHearts;
}

int32 ASpartaPlayerState::GetStartHearts() const
{
	return StartHearts;
}

void ASpartaPlayerState::SetSelfReviveHearts(int32 NewSelfReviveHearts)
{
	SelfReviveHearts = NewSelfReviveHearts;
}

int32 ASpartaPlayerState::GetSelfReviveHearts() const
{
	return SelfReviveHearts;
}

void ASpartaPlayerState::OnRep_Hearts()
{
	if (OnHeartsChanged.IsBound())
	{
		OnHeartsChanged.Broadcast(Hearts, StartHearts);
	}
}

void ASpartaPlayerState::OnRep_CurrentState()
{
	if (OnStunStateChanged.IsBound())
	{
		OnStunStateChanged.Broadcast(CurrentState == EBomberPlayerState::Stunned);
	}
}
	

void ASpartaPlayerState::BroadcastCurrentState()
{
	OnRep_Hearts();
	OnRep_CurrentState();
}