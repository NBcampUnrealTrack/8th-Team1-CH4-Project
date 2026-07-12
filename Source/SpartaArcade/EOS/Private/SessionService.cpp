// Fill out your copyright notice in the Description page of Project Settings.


#include "SessionService.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "SpartaUIDefs.h"
#include "GameFlow/TravelGameInstanceSubsystem.h"
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

void USessionService::CreateSession(FSessionInfo CreationSettings)
{
	if (!Session.IsValid())
	{
		return;
	}
	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = false;
	Settings.NumPublicConnections = CreationSettings.MaxPlayers;
	Settings.bShouldAdvertise = !CreationSettings.bIsPrivate;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.Set(SEARCH_KEYWORDS, FString("SpartaArcade"), EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(SessionKeys::SessionName, CreationSettings.SessionName, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(SessionKeys::GameMode, FString::FromInt(CreationSettings.GameModeType), EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(SessionKeys::Private, CreationSettings.bIsPrivate, EOnlineDataAdvertisementType::ViaOnlineService);
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
	Search->QuerySettings.Set(SEARCH_KEYWORDS, FString(TEXT("SpartaArcade")), EOnlineComparisonOp::Equals);
	Search->QuerySettings.Set(SessionKeys::Private, false, EOnlineComparisonOp::Equals);

	Session->FindSessions(0, Search.ToSharedRef());
}

void USessionService::JoinSession(const FOnlineSessionSearchResult& Result)
{
	Session->JoinSession(0, NAME_GameSession, Result);
}

FSessionInfo USessionService::MakeSessionInfo(const FOnlineSessionSearchResult& Result)
{
	FSessionInfo SessionInfo; 
	FString GameModeString;
	Result.Session.SessionSettings.Get(SessionKeys::SessionName, SessionInfo.SessionName);
	Result.Session.SessionSettings.Get(SessionKeys::GameMode, GameModeString);
	SessionInfo.GameModeType = FCString::Atoi(*GameModeString);
	SessionInfo.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
	SessionInfo.CurrentPlayers = SessionInfo.MaxPlayers - Result.Session.NumOpenPublicConnections;
	return SessionInfo;
}

FSessionInfo USessionService::GetCurrentSessionInfo() const
{
	FSessionInfo SessionInfo;
	if (Session.IsValid())
	{
		FNamedOnlineSession* CurrentSession = Session->GetNamedSession(NAME_GameSession);
		if (CurrentSession)
		{
			FString GameModeString;
			CurrentSession->SessionSettings.Get(SessionKeys::SessionName, SessionInfo.SessionName);
			CurrentSession->SessionSettings.Get(SessionKeys::GameMode, GameModeString);
			SessionInfo.GameModeType = FCString::Atoi(*GameModeString);
			SessionInfo.MaxPlayers = CurrentSession->SessionSettings.NumPublicConnections;
			SessionInfo.CurrentPlayers = SessionInfo.MaxPlayers - CurrentSession->NumOpenPublicConnections;
		}
	}
	return SessionInfo;
}

// --------------------------------------------------------------
// 콜백 함수

void USessionService::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("Session created successfully: %s"), *SessionName.ToString());
		OnCreateSessionCompleteEvent.Broadcast(SessionName, bWasSuccessful);
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
		OnDestroySessionCompleteEvent.Broadcast(SessionName, bWasSuccessful);
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
		OnSearchSessionCompleteEvent.Broadcast(bWasSuccessful, Search->SearchResults);
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
		if(Session->GetResolvedConnectString(SessionName, Connect))
		{
			OnJoinSessionCompleteEvent.Broadcast(SessionName, Result, Connect);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to join session: %s"), *SessionName.ToString());
	}
}