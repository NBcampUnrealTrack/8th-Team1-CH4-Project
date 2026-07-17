#include "SpartaMenuFlowWidget.h"
#include "SpartaButton.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Characters/Public/SpartaArcadePlayerController.h"
#include "Framework/Public/Lobby/LobbyPlayerController.h"
#include "GameFlow/TravelGameInstanceSubsystem.h"

void USpartaMenuFlowWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 1) 메인 메뉴 이벤트 바인딩
    if (JoinButton)
    {
        JoinButton->OnClicked.AddDynamic(this, &USpartaMenuFlowWidget::OnJoinClicked);
    }
    if (QuitButton)
    {
        QuitButton->OnClicked.AddDynamic(this, &USpartaMenuFlowWidget::OnQuitClicked);
    }
    if (ResumeButton)
    {
        ResumeButton->OnClicked.AddDynamic(this, &USpartaMenuFlowWidget::OnResumeClicked);
    }
    if (SpectateButton)
    {
        SpectateButton->OnClicked.AddDynamic(this, &USpartaMenuFlowWidget::OnSpectateClicked);
    }
    if (ExitToLobbyButton)
    {
        ExitToLobbyButton->OnClicked.AddDynamic(this, &USpartaMenuFlowWidget::OnExitToLobbyClicked);
    }
    if (LobbyReturnButton)
    {
        LobbyReturnButton->OnClicked.AddDynamic(this, &USpartaMenuFlowWidget::OnLobbyReturnClicked);
    }
}

void USpartaMenuFlowWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

void USpartaMenuFlowWidget::ShowMainMenu()
{
    if (MenuWidgetSwitcher)
    {
        MenuWidgetSwitcher->SetActiveWidgetIndex(Index_MainMenu);
    }
}

void USpartaMenuFlowWidget::ShowPlayMenu()
{
    if (MenuWidgetSwitcher)
    {
        MenuWidgetSwitcher->SetActiveWidgetIndex(Index_PlayMenu);
    }
}


void USpartaMenuFlowWidget::ShowLobbyMenu()
{
    if (MenuWidgetSwitcher)
    {
        MenuWidgetSwitcher->SetActiveWidgetIndex(Index_LobbyMenu);
    }
}

void USpartaMenuFlowWidget::ShowPauseMenu()
{
    if (MenuWidgetSwitcher)
    {
        MenuWidgetSwitcher->SetActiveWidgetIndex(Index_PauseMenu);
    }
}

// void USpartaMenuFlowWidget::ShowStartCountdown(int32 RemainingSeconds)
// {
//     if (MenuWidgetSwitcher)
//     {
//         MenuWidgetSwitcher->SetActiveWidgetIndex(Index_StartCountdown);
//     }
//
//     if (MatchStartCountdownText)
//     {
//         if (RemainingSeconds > 0)
//         {
//             MatchStartCountdownText->SetText(FText::AsNumber(RemainingSeconds));
//         }
//         else
//         {
//             MatchStartCountdownText->SetText(FText::FromString(TEXT("START!")));
//         }
//     }
// }

