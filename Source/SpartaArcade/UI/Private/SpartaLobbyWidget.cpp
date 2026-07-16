#include "SpartaLobbyWidget.h"
#include "SpartaButton.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Lobby/LobbyPlayerController.h"
#include "Lobby/LobbyGameStateBase.h"
#include "Lobby/LobbyPlayerState.h"
#include "UI/Public/SpartaUIDefs.h"

void USpartaLobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 버튼 클릭 이벤트 바인딩
    if (CharacterAButton)
    {
        CharacterAButton->OnClicked.AddDynamic(this, &USpartaLobbyWidget::OnCharacterAClicked);
    }
    if (CharacterBButton)
    {
        CharacterBButton->OnClicked.AddDynamic(this, &USpartaLobbyWidget::OnCharacterBClicked);
    }
    if (CharacterCButton)
    {
        CharacterCButton->OnClicked.AddDynamic(this, &USpartaLobbyWidget::OnCharacterCClicked);
    }
    if (ReadyButton)
    {
        ReadyButton->OnClicked.AddDynamic(this, &USpartaLobbyWidget::OnReadyClicked);
    }
    if (StartButton)
    {
        StartButton->OnClicked.AddDynamic(this, &USpartaLobbyWidget::OnStartClicked);
    }
    if(QuitButton)
    {
        QuitButton->OnClicked.AddDynamic(this, &USpartaLobbyWidget::OnQuitClicked);
    }

    // 팀 선택 및 자동 분배 버튼 클릭 이벤트 바인딩 추가
    if (RedTeamButton)
    {
        RedTeamButton->OnClicked.AddDynamic(this, &USpartaLobbyWidget::OnRedTeamClicked);
    }
    if (BlueTeamButton)
    {
        BlueTeamButton->OnClicked.AddDynamic(this, &USpartaLobbyWidget::OnBlueTeamClicked);
    }
    if (AutoBalanceToggleButton)
    {
        AutoBalanceToggleButton->OnClicked.AddDynamic(this, &USpartaLobbyWidget::OnAutoBalanceToggleClicked);
    }

    // 기본 프리뷰 세팅
    UpdateCharacterPreview(ESpartaArcadeCharacterType::Explosive);
    
    // 카운트다운 기본 숨김 처리
    if (Countdown)
    {
        Countdown->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (CountdownTextBlock)
    {
        CountdownTextBlock->SetVisibility(ESlateVisibility::Collapsed);
    }

	// StartButton 기본 숨김 처리
    if(StartButton)
    {
        StartButton->SetVisibility(ESlateVisibility::Collapsed);
	}

    if (ALobbyGameStateBase* LobbyGameState = GetWorld()->GetGameState<ALobbyGameStateBase>())
    {
        LobbyGameState->OnLobbyInfoChanged.AddUObject(this, &USpartaLobbyWidget::RefreshLobbyUI);
        LobbyGameState->OnCountdownChanged.AddUObject(this, &USpartaLobbyWidget::UpdateCountdown);
		LobbyGameState->NotifyLobbyUI();
    }
}

void USpartaLobbyWidget::NativeDestruct()
{
    if (ALobbyGameStateBase* LobbyGameState = GetWorld()->GetGameState<ALobbyGameStateBase>())
    {
        LobbyGameState->OnLobbyInfoChanged.RemoveAll(this);
        LobbyGameState->OnCountdownChanged.RemoveAll(this);
    }
    Super::NativeDestruct();

}

void USpartaLobbyWidget::UpdatePlayerList(const TArray<FString>& PlayerNames, const TArray<bool>& ReadyStates, const TArray<int32>& TeamIDs)
{
    if (!PlayerListScrollBox)
    {
        return;
    }

    PlayerListScrollBox->ClearChildren();

    // 로비 게임스테이트에서 개인전 여부를 판별
    bool bIsSoloMode = false;
    if (ALobbyGameStateBase* LobbyGameState = GetWorld()->GetGameState<ALobbyGameStateBase>())
    {
        bIsSoloMode = (LobbyGameState->GameModeType == EGameModeType::Solo);
    }

    // 각 플레이어별 로비 리스트 한 줄을 동적 생성하여 리스트에 부착
    for (int32 i = 0; i < PlayerNames.Num(); ++i)
    {
        if (PlayerEntryWidgetClass)
        {
            UUserWidget* EntryWidget = CreateWidget<UUserWidget>(GetWorld(), PlayerEntryWidgetClass);
            if (EntryWidget)
            {
                UTextBlock* NameText = Cast<UTextBlock>(EntryWidget->GetWidgetFromName(TEXT("PlayerNameText")));
                UTextBlock* ReadyText = Cast<UTextBlock>(EntryWidget->GetWidgetFromName(TEXT("ReadyStateText")));

                if (NameText)
                {
                    FString DisplayName = PlayerNames[i];
                    // 개인전이 아닐 때만 각 플레이어의 팀 정보를 텍스트에 추가 표시
                    if (!bIsSoloMode && TeamIDs.IsValidIndex(i))
                    {
                        if (TeamIDs[i] == 1)
                        {
                            DisplayName += TEXT(" [RED]");
                        }
                        else if (TeamIDs[i] == 2)
                        {
                            DisplayName += TEXT(" [BLUE]");
                        }
                    }
                    NameText->SetText(FText::FromString(DisplayName));
                }
                if (ReadyText)
                {
                    FString ReadyStr = ReadyStates.IsValidIndex(i) && ReadyStates[i] ? TEXT("READY") : TEXT("WAITING");
                    ReadyText->SetText(FText::FromString(ReadyStr));
                }

                PlayerListScrollBox->AddChild(EntryWidget);
            }
        }
    }
}

