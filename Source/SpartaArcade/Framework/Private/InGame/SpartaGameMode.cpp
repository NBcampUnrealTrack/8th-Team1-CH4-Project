// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/SpartaGameMode.h"
#include "InGame/SpartaGameState.h"
#include "InGame/SpartaPlayerState.h"
#include "SpartaUIDefs.h"
#include "CombatComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "Level/Public/SpartaArcadeMapBuilder.h"
#include "SpartaArcadePlayerController.h"

ASpartaGameMode::ASpartaGameMode()
	: MaxInitializeTeamInfoCount(10)
	, CurrentInitializeTeamInfoCount(0)
{
    GameStateClass = ASpartaGameState::StaticClass();
	PlayerStateClass = ASpartaPlayerState::StaticClass();

	bUseSeamlessTravel = true;
}

void ASpartaGameMode::BeginPlay()
{
	Super::BeginPlay();
	SpartaGameState = GetGameState<ASpartaGameState>();
	GetWorldTimerManager().SetTimerForNextTick(this, &ASpartaGameMode::InitializeTeamInfo);
}

void ASpartaGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	if (IsValid(SpartaGameState))
	{
		ASpartaPlayerState* ExitingPlayerState = Cast<ASpartaPlayerState>(Exiting->PlayerState);
		if (IsValid(ExitingPlayerState))
		{
			HandlePlayerEliminated(ExitingPlayerState);
		}
	}

	if (AssignedSpawnIndices.Contains(Exiting))
	{
		int32 ReturnedIndex = AssignedSpawnIndices[Exiting];
		AssignedSpawnIndices.Remove(Exiting);
		UE_LOG(LogTemp, Warning, TEXT("[SpawnSystem] 플레이어 %s 가 퇴장하여 스폰 인덱스 %d 를 반납하였습니다."), *Exiting->GetName(), ReturnedIndex);
	}
}

void ASpartaGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (!NewPlayer) return;

	APlayerController* PC = Cast<APlayerController>(NewPlayer);
	if (PC)
	{
		for (TActorIterator<ASpartaArcadeMapBuilder> It(GetWorld()); It; ++It)
		{
			ASpartaArcadeMapBuilder* MapBuilder = *It;
			if (MapBuilder && MapBuilder->bMapBuilt)
			{
				const TArray<FVector>& SpawnLocations = MapBuilder->GetSpawnWorldLocations();

				ExecuteSafeTeleportAndClear(PC, SpawnLocations);
				break;
			}
		}
	}
}

AActor* ASpartaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!Player) return Super::ChoosePlayerStart_Implementation(Player);

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* Start = *It;
		if (Start && Start->PlayerStartTag.IsNone())
		{
			return Start;
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
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

	if (!IsValid(SpartaGameState))
	{
		return;
	}

	for (TPair<int32, FTeamInfo>& TeamInfoPair : TeamInfoMap)
	{
		if (!TeamInfoPair.Value.bEliminated)
		{
			TeamInfoPair.Value.Rank = 1;
			TeamInfoPair.Value.SurvivalTime = SpartaGameState->ElapsedTime;

			for (ASpartaPlayerState* PlayerState : TeamInfoPair.Value.TeamPlayerStates)
			{
				if (IsValid(PlayerState))
				{
					MatchResults.Add(CreateGameResult(PlayerState));
				}
			}
			break;
		}
	}
	Algo::Reverse(MatchResults);
	ShowGameResultToAllPlayers();
}

void ASpartaGameMode::HandlePlayerEliminated(ASpartaPlayerState* DeadPlayer)
{
	if (!IsValid(DeadPlayer) || !IsValid(SpartaGameState))
	{
		return;
	}

	int32 TeamID = SpartaGameState->GetGameModeType() == EGameModeType::Solo ?
		DeadPlayer->GetPlayerId() : DeadPlayer->GetTeamID();

	DecreaseAlivePlayer();
	
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
			for (ASpartaPlayerState* PlayerState : TeamInfoMap[TeamID].TeamPlayerStates)
			{
				if (IsValid(PlayerState))
				{
					MatchResults.Add(CreateGameResult(PlayerState));
				}
			}
			DecreaseAliveTeam();
		}
		TeamInfo.bEliminated ? ShowGameResultToTeam(TeamID) : ShowGameResultToEliminatedPlayer(DeadPlayer);
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
		for (APlayerState* PlayerState : SpartaGameState->PlayerArray)
		{
			ASpartaPlayerState* SpartaPlayerState = Cast<ASpartaPlayerState>(PlayerState);
			if (IsValid(SpartaPlayerState) == false)
			{
				
				if (CurrentInitializeTeamInfoCount < MaxInitializeTeamInfoCount)
				{
					++CurrentInitializeTeamInfoCount;
					GetWorldTimerManager().SetTimerForNextTick(this, &ASpartaGameMode::InitializeTeamInfo);
					return;
				}
				else break;
			}
		}

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

