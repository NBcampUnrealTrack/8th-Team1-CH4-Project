// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LobbyGameStateBase.generated.h"

class USpartaLobbyWidget;
enum class EGameModeType : uint8;

UCLASS()
class SPARTAARCADE_API ALobbyGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	ALobbyGameStateBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void RefreshLobbyUI();

	void SetLobbyUIWidget(USpartaLobbyWidget* NewLobbyUIWidget);
	
	UFUNCTION()
	void OnRep_RoomInfoChanged();

	UFUNCTION()
	void OnRep_StartCountdownTime();

protected:
	UPROPERTY()
	USpartaLobbyWidget* LobbyUIWidget;

public:
	UPROPERTY(ReplicatedUsing = OnRep_RoomInfoChanged)
	APlayerState* HostPlayerState;

	UPROPERTY(ReplicatedUsing = OnRep_RoomInfoChanged)
	int32 MaxPlayerCount;

	UPROPERTY(ReplicatedUsing = OnRep_RoomInfoChanged)
	int32 MinPlayerCount;

	UPROPERTY(ReplicatedUsing = OnRep_RoomInfoChanged)
	int32 CurrentPlayerCount;
	
	UPROPERTY(ReplicatedUsing = OnRep_StartCountdownTime)
	int32 StartCountdownTime;

	UPROPERTY(ReplicatedUsing = OnRep_RoomInfoChanged)
	EGameModeType GameModeType;

	UPROPERTY(ReplicatedUsing = OnRep_RoomInfoChanged)
	TArray<APlayerState*> PlayerStates;

	UPROPERTY(ReplicatedUsing = OnRep_RoomInfoChanged)
	bool bAutoBalanceTeam;

	UPROPERTY(ReplicatedUsing = OnRep_RoomInfoChanged)
	int32 TeamCount;
};
