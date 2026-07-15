// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/SpartaGameMode.h"
#include "InGame/SpartaGameState.h"
#include "InGame/SpartaPlayerState.h"
#include "SpartaUIDefs.h"
#include "CombatComponent.h"

ASpartaGameMode::ASpartaGameMode()
{
    GameStateClass = ASpartaGameState::StaticClass();
	PlayerStateClass = ASpartaPlayerState::StaticClass();

	bUseSeamlessTravel = true;
}

void ASpartaGameMode::BeginPlay()
{
	Super::BeginPlay();
	SpartaGameState = GetGameState<ASpartaGameState>();

	if (IsValid(SpartaGameState))
	{
		InitializeTeamInfo();
	}
}

void ASpartaGameMode::StartMatch()
{
	Super::StartMatch();
	UE_LOG(LogTemp, Warning, TEXT("Match Started!"));
}

void ASpartaGameMode::EndMatch()
{
	Super::EndMatch();
	UE_LOG(LogTemp, Warning, TEXT("Match Ended!"));

	for(APlayerState* PlayerState : SpartaGameState->PlayerArray)
	{
		ASpartaPlayerState* SpartaPlayerState = Cast<ASpartaPlayerState>(PlayerState);
		if (IsValid(SpartaPlayerState))
		{
			SetGameResult(SpartaPlayerState);
		}
	}
}

void ASpartaGameMode::HandlePlayerEliminated(ASpartaPlayerState* DeadPlayer)
{
	if (!IsValid(DeadPlayer))
	{
		return;
	}

	DecreaseAlivePlayer();
	int32 TeamID = DeadPlayer->GetTeamID();

	if (TeamInfoMap.Contains(TeamID))
	{
		FTeamInfo& TeamInfo = TeamInfoMap[TeamID];
		--TeamInfo.AliveCount;

		if (SpartaGameState)
		{
			TeamInfo.Rank = SpartaGameState->GetAliveTeamCount();
			TeamInfo.SurvivalTime = SpartaGameState->ElapsedTime;
		}

		if (TeamInfo.AliveCount <= 0 && !TeamInfo.bEliminated)
		{
			TeamInfo.bEliminated = true;
			
			DecreaseAliveTeam();
		}
	}
	CheckGameEnd();
}

void ASpartaGameMode::DecreaseAlivePlayer()
{
	if (SpartaGameState)
	{
		int32 NewAlivePlayerCount = SpartaGameState->GetAlivePlayerCount() - 1;
		SpartaGameState->SetAlivePlayerCount(NewAlivePlayerCount);
	}
}

void ASpartaGameMode::DecreaseAliveTeam()
{
	if (SpartaGameState)
	{
		int32 NewAliveTeamCount = SpartaGameState->GetAliveTeamCount() - 1;
		SpartaGameState->SetAliveTeamCount(NewAliveTeamCount);
	}
}

void ASpartaGameMode::CheckGameEnd()
{
	if (SpartaGameState && SpartaGameState->GetAliveTeamCount() <= 1)
	{
		EndMatch();
	}
}

void ASpartaGameMode::InitializeTeamInfo()
{
	TeamInfoMap.Empty();
	
	if (SpartaGameState)
	{
		int32 TotalAlivePlayers = 0;
		for (APlayerState* PlayerState : SpartaGameState->PlayerArray)
		{
			ASpartaPlayerState* SpartaPlayerState = Cast<ASpartaPlayerState>(PlayerState);
			if (IsValid(SpartaPlayerState))
			{
				int32 TeamID = SpartaGameState->GetGameModeType() == EGameModeType::Solo ? 
					SpartaPlayerState->GetPlayerId() : SpartaPlayerState->GetTeamID();
				if (!TeamInfoMap.Contains(TeamID))
				{
					FTeamInfo NewTeamInfo;
					NewTeamInfo.TeamID = TeamID;
					NewTeamInfo.AliveCount = 0;
					NewTeamInfo.bEliminated = false;
					NewTeamInfo.Rank = 0;
					TeamInfoMap.Add(TeamID, NewTeamInfo);
				}
				++TeamInfoMap[TeamID].AliveCount;
				++TotalAlivePlayers;
			}
		}

		SpartaGameState->SetAlivePlayerCount(TotalAlivePlayers);
		SpartaGameState->SetAliveTeamCount(TeamInfoMap.Num());
		SpartaGameState->SetTotalAliveTeamCount(TeamInfoMap.Num());
	}
}

void ASpartaGameMode::SetGameResult(ASpartaPlayerState* PlayerState)
{
	FMatchPlayerResult MatchResult;

	if(IsValid(PlayerState) && TeamInfoMap.Contains(PlayerState->GetTeamID()))
	{
		int32 TeamID = PlayerState->GetTeamID();
		MatchResult.PlayerName = PlayerState->GetPlayerName();
		MatchResult.Rank = TeamInfoMap[TeamID].Rank;
		MatchResult.SurvivalTime = TeamInfoMap[TeamID].SurvivalTime;
	}
	else
	{
		MatchResult.PlayerName = TEXT("Unknown");
		MatchResult.Rank = 0;
		MatchResult.SurvivalTime = 0;
	}
}