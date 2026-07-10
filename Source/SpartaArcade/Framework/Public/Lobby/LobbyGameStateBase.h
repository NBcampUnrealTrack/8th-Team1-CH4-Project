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

	// 추후에 최대 인원을 설정할 수 있게 한다면 활성화
	//UPROPERTY(ReplicatedUsing = OnRep_RoomInfoChanged)
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
};
