// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SpartaUIDefs.h"
#include "ResultGameInstanceSubsystem.generated.h"

UENUM(BlueprintType)
enum class ESessionEndReason : uint8
{
    None                UMETA(DisplayName = "None"),
    GameComplete        UMETA(DisplayName = "Game Complete"),
    LobbyCollapsed      UMETA(DisplayName = "Lobby Collapsed"),
    Disconnected        UMETA(DisplayName = "Disconnected") 
};

UCLASS()
class SPARTAARCADE_API UResultGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void SetGameResult(ESessionEndReason InEndReason, const FString& InErrorMessage, const FMatchPlayerResult& InGameResult);

    ESessionEndReason GetEndReason() const;

    const FString& GetErrorMessage() const;

    const FMatchPlayerResult& GetGameResult() const;

    void ResetGameResult();

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Game Result")
    ESessionEndReason EndReason = ESessionEndReason::None;

    UPROPERTY(BlueprintReadOnly, Category = "Game Result")
    FString ErrorMessage;

    UPROPERTY(BlueprintReadOnly, Category = "Game Result")
    FMatchPlayerResult GameResult;
};
