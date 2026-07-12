// Fill out your copyright notice in the Description page of Project Settings.


#include "AuthService.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"

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
		return;
	}

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG
	FOnlineAccountCredentials Credentials;
	Credentials.Type = TEXT("developer");
	Credentials.Id = TEXT("localhost:6300");
	Credentials.Token = AuthToken;
	Identity->Login(0, Credentials);
	return;
#else
	Identity->AutoLogin(0);
#endif;
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
		UE_LOG(LogTemp, Log, TEXT("Login successful for user: %s"), *UserId.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Login failed: %s"), *Error);
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