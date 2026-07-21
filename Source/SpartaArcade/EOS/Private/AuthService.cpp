// Fill out your copyright notice in the Description page of Project Settings.


#include "AuthService.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemUtils.h"

void UAuthService::Initialize(IOnlineSubsystem* InOnlineSubsystem)
{
	Identity = InOnlineSubsystem->GetIdentityInterface();

	if(Identity.IsValid())
	{
		Identity->AddOnLoginCompleteDelegate_Handle(0, FOnLoginCompleteDelegate::CreateUObject(this, &UAuthService::OnLoginComplete));
		Identity->AddOnLogoutCompleteDelegate_Handle(0, FOnLogoutCompleteDelegate::CreateUObject(this, &UAuthService::OnLogoutComplete));
	}

}

void UAuthService::Login(const FString& AuthToken)
{
	if (!Identity.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[AuthService] Identity Interface가 유효하지 않아 로그인을 진행할 수 없습니다."));
		return;
	}

#if WITH_EDITOR || UE_BUILD_DEBUG
	if (!AuthToken.IsEmpty())
	{
		FOnlineAccountCredentials Credentials;
		Credentials.Type = TEXT("developer");
		Credentials.Id = TEXT("localhost:6300");
		Credentials.Token = AuthToken;

		UE_LOG(LogTemp, Log, TEXT("[AuthService] Developer Auth Tool 로그인 시도 (%s)"), *AuthToken);
		Identity->Login(0, Credentials);
		return;
	}
#endif

	UE_LOG(LogTemp, Log, TEXT("[AuthService] EOS / EOSPlus AutoLogin 시도..."));
	if (!Identity->AutoLogin(0))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AuthService] AutoLogin(0) 실패 -> persistentauth 로그인 재시도"));
		FOnlineAccountCredentials Credentials;
		Credentials.Type = TEXT("persistentauth");
		Credentials.Id = TEXT("");
		Credentials.Token = TEXT("");
		Identity->Login(0, Credentials);
	}
}

void UAuthService::Logout()
{
	if (!Identity.IsValid())
	{
		return;
	}
	Identity->Logout(0);
}

FString UAuthService::GetDisplayName() const
{
	if (!Identity.IsValid())
	{
		return FString();
	}
	TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(0);
	if (UserId.IsValid())
	{
		return Identity->GetPlayerNickname(*UserId);
	}
	return FString();
}

// --------------------------------------------------------------
// 콜백 함수

void UAuthService::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AuthService] EOS/EOSPlus 로그인 성공! UserID: %s, DisplayName: %s"), *UserId.ToString(), *GetDisplayName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[AuthService] EOS 로그인 실패: %s"), *Error);
		
		if (Error.Contains(TEXT("persistentauth")) || Error.Contains(TEXT("credentials")) || Error.Contains(TEXT("NOT_FOUND")))
		{
			UE_LOG(LogTemp, Warning, TEXT("[AuthService] accountportal 에픽 로그인 포털 시도"));
			FOnlineAccountCredentials Credentials;
			Credentials.Type = TEXT("accountportal");
			Credentials.Id = TEXT("");
			Credentials.Token = TEXT("");
			Identity->Login(0, Credentials);
		}
	}
}

void UAuthService::OnLogoutComplete(int32 LocalUserNum, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("Logout successful for user: %d"), LocalUserNum);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Logout failed for user: %d"), LocalUserNum);
	}
}