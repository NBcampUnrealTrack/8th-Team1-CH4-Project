// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/SpartaGameMode.h"
#include "InGame/SpartaGameState.h"
#include "InGame/SpartaPlayerState.h"
#include "SpartaUIDefs.h"
#include "GameFlow/TravelGameInstanceSubsystem.h"
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

}

void ASpartaGameMode::UpdatePlayZone(float DeltaTime)
{
	// 자기장 로직
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

		if (TeamInfo.AliveCount <= 0 && !TeamInfo.bEliminated)
		{
			TeamInfo.bEliminated = true;
			DecreaseAliveTeam();
		}
	}
	CheckGameEnd();
}

void ASpartaGameMode::AddPlayerScore(ASpartaPlayerState* PlayerState, int32 Score)
{
	// 점수 로직은 추후에 추가
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
		UE_LOG(LogTemp, Warning, TEXT("PlayerArray Count : %d"),
			SpartaGameState->PlayerArray.Num());
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
					TeamInfoMap.Add(TeamID, NewTeamInfo);
				}
				++TeamInfoMap[TeamID].AliveCount;
				++TotalAlivePlayers;
			}
		}

		SpartaGameState->SetAlivePlayerCount(TotalAlivePlayers);
		SpartaGameState->SetAliveTeamCount(TeamInfoMap.Num());
	}
}