// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SpartaUIDefs.h"
#include "SpartaGameMode.generated.h"

class ASpartaGameState;
class ASpartaPlayerState;
class UCombatComponent;

USTRUCT(BlueprintType)
struct FTeamInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 TeamID;

	UPROPERTY(BlueprintReadOnly)
	int32 AliveCount;

	UPROPERTY(BlueprintReadOnly)
	bool bEliminated;

	UPROPERTY(BlueprintReadOnly)
	int32 Rank;

	UPROPERTY(BlueprintReadOnly)
	int32 SurvivalTime;

	UPROPERTY(BlueprintReadOnly)
	TArray<ASpartaPlayerState*> TeamPlayerStates;
};

UCLASS()
class SPARTAARCADE_API ASpartaGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	ASpartaGameMode();
		
	virtual void BeginPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void Logout(AController* Exiting) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	virtual void StartMatch() override;

	virtual void EndMatch() override;

    void HandlePlayerEliminated(ASpartaPlayerState* DeadPlayer);

    void DecreaseAlivePlayer();

    void DecreaseAliveTeam();

    void CheckGameEnd();

	void InitializeTeamInfo();

	void ShowGameResultToAllPlayers();

	void ShowGameResultToTeam(int32 TeamID);

	void ShowGameResultToEliminatedPlayer(ASpartaPlayerState* DeadPlayer);

	FMatchPlayerResult CreateGameResult(const ASpartaPlayerState* PlayerState);

	bool IsTeamEliminated(int32 TeamID) const;

	//----------------------
	// 스폰 위치 조정 함수
	void TeleportPlayersToSpawns(const TArray<FVector>& SpawnLocations);

	void ExecuteSafeTeleportAndClear(APlayerController* PC, const TArray<FVector>& SpawnLocations);

	int32 GetOrAssignSpawnIndex(APlayerController* PC);

	FVector CalculateSafeSpawnLocation(int32 AssignedIndex, const FVector& BaseLocation, APawn* IgnoredPawn);

	void ClearAroundLocation(const FVector& Location, APawn* IgnoredPawn);



private:
	TObjectPtr<ASpartaGameState> SpartaGameState;

	TMap<int32, FTeamInfo> TeamInfoMap;

	TArray<FMatchPlayerResult> MatchResults;

	int32 MaxInitializeTeamInfoCount;
	int32 CurrentInitializeTeamInfoCount;

	UPROPERTY()
	TMap<class AController*, int32> AssignedSpawnIndices;

};
