#include "Title/UW_TitleUserWidget.h"
#include "SpartaButton.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Title/TitlePlayerController.h"
#include "EOSGameInstanceSubsystem.h"
#include "SessionService.h"
#include "AuthService.h"
#include "Components/CheckBox.h"
#include "SpartaUIDefs.h"
#include "Components/VerticalBox.h"
#include "OnlineSessionSettings.h"
#include "Components/TextBlock.h"
#include "Title/SpartaSessionEntryWidget.h"

UUW_TitleUserWidget::UUW_TitleUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UUW_TitleUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (IsValid(PlayButton) == true)
	{
		PlayButton->OnClicked.AddDynamic(this, &UUW_TitleUserWidget::OnPlayButtonClicked);
	}
	if (IsValid(ExitButton) == true)
	{
		ExitButton->OnClicked.AddDynamic(this, &UUW_TitleUserWidget::OnExitButtonClicked);
	}
	if(IsValid(SettingSessionButton) == true)
	{
		SettingSessionButton->OnClicked.AddDynamic(this, &UUW_TitleUserWidget::OnSettingSessionClicked);
	}
	if(IsValid(SearchSessionButton) == true)
	{
		SearchSessionButton->OnClicked.AddDynamic(this, &UUW_TitleUserWidget::OnSearchSessionButtonClicked);
	}
	if(IsValid(LoginButton) == true)
	{
		LoginButton->OnClicked.AddDynamic(this, &UUW_TitleUserWidget::OnLoginButtonClicked);
	}
	if(IsValid(SoloModeButton) == true)
	{
		SoloModeButton->OnClicked.AddDynamic(this, &UUW_TitleUserWidget::OnSoloModeButtonClicked);
	}
	if(IsValid(TeamModeButton) == true)
	{
		TeamModeButton->OnClicked.AddDynamic(this, &UUW_TitleUserWidget::OnTeamModeButtonClicked);
	}
	if(IsValid(CreateSessionButton) == true)
	{
		CreateSessionButton->OnClicked.AddDynamic(this, &UUW_TitleUserWidget::OnCreateSessionButtonClicked);
	}
	if(UGameInstance* GameInstance = GetGameInstance())
	{
		EOSGameInstanceSubsystem = GameInstance->GetSubsystem<UEOSGameInstanceSubsystem>();
		if(EOSGameInstanceSubsystem)
		{
			EOSGameInstanceSubsystem->GetSessionService()->OnSearchSessionCompleteEvent.AddUObject(this, &UUW_TitleUserWidget::HandleSearchSessionComplete);
		}
	}
}

void UUW_TitleUserWidget::OnPlayButtonClicked()
{
	ATitlePlayerController* TitlePlayerController = Cast<ATitlePlayerController>(GetOwningPlayer());
	if(IsValid(TitlePlayerController) == true)
	{
		if (IsValid(ServerIPEditableText) == true)
		{
			FString ServerIP = ServerIPEditableText->GetText().ToString();
			if (ServerIP.IsEmpty() == false)
			{
				FString PlayerName = TEXT("Player");	
				FString PlayerNameInput = PlayerNameEditableText->GetText().ToString();
				if (PlayerNameInput.IsEmpty() == false)
				{
					PlayerName = PlayerNameInput;
				}
				TitlePlayerController->JoinServer(ServerIP, PlayerName);
			}
		}
	}
}

