// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFlow/ResultGameInstanceSubsystem.h"

void UResultGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResetGameResult();
}

void UResultGameInstanceSubsystem::SetGameResult(ESessionEndReason InEndReason, const FString& InErrorMessage, const FMatchPlayerResult& InGameResult)
{
	EndReason = InEndReason;
	ErrorMessage = InErrorMessage;
	GameResult = InGameResult;
}

ESessionEndReason UResultGameInstanceSubsystem::GetEndReason() const
{
	return EndReason;
}

const FString& UResultGameInstanceSubsystem::GetErrorMessage() const
{
	return ErrorMessage;
}

const FMatchPlayerResult& UResultGameInstanceSubsystem::GetGameResult() const
{
	return GameResult;
}

void UResultGameInstanceSubsystem::ResetGameResult()
{
	EndReason = ESessionEndReason::None;
	ErrorMessage = TEXT("");
	GameResult = FMatchPlayerResult();
}