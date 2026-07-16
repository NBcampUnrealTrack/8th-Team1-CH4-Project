// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LobbyPlayerState.generated.h"

enum class ESpartaArcadeCharacterType : uint8;

UCLASS()
class SPARTAARCADE_API ALobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void CopyProperties(APlayerState* PlayerState) override;

public:
	ALobbyPlayerState();
	ESpartaArcadeCharacterType GetSelectedCharacterType() const;
	bool GetIsReady() const;
	int32 GetTeamID() const;

	void SetSelectedCharacterType(ESpartaArcadeCharacterType NewCharacterType);
	void SetIsReady(bool bNewReady);
	void SetTeamID(int32 NewTeamID);

	UFUNCTION()
	void OnRep_LobbyStateChanged();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_LobbyStateChanged)
	ESpartaArcadeCharacterType SelectedCharacterType;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyStateChanged)
	bool bIsReady;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyStateChanged)
	int32 TeamID;
};