FMatchPlayerResult ASpartaGameMode::CreateGameResult(const ASpartaPlayerState* PlayerState)
{
	FMatchPlayerResult MatchResult;
	MatchResult.PlayerName = TEXT("Unknown");

	if(!IsValid(PlayerState) || !IsValid(SpartaGameState))
	{
		int32 TeamID = SpartaGameState->GetGameModeType() == EGameModeType::Solo ?
			PlayerState->GetPlayerId() : PlayerState->GetTeamID();
		if (TeamInfoMap.Contains(TeamID))
		{
			MatchResult.PlayerName = PlayerState->GetPlayerName();
			MatchResult.Rank = TeamInfoMap[TeamID].Rank;
			MatchResult.SurvivalTime = TeamInfoMap[TeamID].SurvivalTime;
		}
	}

	return MatchResult;
}

void ASpartaGameMode::ShowGameResultToAllPlayers()
{
	for (APlayerState* PlayerState : SpartaGameState->PlayerArray)
	{
		ASpartaPlayerState* SpartaPlayerState = Cast<ASpartaPlayerState>(PlayerState);
		if (IsValid(SpartaPlayerState))
		{
			ASpartaArcadePlayerController* PC = Cast<ASpartaArcadePlayerController>(SpartaPlayerState->GetOwner());
			if(IsValid(PC))
			{
				int32 TeamID = SpartaGameState->GetGameModeType() == EGameModeType::Solo ?
					SpartaPlayerState->GetPlayerId() : SpartaPlayerState->GetTeamID();
				int32 PlayerRank = TeamInfoMap.Contains(TeamID) ? TeamInfoMap[TeamID].Rank : 0;
				PC->ClientShowMatchResult(PlayerRank == 1 ? EMatchResult::Victory : EMatchResult::Defeat, PlayerRank, MatchResults);
			}
		}
	}
}

void ASpartaGameMode::ShowGameResultToTeam(int32 TeamID)
{
	if (TeamInfoMap.Contains(TeamID))
	{
		for(ASpartaPlayerState* PlayerState : TeamInfoMap[TeamID].TeamPlayerStates)
		{
			if(IsValid(PlayerState))
			{
				ASpartaArcadePlayerController* PC = Cast<ASpartaArcadePlayerController>(PlayerState->GetOwner());
				if(IsValid(PC))
				{
					int32 PlayerRank = TeamInfoMap[TeamID].Rank;
					PC->ClientShowMatchResult(EMatchResult::Defeat, PlayerRank, TArray<FMatchPlayerResult>());
				}
			}
		}
	}
}

void ASpartaGameMode::ShowGameResultToEliminatedPlayer(ASpartaPlayerState* DeadPlayer)
{
	if(IsValid(DeadPlayer))
	{
		ASpartaArcadePlayerController* PC = Cast<ASpartaArcadePlayerController>(DeadPlayer->GetOwner());
		if(IsValid(PC))
		{
			int32 TeamID = SpartaGameState->GetGameModeType() == EGameModeType::Solo ?
				DeadPlayer->GetPlayerId() : DeadPlayer->GetTeamID();
			int32 PlayerRank = TeamInfoMap.Contains(TeamID) ? TeamInfoMap[TeamID].Rank : 0;
			PC->ClientShowMatchResult(EMatchResult::None, PlayerRank, TArray<FMatchPlayerResult>());
		}
	}
}

//----------------------
// 스폰 위치 조정 함수
void ASpartaGameMode::TeleportPlayersToSpawns(const TArray<FVector>& SpawnLocations)
{
	if (SpawnLocations.Num() == 0) return;

	UE_LOG(LogTemp, Warning, TEXT("[SpawnSystem] 맵 빌드 완료 감지: 플레이어 텔레포트 및 3x3 안전지대 확보 연산을 실행합니다."));

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC)
		{
			ExecuteSafeTeleportAndClear(PC, SpawnLocations);
		}
	}
}

