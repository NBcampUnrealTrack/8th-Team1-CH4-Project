// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "TitlePlayerController.generated.h"

class UUserWidget;

UCLASS()
class SPARTAARCADE_API ATitlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	void JoinServer(const FString& InIPAddress, const FString& InPlayerName);

	void HandleStartSessionComplete(FName SessionName, bool bWasSuccessful);

	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result, const FString& Connect);

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG
	UFUNCTION(Exec)
	void DevLogin(const FString& InAuthToken);
#endif

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = PlayerController, Meta = (AllowPrivateAccess))
	TSubclassOf<UUserWidget> UIWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = PlayerController, Meta = (AllowPrivateAccess))
	TObjectPtr<UUserWidget> UIWidgetInstance;
};