void USpartaLobbyWidget::UpdateCountdown(int32 RemainingSeconds)
{
    if (!CountdownTextBlock)
    {
        return;
    }

    if (RemainingSeconds > 0)
    {
        if (Countdown)
        {
            Countdown->SetVisibility(ESlateVisibility::Visible);
        }
        CountdownTextBlock->SetVisibility(ESlateVisibility::Visible);
        CountdownTextBlock->SetText(FText::FromString(FString::Printf(TEXT("MATCH STARTS IN %d..."), RemainingSeconds)));
    }
    else
    {
        if (Countdown)
        {
            Countdown->SetVisibility(ESlateVisibility::Collapsed);
        }
        CountdownTextBlock->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void USpartaLobbyWidget::SetStartButtonVisibility(bool bIsHost, bool bCanStart)
{
    if (StartButton)
    {
        // 호스트이고 모든 플레이어가 준비됐을 때만 보이거나 활성화
        StartButton->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        StartButton->SetIsEnabled(bCanStart);
    }
}

void USpartaLobbyWidget::UpdateCharacterPreview(ESpartaArcadeCharacterType CharacterType)
{
    SelectedCharacterType = CharacterType;

    // 캐릭터 타입 설명 및 스탯 프리뷰 데이터 세팅 (임시값 적용)
    FString RangeText;
    FString SpeedText;
    FString BombText;
    FString Description;

    // 최종 기획에 맞게 수치 수정
    switch (CharacterType)
    {
    case ESpartaArcadeCharacterType::Explosive:
        RangeText = TEXT("폭발 범위 : 2");
        BombText = TEXT("폭탄 갯수 : 1");
        SpeedText = TEXT("이동 속도 : 1");
        Description = TEXT("화력광 \n 폭발 범위 1단계를 더 갖고 시작합니다.");
        break;
    case ESpartaArcadeCharacterType::Speed:
        RangeText = TEXT("폭발 범위 : 1");
        BombText = TEXT("폭탄 갯수 : 1");
        SpeedText = TEXT("이동 속도 : 2");
        Description = TEXT("속도광 \n 이동 속도 1단계를 더 갖고 시작합니다.");
        break;
    case ESpartaArcadeCharacterType::BombCount:
        RangeText = TEXT("폭발 범위 : 1");
        BombText = TEXT("폭탄 갯수 : 2");
        SpeedText = TEXT("이동 속도 : 1");
        Description = TEXT("폭탄광 \n 폭탄 갯수 1개를 더 갖고 시작합니다.");
        break;
    }

    if (PreviewStatRangeText)
    {
        PreviewStatRangeText->SetText(FText::FromString(RangeText));
    }
    if (PreviewStatSpeedText)
    {
        PreviewStatSpeedText->SetText(FText::FromString(SpeedText));
    }
    if (PreviewStatBombCountText)
    {
        PreviewStatBombCountText->SetText(FText::FromString(BombText));
    }
    if (PreviewStatDescriptionText)
    {
        PreviewStatDescriptionText->SetText(FText::FromString(Description));
    }
}

void USpartaLobbyWidget::OnCharacterAClicked()
{
    UpdateCharacterPreview(ESpartaArcadeCharacterType::Explosive);

    // 네트워크 컨트롤러에 선택 정보 전송
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
		ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(PC);
        if(IsValid(LobbyPC))
        {
            LobbyPC->ServerSelectCharacter(ESpartaArcadeCharacterType::Explosive);
        }
    }
}

void USpartaLobbyWidget::OnCharacterBClicked()
{
    UpdateCharacterPreview(ESpartaArcadeCharacterType::Speed);

    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
		ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(PC);
        if(IsValid(LobbyPC))
        {
            LobbyPC->ServerSelectCharacter(ESpartaArcadeCharacterType::Speed);
		}
    }
}

