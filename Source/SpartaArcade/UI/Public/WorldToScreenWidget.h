// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WorldToScreenWidget.generated.h"

class UTextBlock;

UCLASS()
class SPARTAARCADE_API UWorldToScreenWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	/** Can be adjusted in the child WBP or set by caller. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	float HeightOffset{ 130.f };

	/** Will normally be self during create widget. Is the Actor this widget will follow around. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, meta = (ExposeOnSpawn = true))
	TObjectPtr<AActor> AttachedActor;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetNickname(const FString& Nickname, int32 TeamID, APlayerController* InPlayerController);

protected:
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bIsInitialized{ false };
	
};
