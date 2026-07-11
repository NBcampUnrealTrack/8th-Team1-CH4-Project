// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SessionService.generated.h"

class IOnlineSubsystem;

UCLASS()
class SPARTAARCADE_API USessionService : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(IOnlineSubsystem* InOnlineSubsystem);

	void CreateSession();

	void DestroySession();

	void FindSessions();

	void JoinSession(const FOnlineSessionSearchResult& Result);

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	void OnFindSessionsComplete(bool bWasSuccessful);

	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

private:
	IOnlineSessionPtr Session;

	TSharedPtr<FOnlineSessionSearch> Search;
};
