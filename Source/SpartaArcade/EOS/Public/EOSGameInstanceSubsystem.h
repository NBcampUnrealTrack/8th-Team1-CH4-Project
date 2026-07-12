// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EOSGameInstanceSubsystem.generated.h"

class UAuthService;
class USessionService;
class IOnlineSubsystem;

UCLASS()
class SPARTAARCADE_API UEOSGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UAuthService* GetAuthService() const;
	USessionService* GetSessionService() const;


private:
	IOnlineSubsystem* OnlineSubsystem;

	UPROPERTY()
	TObjectPtr<UAuthService> AuthService;

	UPROPERTY()
	TObjectPtr<USessionService> SessionService;
	
};