int32 ASpartaGameMode::GetOrAssignSpawnIndex(APlayerController* PC)
{
	if (!PC) return 0;

	if (AssignedSpawnIndices.Contains(PC))
	{
		return AssignedSpawnIndices[PC];
	}

	TSet<int32> UsedIndices;
	for (const auto& Pair : AssignedSpawnIndices)
	{
		UsedIndices.Add(Pair.Value);
	}

	int32 AssignedIndex = -1;
	for (int32 i = 0; i < 4; ++i)
	{
		if (!UsedIndices.Contains(i))
		{
			AssignedIndex = i;
			break;
		}
	}

	if (AssignedIndex == -1)
	{
		AssignedIndex = AssignedSpawnIndices.Num() % 4;
	}

	AssignedSpawnIndices.Add(PC, AssignedIndex);
	return AssignedIndex;
}

FVector ASpartaGameMode::CalculateSafeSpawnLocation(int32 AssignedIndex, const FVector& BaseLocation, APawn* IgnoredPawn)
{
	FVector FinalLoc = BaseLocation;

	TArray<FOverlapResult> WallOverlaps;
	FCollisionQueryParams TraceParams;
	if (IgnoredPawn) TraceParams.AddIgnoredActor(IgnoredPawn);

	FCollisionShape CheckSphere = FCollisionShape::MakeSphere(40.f);

	bool bIsFixedWallBlocked = false;
	if (GetWorld()->OverlapMultiByObjectType(
		WallOverlaps,
		FinalLoc,
		FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects),
		CheckSphere,
		TraceParams))
	{
		for (const FOverlapResult& Overlap : WallOverlaps)
		{
			AActor* OverlapActor = Overlap.GetActor();
			if (OverlapActor)
			{
				FString ActorName = OverlapActor->GetName();
				if (ActorName.Contains(TEXT("Fixed")) || ActorName.Contains(TEXT("Wall")))
				{
					if (!ActorName.Contains(TEXT("Destructible")))
					{
						bIsFixedWallBlocked = true;
						break;
					}
				}
			}
		}
	}

	if (bIsFixedWallBlocked)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnSystem] 목적지 (%s) 에 고정벽 감지! 안전 우회 연산을 수행합니다."), *FinalLoc.ToString());

		float ShiftX = 0.f;
		float ShiftY = 0.f;
		const float ShiftDistance = 100.f;

		switch (AssignedIndex % 4)
		{
		case 0: ShiftX = ShiftDistance;  ShiftY = ShiftDistance;  break;
		case 1: ShiftX = -ShiftDistance; ShiftY = ShiftDistance;  break;
		case 2: ShiftX = ShiftDistance;  ShiftY = -ShiftDistance; break;
		case 3: ShiftX = -ShiftDistance; ShiftY = -ShiftDistance; break;
		}

		FinalLoc.X += ShiftX;
		FinalLoc.Y += ShiftY;
	}

	FinalLoc.Z += 10.f;
	return FinalLoc;
}

void ASpartaGameMode::ClearAroundLocation(const FVector& Location, APawn* IgnoredPawn)
{
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	if (IgnoredPawn) QueryParams.AddIgnoredActor(IgnoredPawn);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(140.f);
	if (GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		Location,
		FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects),
		Sphere,
		QueryParams))
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* OverlapActor = Overlap.GetActor();
			if (OverlapActor)
			{
				FString ActorName = OverlapActor->GetName();
				if (ActorName.Contains(TEXT("Destructible")) || ActorName.Contains(TEXT("Box")))
				{
					if (!ActorName.Contains(TEXT("Fixed")) && !ActorName.Contains(TEXT("Wall")))
					{
						OverlapActor->Destroy();
						UE_LOG(LogTemp, Warning, TEXT("[SpawnSystem] 안전구역 장애물 %s 파괴 완료"), *ActorName);
					}
				}
			}
		}
	}
}

void ASpartaGameMode::ExecuteSafeTeleportAndClear(APlayerController* PC, const TArray<FVector>& SpawnLocations)
{
	if (!PC || SpawnLocations.Num() == 0) return;

	APawn* PlayerPawn = PC->GetPawn();
	if (!PlayerPawn) return;

	int32 AssignedIndex = GetOrAssignSpawnIndex(PC);

	int32 TargetLocIndex = AssignedIndex % SpawnLocations.Num();
	FVector SafeLoc = CalculateSafeSpawnLocation(AssignedIndex, SpawnLocations[TargetLocIndex], PlayerPawn);

	FRotator TeleportRot = FRotator::ZeroRotator;
	PlayerPawn->TeleportTo(SafeLoc, TeleportRot, false, true);
	PC->ClientSetLocation(SafeLoc, TeleportRot);

	UE_LOG(LogTemp, Warning, TEXT("[SpawnSystem] 플레이어 %s 텔레포트 완료 (인덱스 %d -> 좌표 %s)"), *PC->GetName(), AssignedIndex, *SafeLoc.ToString());

	ClearAroundLocation(SafeLoc, PlayerPawn);
}