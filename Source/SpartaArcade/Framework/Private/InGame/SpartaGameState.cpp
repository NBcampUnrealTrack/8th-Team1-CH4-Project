// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/SpartaGameState.h"
#include "Net/UnrealNetwork.h"
#include "SpartaUIDefs.h"

ASpartaGameState::ASpartaGameState()
	: GameModeType(EGameModeType::Solo)
	, AlivePlayerCount(0)
	, AliveTeamCount(0)
	, ZonePhase(0)
{

}

void ASpartaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpartaGameState, GameModeType);
	DOREPLIFETIME(ASpartaGameState, AlivePlayerCount);
	DOREPLIFETIME(ASpartaGameState, AliveTeamCount);
	DOREPLIFETIME(ASpartaGameState, TotalAliveTeamCount);
	DOREPLIFETIME(ASpartaGameState, ZonePhase);
}

void ASpartaGameState::OnRep_AlivePlayerCount()
{
	OnAliveCountChanged.Broadcast();
}

void ASpartaGameState::OnRep_ElapsedTime()
{
	OnElapsedTimeChanged.Broadcast();
}

void ASpartaGameState::OnRep_ZonePhase()
{
	OnZonePhaseChanged.Broadcast();
}

EGameModeType ASpartaGameState::GetGameModeType() const
{
	return GameModeType;
}

int32 ASpartaGameState::GetAlivePlayerCount() const
{
	return AlivePlayerCount;
}

int32 ASpartaGameState::GetAliveTeamCount() const
{
	return AliveTeamCount;
}

int32 ASpartaGameState::GetTotalAliveTeamCount() const
{
	return TotalAliveTeamCount;
}

int32 ASpartaGameState::GetZonePhase() const
{
	return ZonePhase;
}

void ASpartaGameState::SetAlivePlayerCount(int32 NewAlivePlayerCount)
{
	if (HasAuthority())
	{
		AlivePlayerCount = NewAlivePlayerCount;
		OnRep_AlivePlayerCount();
	}
}

void ASpartaGameState::SetAliveTeamCount(int32 NewAliveTeamCount)
{
	if (HasAuthority())
	{
		AliveTeamCount = NewAliveTeamCount;
	}
}

void ASpartaGameState::SetTotalAliveTeamCount(int32 NewTotalAliveTeamCount)
{
	if (HasAuthority())
	{
		TotalAliveTeamCount = NewTotalAliveTeamCount;
	}
}

void ASpartaGameState::SetZonePhase(int32 NewZonePhase)
{
	if (HasAuthority())
	{
		ZonePhase = NewZonePhase;
		OnRep_ZonePhase();
	}
}

void ASpartaGameState::SetGameModeType(EGameModeType NewGameModeType)
{
	if (HasAuthority())
	{
		GameModeType = NewGameModeType;
	}
}
