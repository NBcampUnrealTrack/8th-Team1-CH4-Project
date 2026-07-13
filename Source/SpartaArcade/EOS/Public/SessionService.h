// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SpartaUIDefs.h"
#include "SessionService.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCreateSessionCompleteEvent, FName, bool);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSearchSessionCompleteEvent, bool, const TArray<FOnlineSessionSearchResult>&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnJoinSessionCompleteEvent, FName, EOnJoinSessionCompleteResult::Type, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDestroySessionCompleteEvent, FName, bool);

namespace SessionKeys
{
	static const FName SessionName = TEXT("SESSION_NAME");
	static const FName GameMode = TEXT("GAME_MODE");
	static const FName Private = TEXT("PRIVATE");
}

USTRUCT(BlueprintType)
struct FSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SessionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentPlayers = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxPlayers = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsPrivate = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 GameModeType = static_cast<int32>(EGameModeType::Solo);
};


class IOnlineSubsystem;

UCLASS()
class SPARTAARCADE_API USessionService : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(IOnlineSubsystem* InOnlineSubsystem);

	void CreateSession(FSessionInfo CreationSettings);

	void DestroySession();

	void FindSessions();

	void JoinSession(const FOnlineSessionSearchResult& Result);

	FSessionInfo MakeSessionInfo(const FOnlineSessionSearchResult& Result);

	FSessionInfo GetCurrentSessionInfo() const;

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	void OnFindSessionsComplete(bool bWasSuccessful);

	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

private:
	IOnlineSessionPtr Session;

	TSharedPtr<FOnlineSessionSearch> Search;

public:
	FOnCreateSessionCompleteEvent OnCreateSessionCompleteEvent;
	FOnSearchSessionCompleteEvent OnSearchSessionCompleteEvent;
	FOnJoinSessionCompleteEvent OnJoinSessionCompleteEvent;
	FOnDestroySessionCompleteEvent OnDestroySessionCompleteEvent;
};
