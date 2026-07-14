// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SpartaGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAliveCountChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnElapsedTimeChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnZonePhaseChanged);

enum class EGameModeType : uint8;

UCLASS()
class SPARTAARCADE_API ASpartaGameState : public AGameState
{
	GENERATED_BODY()

public:
	ASpartaGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_AlivePlayerCount();

	virtual void OnRep_ElapsedTime();

	UFUNCTION()
	void OnRep_ZonePhase();

	int32 GetAlivePlayerCount() const;
	int32 GetAliveTeamCount() const;
	int32 GetTotalAliveTeamCount() const;
	int32 GetZonePhase() const;
	EGameModeType GetGameModeType() const;
	void SetAlivePlayerCount(int32 NewAlivePlayerCount);
	void SetAliveTeamCount(int32 NewAliveTeamCount);
	void SetTotalAliveTeamCount(int32 NewTotalAliveTeamCount);
	void SetZonePhase(int32 NewZonePhase);

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "GameState")
	EGameModeType GameModeType;

	UPROPERTY(ReplicatedUsing = OnRep_AlivePlayerCount, BlueprintReadOnly, Category = "GameState")
	int32 AlivePlayerCount;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "GameState")
	int32 AliveTeamCount;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "GameState")
	int32 TotalAliveTeamCount;

	UPROPERTY(ReplicatedUsing = OnRep_ZonePhase, BlueprintReadOnly, Category = "GameState")
	int32 ZonePhase;

public:
	UPROPERTY(BlueprintAssignable)
	FOnAliveCountChanged OnAliveCountChanged;

	UPROPERTY(BlueprintAssignable)
	FOnElapsedTimeChanged OnElapsedTimeChanged;

	UPROPERTY(BlueprintAssignable)
	FOnZonePhaseChanged OnZonePhaseChanged;
	
};
