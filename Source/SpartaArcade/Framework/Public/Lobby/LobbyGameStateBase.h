// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LobbyGameStateBase.generated.h"

// 로비 화면 UI에 각 플레이어의 TeamID와 팀 자동 분배 여부(bAutoBalance)를 같이 전달할 수 있도록 7개 인자값 델리게이트로 확장 선언
DECLARE_MULTICAST_DELEGATE_SevenParams(FOnLobbyInfoChanged, const TArray<FString>&, const TArray<bool>&, const TArray<int32>&, bool, bool, bool, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCountdownChanged, int32);

class USpartaLobbyWidget;
enum class EGameModeType : uint8;

UCLASS()
class SPARTAARCADE_API ALobbyGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	ALobbyGameStateBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void NotifyLobbyUI();
	
	UFUNCTION()
	void OnRep_RoomInfoChanged();

	UFUNCTION()
	void OnRep_StartCountdownTime();

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

public:
	FOnLobbyInfoChanged OnLobbyInfoChanged;
	FOnCountdownChanged OnCountdownChanged;
};
