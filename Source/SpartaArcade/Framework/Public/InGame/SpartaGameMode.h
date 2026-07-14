// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SpartaGameMode.generated.h"

USTRUCT(BlueprintType)
struct FTeamInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TeamID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AliveCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEliminated;

};

class ASpartaGameState;
class ASpartaPlayerState;
class UCombatComponent;

UCLASS()
class SPARTAARCADE_API ASpartaGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	ASpartaGameMode();

	virtual void BeginPlay() override;

	virtual void StartMatch() override;

	virtual void EndMatch() override;

    void UpdatePlayZone(float DeltaTime);

    void HandlePlayerEliminated(ASpartaPlayerState* DeadPlayer);

    void AddPlayerScore(ASpartaPlayerState* PlayerState, int32 Score);

    void DecreaseAlivePlayer();

    void DecreaseAliveTeam();

    void CheckGameEnd();

	void InitializeTeamInfo();

private:
	TObjectPtr<ASpartaGameState> SpartaGameState;

	TMap<int32, FTeamInfo> TeamInfoMap;
};
