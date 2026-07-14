#include "UW_TitleUserWidget.h"
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
#include "SpartaMenuFlowWidget.h"
#include "Components/TextBlock.h"
#include "SpartaSessionEntryWidget.h"
#include "SpartaUIManagerSubsystem.h"
#include "Components/Widget.h"
#include "UObject/UObjectIterator.h"

UUW_TitleUserWidget::UUW_TitleUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UUW_TitleUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
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
	if(IsValid(MaxPlayerEditableText) == true)
	{
		MaxPlayerEditableText->OnTextCommitted.AddDynamic(this, &UUW_TitleUserWidget::OnMaxPlayerTextCommitted);
	}
	if(UGameInstance* GameInstance = GetGameInstance())
	{
		EOSGameInstanceSubsystem = GameInstance->GetSubsystem<UEOSGameInstanceSubsystem>();
		if(EOSGameInstanceSubsystem)
		{
			EOSGameInstanceSubsystem->GetSessionService()->OnSearchSessionCompleteEvent.AddUObject(this, &UUW_TitleUserWidget::HandleSearchSessionComplete);
		}
	}

	// 기본 상태로 방 만들기 및 참가 오버레이를 안 보이게 설정
	if (IsValid(MakeRoomOverlay))
	{
		MakeRoomOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(SessionOverlay))
	{
		SessionOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UUW_TitleUserWidget::OnExitButtonClicked()
{
	for (TObjectIterator<USpartaMenuFlowWidget> It; It; ++It)
	{
		if (It->GetWorld() == GetWorld())
		{
			It->ShowMainMenu();
			return;
		}
	}
}

void UUW_TitleUserWidget::OnSettingSessionClicked()
{
	if(!IsValid(SessionNameEditableText) || !IsValid(MaxPlayerEditableText) || !IsValid(IsPrivateCheckBox) || !IsValid(CreateSessionButton) || !IsValid(SoloModeButton) || !IsValid(TeamModeButton))
	{
		return;
	}

	// 방 만들기 버튼(OnSettingSessionClicked) 클릭 시, MakeRoomOverlay 가시성 제어 및 SessionOverlay 비활성화
	if (IsValid(SessionOverlay))
	{
		SessionOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(SessionListVerticalBox))
	{
		SessionListVerticalBox->SetVisibility(ESlateVisibility::Hidden);
	}

	if (IsValid(MakeRoomOverlay))
	{
		if (MakeRoomOverlay->GetVisibility() == ESlateVisibility::Visible)
		{
			MakeRoomOverlay->SetVisibility(ESlateVisibility::Collapsed);
			
			SessionNameEditableText->SetVisibility(ESlateVisibility::Hidden);
			MaxPlayerEditableText->SetVisibility(ESlateVisibility::Hidden);
			IsPrivateCheckBox->SetVisibility(ESlateVisibility::Hidden);	
			SoloModeButton->SetVisibility(ESlateVisibility::Hidden);
			TeamModeButton->SetVisibility(ESlateVisibility::Hidden);
			CreateSessionButton->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			MakeRoomOverlay->SetVisibility(ESlateVisibility::Visible);

			SessionNameEditableText->SetVisibility(ESlateVisibility::Visible);
			MaxPlayerEditableText->SetVisibility(ESlateVisibility::Visible);
			IsPrivateCheckBox->SetVisibility(ESlateVisibility::Visible);
			SoloModeButton->SetVisibility(ESlateVisibility::Visible);
			TeamModeButton->SetVisibility(ESlateVisibility::Visible);
			CreateSessionButton->SetVisibility(ESlateVisibility::Visible);

			// if(EOSGameInstanceSubsystem)
			// {
			// 	SessionNameEditableText->SetText(FText::FromString(EOSGameInstanceSubsystem->GetAuthService()->GetDisplayName() + TEXT("'s Session")));
			// }
			// else
			// {
			// 	SessionNameEditableText->SetText(FText::FromString(TEXT("Session")));
			// }
			// MaxPlayerEditableText->SetText(FText::FromString(TEXT("4")));
			IsPrivateCheckBox->SetIsChecked(false);
			SoloModeButton->SetIsEnabled(false);
			TeamModeButton->SetIsEnabled(true);
		}
	}
}

void UUW_TitleUserWidget::OnSearchSessionButtonClicked()
{
	// 참가하기 버튼(OnSearchSessionButtonClicked) 클릭 시, SessionOverlay 가시성 제어 및 MakeRoomOverlay 비활성화
	if (IsValid(MakeRoomOverlay))
	{
		MakeRoomOverlay->SetVisibility(ESlateVisibility::Collapsed);
		
		if (IsValid(SessionNameEditableText)) SessionNameEditableText->SetVisibility(ESlateVisibility::Hidden);
		if (IsValid(MaxPlayerEditableText)) MaxPlayerEditableText->SetVisibility(ESlateVisibility::Hidden);
		if (IsValid(IsPrivateCheckBox)) IsPrivateCheckBox->SetVisibility(ESlateVisibility::Hidden);
		if (IsValid(SoloModeButton)) SoloModeButton->SetVisibility(ESlateVisibility::Hidden);
		if (IsValid(TeamModeButton)) TeamModeButton->SetVisibility(ESlateVisibility::Hidden);
		if (IsValid(CreateSessionButton)) CreateSessionButton->SetVisibility(ESlateVisibility::Hidden);
	}

	if (IsValid(SessionOverlay))
	{
		if (SessionOverlay->GetVisibility() == ESlateVisibility::Visible)
		{
			SessionOverlay->SetVisibility(ESlateVisibility::Collapsed);
			if (IsValid(SessionListVerticalBox))
			{
				SessionListVerticalBox->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else
		{
			SessionOverlay->SetVisibility(ESlateVisibility::Visible);
			if (IsValid(SessionListVerticalBox))
			{
				if (EOSGameInstanceSubsystem)
				{
					SessionListVerticalBox->SetVisibility(ESlateVisibility::Visible);
					EOSGameInstanceSubsystem->GetSessionService()->FindSessions();
				}
			}
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
		
		int32 MaxPlayers = 4;
		FString MaxPlayersStr = MaxPlayerEditableText->GetText().ToString();
		if (!MaxPlayersStr.IsEmpty())
		{
			int32 ParsedValue = FCString::Atoi(*MaxPlayersStr);
			MaxPlayers = FMath::Clamp(ParsedValue, 2, 4);
		}
		CreationSettings.MaxPlayers = MaxPlayers;

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

void UUW_TitleUserWidget::OnMaxPlayerTextCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	if (IsValid(MaxPlayerEditableText))
	{
		FString InputStr = Text.ToString();
		
		// 문자열 전체가 숫자로만 구성되어 있는지 루프 검증
		bool bIsNumeric = !InputStr.IsEmpty();
		for (int32 i = 0; i < InputStr.Len(); ++i)
		{
			if (!FChar::IsDigit(InputStr[i]))
			{
				bIsNumeric = false;
				break;
			}
		}

		if (!bIsNumeric)
		{
			MaxPlayerEditableText->SetText(FText::GetEmpty());
		}
		else
		{
			int32 Value = FCString::Atoi(*InputStr);
			int32 ClampedValue = FMath::Clamp(Value, 2, 4);
			MaxPlayerEditableText->SetText(FText::AsNumber(ClampedValue));
		}
	}
}