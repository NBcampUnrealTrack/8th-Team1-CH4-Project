// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OnlineSessionSettings.h"
#include "SpartaSessionEntryWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class SPARTAARCADE_API USpartaSessionEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

    void InitializeSessionEntry(FOnlineSessionSearchResult SearchResult);

protected:

    UPROPERTY(meta = (BindWidget))
    UTextBlock* SessionNameText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* PlayerCountText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* GameModeText;

    UPROPERTY(meta = (BindWidget))
    UButton* JoinButton;

    UFUNCTION()
    void OnJoinClicked();

private:
    FOnlineSessionSearchResult Result;

};
