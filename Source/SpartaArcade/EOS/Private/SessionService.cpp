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
	InviteSession = nullptr;
	bPendingJoinAfterDestroy = false;

	if(Session.IsValid())
	{
		Session->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &USessionService::OnCreateSessionComplete));
		Session->AddOnStartSessionCompleteDelegate_Handle(FOnStartSessionCompleteDelegate::CreateUObject(this, &USessionService::OnStartSessionComplete));
		//Session->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &USessionService::OnDestroySessionComplete));
		SessionDestroyComplete.BindUObject(this, &USessionService::OnDestroySessionComplete);
		Session->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &USessionService::OnFindSessionsComplete));
		Session->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &USessionService::OnJoinSessionComplete));
		Session->AddOnSessionUserInviteAcceptedDelegate_Handle(FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &USessionService::OnSessionInviteAccepted));
	}
	if(GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &USessionService::HandleNetworkFailure);
	}
}

void USessionService::CreateSession(FSessionInfo CreationSettings)
{
	if (!Session.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionService] Session Interface가 유효하지 않아 방 생성 실패!"));
		return;
	}

	if (Session->GetNamedSession(NAME_GameSession) != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionService] 기존 세션이 발견되어 세션 파괴를 요청합니다."));
		Session->DestroySession(NAME_GameSession);
	}

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = false;
	Settings.NumPublicConnections = CreationSettings.MaxPlayers;
	Settings.bShouldAdvertise = !CreationSettings.bIsPrivate;
	Settings.bUsesPresence = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bAllowInvites = true;
	Settings.Set(SEARCH_KEYWORDS, FString("SpartaArcade"), EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(SessionKeys::SessionName, CreationSettings.SessionName, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(SessionKeys::GameMode, FString::FromInt(CreationSettings.GameModeType), EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(SessionKeys::Private, CreationSettings.bIsPrivate, EOnlineDataAdvertisementType::ViaOnlineService);
	
	bool bResult = Session->CreateSession(0, NAME_GameSession, Settings);
	UE_LOG(LogTemp, Warning, TEXT("[SessionService] CreateSession 호출 결과: %s"), bResult ? TEXT("True") : TEXT("False"));
}

void USessionService::DestroySession()
{
	if (!Session.IsValid())
	{
		return;
	}

	// EOSPlus 에서는 해당 함수로 세션을 삭제하면, 델리게이트 OnDestroySessionComplete가 호출되지 않음. 
	// 따라서, 델리게이트를 직접 호출하도록 수정함.
	//Session->DestroySession(NAME_GameSession);
	Session->DestroySession(NAME_GameSession, SessionDestroyComplete);
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
		Session->StartSession(SessionName);
	}
	else
	{
		// Modified: 온라인(EOS/Steam) 세션 생성이 로그인 미완료/네트워크 이유로 실패할 경우 LAN 세션으로 Fallback 시도하여 무반응 방지
		UE_LOG(LogTemp, Warning, TEXT("[SessionService] 온라인 세션 생성 실패 -> LAN 세션으로 재시도합니다."));
		FOnlineSessionSettings Settings;
		Settings.bIsLANMatch = true;
		Settings.NumPublicConnections = 4;
		Settings.bShouldAdvertise = true;
		Settings.bUsesPresence = true;
		Settings.bAllowJoinViaPresence = true;
		Settings.bUseLobbiesIfAvailable = false;
		Settings.bAllowInvites = true;

		Session->CreateSession(0, SessionName, Settings);
	}
}

void USessionService::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("Session start callback: %s (Success: %d)"), *SessionName.ToString(), bWasSuccessful);
	// Modified: 세션 시작 성공 여부와 관계없이 호스트로서 로비 맵으로 전환되도록 델리게이트 알림
	OnStartSessionCompleteEvent.Broadcast(SessionName, true);
}

void USessionService::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("Session destroyed successfully: %s"), *SessionName.ToString());
		OnDestroySessionCompleteEvent.Broadcast(SessionName, bWasSuccessful);
		if (bPendingJoinAfterDestroy && InviteSession.IsValid())
		{
			JoinSession(*InviteSession);
			InviteSession = nullptr;
			bPendingJoinAfterDestroy = false;
		}
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

void USessionService::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if(FailureType == ENetworkFailure::ConnectionLost || FailureType == ENetworkFailure::FailureReceived)
	{
		UE_LOG(LogTemp, Error, TEXT("Network failure detected: %s"), *ErrorString);
		DestroySession();
	}
}

void USessionService::OnSessionInviteAccepted(const bool bWasSuccessful, int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (bWasSuccessful && InviteResult.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Session invite accepted."));
		InviteSession = MakeShared<FOnlineSessionSearchResult>(InviteResult);
		if(Session.IsValid())
		{
			if(Session->GetNamedSession(NAME_GameSession))
			{
				Session->DestroySession(NAME_GameSession);
				bPendingJoinAfterDestroy = true;
			}
			else
			{
				JoinSession(InviteResult);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to accept session invite."));
	}
}