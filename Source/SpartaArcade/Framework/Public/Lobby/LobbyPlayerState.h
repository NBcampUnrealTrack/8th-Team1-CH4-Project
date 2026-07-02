// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LobbyPlayerState.generated.h"

enum class ELobbyCharacterType : uint8;

UCLASS()
class SPARTAARCADE_API ALobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ALobbyPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyStateChanged)
	bool bIsReady;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyStateChanged)
	ELobbyCharacterType SelectedCharacterType;

	UFUNCTION()
	void OnRep_LobbyStateChanged();
};
