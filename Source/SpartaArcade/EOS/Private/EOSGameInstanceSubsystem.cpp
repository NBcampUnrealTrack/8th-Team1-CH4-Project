// Fill out your copyright notice in the Description page of Project Settings.


#include "EOSGameInstanceSubsystem.h"
#include "AuthService.h"
#include "SessionService.h"
#include "OnlineSubsystem.h"
#include "GameFlow/TravelGameInstanceSubsystem.h"
void UEOSGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AuthService = NewObject<UAuthService>(this);
	SessionService = NewObject<USessionService>(this);
	OnlineSubsystem = IOnlineSubsystem::Get();

	if(!OnlineSubsystem)
	{
		return;
	}

	AuthService->Initialize(OnlineSubsystem);
	SessionService->Initialize(OnlineSubsystem);
}

void UEOSGameInstanceSubsystem::Deinitialize()
{
	Super::Deinitialize();

	AuthService->Logout();

	AuthService = nullptr;
	SessionService = nullptr;
}

UAuthService* UEOSGameInstanceSubsystem::GetAuthService() const
{
	return AuthService;
}

USessionService* UEOSGameInstanceSubsystem::GetSessionService() const
{
	return SessionService;
}