// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/SpartaSessionEntryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "OnlineSessionSettings.h"
#include "EOSGameInstanceSubsystem.h"
#include "SessionService.h"

void USpartaSessionEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &USpartaSessionEntryWidget::OnJoinClicked);
	}
}

void USpartaSessionEntryWidget::InitializeSessionEntry(FOnlineSessionSearchResult SearchResult)
{
	Result = SearchResult;
	UEOSGameInstanceSubsystem* EOSGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UEOSGameInstanceSubsystem>();
	if (EOSGameInstanceSubsystem)
	{
		const FSessionInfo SessionInfo = EOSGameInstanceSubsystem->GetSessionService()->MakeSessionInfo(Result);

		if (SessionNameText)
		{
			SessionNameText->SetText(FText::FromString(SessionInfo.SessionName));
		}
		if (PlayerCountText)
		{
			PlayerCountText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), SessionInfo.CurrentPlayers, SessionInfo.MaxPlayers)));
		}
		if (GameModeText)
		{
			GameModeText->SetText(FText::FromString(StaticEnum<EGameModeType>()->GetNameStringByValue(SessionInfo.GameModeType)));
		}
	}
}

void USpartaSessionEntryWidget::OnJoinClicked()
{
	UEOSGameInstanceSubsystem* EOSGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UEOSGameInstanceSubsystem>();
	if (EOSGameInstanceSubsystem)
	{
		EOSGameInstanceSubsystem->GetSessionService()->JoinSession(Result);
	}
}