void USpartaMenuFlowWidget::ShowMatchResult(EMatchResult Result, int32 MyRank, const TArray<FMatchPlayerResult>& PlayerResults)
{
    if (MenuWidgetSwitcher)
    {
        MenuWidgetSwitcher->SetActiveWidgetIndex(Index_ResultScreen);
    }
	UE_LOG(LogTemp, Warning, TEXT("[MatchResult] ShowMatchResult() 호출: Result=%d, MyRank=%d, PlayerResults.Num()=%d"), static_cast<int32>(Result), MyRank, PlayerResults.Num());
    // 1. 승리/패배 타이틀 텍스트 설정
    if (ResultTitleText)
    {
        FString TitleStr;
        switch (Result)
        {
        case EMatchResult::Victory:
            TitleStr = TEXT("승리!");
            break;
        case EMatchResult::Defeat:
            TitleStr = TEXT("패배..");
            break;
        case EMatchResult::Draw:
            TitleStr = TEXT("무승부!");
            break;
        case EMatchResult::None:
            TitleStr = TEXT("팀 생존 중..");
			break;
        }
        ResultTitleText->SetText(FText::FromString(TitleStr));
    }

    // 2. 본인 순위 출력
    if (MyRankText)
    {
        if(Result == EMatchResult::None)
        {
            MyRankText->SetText(FText::FromString(TEXT("순위 : 진행 중..")));
        }
        else
        {
            MyRankText->SetText(FText::FromString(FString::Printf(TEXT("순위 : #%d"), MyRank)));
        }
    }

    // 3. 리더보드 목록 생성 및 렌더링
    if (LeaderboardScrollBox)
    {
        LeaderboardScrollBox->ClearChildren();

        for (const FMatchPlayerResult& PlayerRes : PlayerResults)
        {
            if (LeaderboardEntryWidgetClass)
            {
                UUserWidget* EntryWidget = CreateWidget<UUserWidget>(GetWorld(), LeaderboardEntryWidgetClass);
                if (EntryWidget)
                {
                    UTextBlock* RankText = Cast<UTextBlock>(EntryWidget->GetWidgetFromName(TEXT("RankTextBlock")));
                    UTextBlock* NameText = Cast<UTextBlock>(EntryWidget->GetWidgetFromName(TEXT("PlayerNameTextBlock")));

                    if (RankText)
                    {
                        RankText->SetText(FText::FromString(FString::Printf(TEXT("#%d "), PlayerRes.Rank)));
                    }
                    if (NameText)
                    {
                        NameText->SetText(FText::FromString(PlayerRes.PlayerName));
                    }
                    LeaderboardScrollBox->AddChild(EntryWidget);
                }
            }
        }
    }
}

void USpartaMenuFlowWidget::OnJoinClicked()
{
    ShowPlayMenu();
}

void USpartaMenuFlowWidget::OnQuitClicked()
{
    APlayerController* PC = GetOwningPlayer();
    UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
}

void USpartaMenuFlowWidget::OnResumeClicked()
{
    // 일시정지 해제 처리
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        // UI 모드에서 인게임 모드로 전환 설정 및 입력 바인딩 복원
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(false);
    }
    
    // 화면에서 일시정지 위젯 제거 또는 비활성화
    SetVisibility(ESlateVisibility::Collapsed);
}

void USpartaMenuFlowWidget::OnSpectateClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (ASpartaArcadePlayerController* InGamePC = Cast<ASpartaArcadePlayerController>(PC))
    {
        InGamePC->StartSpectating();

        // 결과/일시정지 UI를 닫고 인게임 조작 모드로 복귀
        FInputModeGameOnly InputMode;
        InGamePC->SetInputMode(InputMode);
        InGamePC->SetShowMouseCursor(false);
    }

    SetVisibility(ESlateVisibility::Collapsed);
}

void USpartaMenuFlowWidget::OnExitToLobbyClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (ASpartaArcadePlayerController* InGamePC = Cast<ASpartaArcadePlayerController>(PC))
	{
		InGamePC->LeaveGame();
	}
	else if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(PC))
	{
		LobbyPC->LeaveLobby();
	}
	else
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UTravelGameInstanceSubsystem* TravelSubsystem = GameInstance->GetSubsystem<UTravelGameInstanceSubsystem>())
			{
				TravelSubsystem->TravelToTitleMap();
			}
		}
	}
}

void USpartaMenuFlowWidget::OnLobbyReturnClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (ASpartaArcadePlayerController* InGamePC = Cast<ASpartaArcadePlayerController>(PC))
	{
		InGamePC->LeaveGame();
	}
	else if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(PC))
	{
		LobbyPC->LeaveLobby();
	}
	else
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UTravelGameInstanceSubsystem* TravelSubsystem = GetGameInstance()->GetSubsystem<UTravelGameInstanceSubsystem>())
			{
				TravelSubsystem->TravelToTitleMap();
			}
		}
	}
}