void UUW_TitleUserWidget::OnExitButtonClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UUW_TitleUserWidget::OnSettingSessionClicked()
{
	if(!IsValid(SessionNameEditableText) || !IsValid(MaxPlayerEditableText) || !IsValid(IsPrivateCheckBox) || !IsValid(CreateSessionButton) || !IsValid(SoloModeButton) || !IsValid(TeamModeButton))
	{
		return;
	}

	if (SessionNameEditableText->GetVisibility() == ESlateVisibility::Visible)
	{
		SessionNameEditableText->SetVisibility(ESlateVisibility::Hidden);
		MaxPlayerEditableText->SetVisibility(ESlateVisibility::Hidden);
		IsPrivateCheckBox->SetVisibility(ESlateVisibility::Hidden);	
		SoloModeButton->SetVisibility(ESlateVisibility::Hidden);
		TeamModeButton->SetVisibility(ESlateVisibility::Hidden);
		CreateSessionButton->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{		
		SessionNameEditableText->SetVisibility(ESlateVisibility::Visible);
		MaxPlayerEditableText->SetVisibility(ESlateVisibility::Visible);
		IsPrivateCheckBox->SetVisibility(ESlateVisibility::Visible);
		SoloModeButton->SetVisibility(ESlateVisibility::Visible);
		TeamModeButton->SetVisibility(ESlateVisibility::Visible);
		CreateSessionButton->SetVisibility(ESlateVisibility::Visible);

		if(EOSGameInstanceSubsystem)
		{
			SessionNameEditableText->SetText(FText::FromString(EOSGameInstanceSubsystem->GetAuthService()->GetDisplayName() + TEXT("'s Session")));
		}
		else
		{
			SessionNameEditableText->SetText(FText::FromString(TEXT("Session")));
		}
		MaxPlayerEditableText->SetText(FText::FromString(TEXT("4")));
		IsPrivateCheckBox->SetIsChecked(false);
		SoloModeButton->SetIsEnabled(false);
		TeamModeButton->SetIsEnabled(true);
	}
}


void UUW_TitleUserWidget::OnSearchSessionButtonClicked()
{
	if (IsValid(SessionListVerticalBox))
	{
		if (SessionListVerticalBox->GetVisibility() == ESlateVisibility::Hidden)
		{
			if (EOSGameInstanceSubsystem)
			{
				SessionListVerticalBox->SetVisibility(ESlateVisibility::Visible);
				EOSGameInstanceSubsystem->GetSessionService()->FindSessions();
			}
		}
		else
		{
			SessionListVerticalBox->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UUW_TitleUserWidget::OnLoginButtonClicked()
{
	if (IsValid(PlayerNameEditableText) == true)
	{
		FString PlayerName = TEXT("Player");
		FString PlayerNameInput = PlayerNameEditableText->GetText().ToString();
		if (PlayerNameInput.IsEmpty() == false)
		{
			PlayerName = PlayerNameInput;
		}
		if (EOSGameInstanceSubsystem)
		{
			EOSGameInstanceSubsystem->GetAuthService()->Login(PlayerName);
		}
	}
}

void UUW_TitleUserWidget::OnSoloModeButtonClicked()
{
	SoloModeButton->SetIsEnabled(false);
	TeamModeButton->SetIsEnabled(true);
}

void UUW_TitleUserWidget::OnTeamModeButtonClicked()
{
	TeamModeButton->SetIsEnabled(false);
	SoloModeButton->SetIsEnabled(true);
}

void UUW_TitleUserWidget::OnCreateSessionButtonClicked()
{
	if (EOSGameInstanceSubsystem)
	{
		FSessionInfo CreationSettings;
		CreationSettings.SessionName = SessionNameEditableText->GetText().ToString();
		CreationSettings.MaxPlayers = FCString::Atoi(*MaxPlayerEditableText->GetText().ToString());
		CreationSettings.bIsPrivate = (IsPrivateCheckBox->GetCheckedState() == ECheckBoxState::Checked);
		CreationSettings.GameModeType = SoloModeButton->bIsEnabled ? static_cast<int32>(EGameModeType::Team) : static_cast<int32>(EGameModeType::Solo);

		EOSGameInstanceSubsystem->GetSessionService()->CreateSession(CreationSettings);
	}
}

void UUW_TitleUserWidget::HandleSearchSessionComplete(bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResults)
{
	if (bWasSuccessful)
	{
		UpdateSessionList(SearchResults);
	}
}

void UUW_TitleUserWidget::UpdateSessionList(const TArray<FOnlineSessionSearchResult>& SearchResults)
{
	if(SessionEntryWidgetClass == nullptr || SessionListVerticalBox == nullptr || EOSGameInstanceSubsystem == nullptr)
	{
		return;
	}

	SessionListVerticalBox->ClearChildren();

	for (const FOnlineSessionSearchResult& Result : SearchResults)
	{
		USpartaSessionEntryWidget* SessionEntryWidget = CreateWidget<USpartaSessionEntryWidget>(GetWorld(), SessionEntryWidgetClass);
		if (SessionEntryWidget)
		{
			SessionEntryWidget->InitializeSessionEntry(Result);
			SessionListVerticalBox->AddChild(SessionEntryWidget);
		}
	}
}