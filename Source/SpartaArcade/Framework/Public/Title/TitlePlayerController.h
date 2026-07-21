// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "TitlePlayerController.generated.h"

class USpartaMenuFlowWidget;
class USoundBase;

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
	// 타이틀 화면 진입 시 재생할 배경음악
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sound", Meta = (AllowPrivateAccess))
	TObjectPtr<USoundBase> LevelBGM;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = PlayerController, Meta = (AllowPrivateAccess))
	TSubclassOf<USpartaMenuFlowWidget> UIWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = PlayerController, Meta = (AllowPrivateAccess))
	TObjectPtr<USpartaMenuFlowWidget> UIWidgetInstance;
};