void USpartaLobbyWidget::OnCharacterCClicked()
{
    UpdateCharacterPreview(ESpartaArcadeCharacterType::BombCount);

    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
		ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(PC);
        if(IsValid(LobbyPC))
        {
			LobbyPC->ServerSelectCharacter(ESpartaArcadeCharacterType::BombCount);
		}
    }
}

void USpartaLobbyWidget::OnReadyClicked()
{
    bIsReady = !bIsReady;

    if (ReadyButton)
    {
        ReadyButton->SetButtonText(FText::FromString(bIsReady ? TEXT("준비 취소") : TEXT("준비")));
    }

    // 네트워크 컨트롤러에 준비 변경 이벤트 알림
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
		ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(PC);
        if(IsValid(LobbyPC))
        {
			LobbyPC->ServerToggleReady();
		}
    }
}

void USpartaLobbyWidget::OnStartClicked()
{
    // 호스트의 게임 시작 요청 처리
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
		ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(PC);
        if(IsValid(LobbyPC))
		{
			LobbyPC->ServerStartMatch();
		}
    }
}

void USpartaLobbyWidget::OnQuitClicked()
{
    // 로비 나가기 요청 처리
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(PC);
        if(IsValid(LobbyPC))
        {
			LobbyPC->LeaveLobby();
        }
    }
}

void USpartaLobbyWidget::RefreshLobbyUI(const TArray<FString>& PlayerNames, const TArray<bool>& ReadyStates, const TArray<int32>& TeamIDs, bool bIsHost, bool bCanStart, bool bAutoBalance, int32 RemainingSeconds)
{
    if (bIsHost)
    {
        SetStartButtonVisibility(true, bCanStart);
    }
    else
    {
        SetStartButtonVisibility(false, false);
    }

    UpdatePlayerList(PlayerNames, ReadyStates, TeamIDs);

    // 로컬 플레이어의 TeamID 확인하여 버튼 색상 갱신
    int32 LocalTeamID = 0;
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        if (ALobbyPlayerState* LocalPS = PC->GetPlayerState<ALobbyPlayerState>())
        {
            LocalTeamID = LocalPS->GetTeamID();
        }
    }

    // 개인전(Solo) 모드 여부를 판단하여 팀 관련 UI를 가림
    bool bIsSoloMode = false;
    if (ALobbyGameStateBase* LobbyGameState = GetWorld()->GetGameState<ALobbyGameStateBase>())
    {
        bIsSoloMode = (LobbyGameState->GameModeType == EGameModeType::Solo);
    }
    
    ESlateVisibility TeamUIVisibility = bIsSoloMode ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;

    // Red/Blue 버튼 비주얼 선택 상태 및 가시성 갱신
    if (RedTeamButton)
    {
        RedTeamButton->SetVisibility(TeamUIVisibility);
        RedTeamButton->SetButtonColor(LocalTeamID == 1 ? FLinearColor(1.0f, 0.15f, 0.15f) : FLinearColor(0.25f, 0.05f, 0.05f));
    }
    if (BlueTeamButton)
    {
        BlueTeamButton->SetVisibility(TeamUIVisibility);
        BlueTeamButton->SetButtonColor(LocalTeamID == 2 ? FLinearColor(0.15f, 0.15f, 1.0f) : FLinearColor(0.05f, 0.05f, 0.25f));
    }

    // 팀 자동 분배 토글 버튼 갱신 및 가시성 적용 (개인전일 때는 숨김)
    if (AutoBalanceToggleButton)
    {
        AutoBalanceToggleButton->SetVisibility(TeamUIVisibility);
        FString ToggleStr = bAutoBalance ? TEXT("팀 자동 배분 : ON") : TEXT("팀 자동 배분 : OFF");
        AutoBalanceToggleButton->SetButtonText(FText::FromString(ToggleStr));
        AutoBalanceToggleButton->SetIsEnabled(bIsHost);
        AutoBalanceToggleButton->SetButtonColor(bAutoBalance ? FLinearColor(0.15f, 0.6f, 0.15f) : FLinearColor(0.3f, 0.3f, 0.3f));
    }
}

// Red 팀 선택 요청 RPC 호출 연동
void USpartaLobbyWidget::OnRedTeamClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(PC))
        {
            LobbyPC->ServerSelectTeam(1);
        }
    }
}

// Blue 팀 선택 요청 RPC 호출 연동
void USpartaLobbyWidget::OnBlueTeamClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(PC))
        {
            LobbyPC->ServerSelectTeam(2);
        }
    }
}

// 방장의 팀 자동 분배 변경 요청 RPC 호출 연동
void USpartaLobbyWidget::OnAutoBalanceToggleClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(PC))
        {
            if (ALobbyGameStateBase* LobbyGameState = GetWorld()->GetGameState<ALobbyGameStateBase>())
            {
                LobbyPC->ServerSetAutoBalanceTeam(!LobbyGameState->bAutoBalanceTeam);
            }
        }
    }
}
