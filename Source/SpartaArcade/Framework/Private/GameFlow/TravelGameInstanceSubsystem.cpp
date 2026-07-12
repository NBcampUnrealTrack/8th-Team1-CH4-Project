// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFlow/TravelGameInstanceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFlow/MapSettings.h"


void UTravelGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UMapSettings* MapSettings = GetDefault<UMapSettings>();
	if (IsValid(MapSettings))
	{
		TitleMap = MapSettings->TitleMap;
		LobbyMap = MapSettings->LobbyMap;
		InGameMap = MapSettings->InGameMap;
	}
}

void UTravelGameInstanceSubsystem::TravelToTitleMap()
{
	if (TitleMap.IsNull() == false)
	{
		FString MapPath = TitleMap.GetLongPackageName();
		APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
		if (IsValid(PlayerController))
		{
			PlayerController->ClientTravel(MapPath, ETravelType::TRAVEL_Absolute);
		}
	}
}

void UTravelGameInstanceSubsystem::TravelToLobbyMap()
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		if (LobbyMap.IsNull() == false)
		{
			FString MapPath = LobbyMap.GetLongPackageName();
			World->ServerTravel(MapPath + TEXT("?listen"));
		}
	}
}

void UTravelGameInstanceSubsystem::TravelToSession(const FString& Connect)
{
	APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
	if(IsValid(PlayerController))
	{
		PlayerController->ClientTravel(Connect, ETravelType::TRAVEL_Absolute);
	}
}

void UTravelGameInstanceSubsystem::TravelToInGameMap()
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		if (InGameMap.IsNull() == false)
		{
			FString MapPath = InGameMap.GetLongPackageName();

			World->ServerTravel(MapPath + TEXT("?listen"));
		}
	}
}