// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class SPARTAARCADE_API ALobbyGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ALobbyGameModeBase();

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void BeginPlay() override;

	void InitializeLobbyGameState();

	void OnPlayerReadyStateChanged();

	void StartInGameMatch();

	bool IsCanStartMatch() const;

	void UpdateMatchStartCountdown();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	int32 StartCountdownTimeRemaining;

	FTimerHandle StartCountdownTimerHandle;
};
