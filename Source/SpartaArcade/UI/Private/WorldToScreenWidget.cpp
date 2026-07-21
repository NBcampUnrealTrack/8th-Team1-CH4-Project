// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Public/WorldToScreenWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/TextBlock.h"

void UWorldToScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (!bIsInitialized)
	{
		return;
	}

	if (bIsInitialized && !IsValid(AttachedActor))
	{
		if (IsInViewport())
		{
			UE_LOG(LogTemp, Log, TEXT("Removing World to Screen Widget %s from %s"), *GetName(), *GetNameSafe(AttachedActor))
			RemoveFromParent();
		}
		return;
	}

	if (!IsValid(AttachedActor) || !IsValid(PlayerController))
	{
		return;
	}
	
	APawn* LocalPawn = PlayerController->GetPawn();
	const float MaxDistance = 3000.f;
	const float DistanceSquared = IsValid(LocalPawn) ? FVector::DistSquared(LocalPawn->GetActorLocation(), AttachedActor->GetActorLocation()) : 0.f;
	if (DistanceSquared > FMath::Square(MaxDistance))
	{
		SetRenderTranslation(FVector2D(-10000.f, -10000.f));
		return;
	}

	const FVector ActorLocation = AttachedActor->GetActorLocation();
	const FVector WorldLocation(ActorLocation.X, ActorLocation.Y, ActorLocation.Z + HeightOffset);
	FVector2D OutScreenPos;
	const bool bIsOnScreen = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, WorldLocation, OutScreenPos, false);

	if (bIsOnScreen)
	{
		const FVector2D Size = GetDesiredSize();
		const FVector2D CenteredTranslation(OutScreenPos.X - Size.X * 0.5f, OutScreenPos.Y);
		SetRenderTranslation(CenteredTranslation);
	}
}

void UWorldToScreenWidget::SetNickname(const FString& Nickname, int32 TeamID, APlayerController* InPlayerController)
{
	if (!IsValid(PlayerNameText))
	{
		return;
	}

	PlayerController = InPlayerController;

	PlayerNameText->SetText(FText::FromString(Nickname));

	FLinearColor TeamColor = FLinearColor::White;
	if (TeamID == 1)
	{
		TeamColor = FLinearColor(1.0f, 0.25f, 0.25f);
	}
	else if (TeamID == 2)
	{
		TeamColor = FLinearColor(0.25f, 0.5f, 1.0f);
	}

	PlayerNameText->SetColorAndOpacity(FSlateColor(TeamColor));
	bIsInitialized = true;
}