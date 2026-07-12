// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

enum class ESpartaArcadeCharacterType : uint8;

UCLASS()
class SPARTAARCADE_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
    virtual void BeginPlay() override;

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSelectCharacter(ESpartaArcadeCharacterType NewType);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerToggleReady();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStartMatch();

	void LeaveLobby();

    void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = PlayerController, Meta = (AllowPrivateAccess))
    TSubclassOf<UUserWidget> LobbyUIWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = PlayerController, Meta = (AllowPrivateAccess))
    TObjectPtr<UUserWidget> LobbyUIWidgetInstance;
};
