// Fill out your copyright notice in the Description page of Project Settings.


#include "SessionService.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"

void USessionService::Initialize(IOnlineSubsystem* InOnlineSubsystem)
{
	Session = InOnlineSubsystem->GetSessionInterface();

	if(Session.IsValid())
	{
		Session->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &USessionService::OnCreateSessionComplete));
		Session->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &USessionService::OnDestroySessionComplete));
		Session->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &USessionService::OnFindSessionsComplete));
		Session->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &USessionService::OnJoinSessionComplete));
	}
}

void USessionService::CreateSession()
{
	if (!Session.IsValid())
	{
		return;
	}
	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = false;
	Settings.NumPublicConnections = 4;
	Settings.bShouldAdvertise = true;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.Set(SEARCH_KEYWORDS, FString("MyGame"), EOnlineDataAdvertisementType::ViaOnlineService);

	Session->CreateSession(0, NAME_GameSession, Settings);
}

void USessionService::DestroySession()
{
	if (!Session.IsValid())
	{
		return;
	}
	Session->DestroySession(NAME_GameSession);
}

void USessionService::FindSessions()
{
	Search = MakeShared<FOnlineSessionSearch>();
	Search->bIsLanQuery = false;
	Search->MaxSearchResults = 20;
	Search->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	Search->QuerySettings.Set(SEARCH_KEYWORDS, FString(TEXT("MyGame")), EOnlineComparisonOp::Equals);

	Session->FindSessions(0, Search.ToSharedRef());
}

void USessionService::JoinSession(const FOnlineSessionSearchResult& Result)
{
	Session->JoinSession(0, NAME_GameSession, Result);
}

// --------------------------------------------------------------
// 콜백 함수

void USessionService::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("Session created successfully: %s"), *SessionName.ToString());
		GetWorld()->ServerTravel(TEXT("/Game/NetworkTemp/Map/LobbyMap?listen"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create session: %s"), *SessionName.ToString());
	}
}

void USessionService::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("Session destroyed successfully: %s"), *SessionName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to destroy session: %s"), *SessionName.ToString());
	}
}

void USessionService::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("Sessions found successfully."));
		JoinSession(Search->SearchResults[0]);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find sessions."));
	}
}

void USessionService::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Log, TEXT("Joined session successfully: %s"), *SessionName.ToString());
		FString Connect;
		if (Session->GetResolvedConnectString(NAME_GameSession, Connect))
		{
			GetWorld()->GetFirstPlayerController()->ClientTravel(Connect, TRAVEL_Absolute);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to join session: %s"), *SessionName.ToString());
	}
}