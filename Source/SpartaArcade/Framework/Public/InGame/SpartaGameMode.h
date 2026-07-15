// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
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
	};

UCLASS()
class SPARTAARCADE_API ASpartaGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	ASpartaGameMode();
		
	virtual void BeginPlay() override;

	virtual void StartMatch() override;

	virtual void EndMatch() override;

    void HandlePlayerEliminated(ASpartaPlayerState* DeadPlayer);

    void DecreaseAlivePlayer();

    void DecreaseAliveTeam();

    void CheckGameEnd();

	void InitializeTeamInfo();

	void SetGameResult(ASpartaPlayerState* PlayerState);

private:
	TObjectPtr<ASpartaGameState> SpartaGameState;

	TMap<int32, FTeamInfo> TeamInfoMap;
};
