#include "SpartaLobbyWidget.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

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

    // 기본 프리뷰 세팅
    UpdateCharacterPreview(ECharacterType::CharacterA);
    
    // 카운트다운 기본 숨김 처리
    if (CountdownTextBlock)
    {
        CountdownTextBlock->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void USpartaLobbyWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

void USpartaLobbyWidget::UpdatePlayerList(const TArray<FString>& PlayerNames, const TArray<bool>& ReadyStates)
{
    if (!PlayerListScrollBox)
    {
        return;
    }

    PlayerListScrollBox->ClearChildren();

    // 각 플레이어별 로비 리스트 한 줄을 동적 생성하여 리스트에 부착
    for (int32 i = 0; i < PlayerNames.Num(); ++i)
    {
        if (PlayerEntryWidgetClass)
        {
            UUserWidget* EntryWidget = CreateWidget<UUserWidget>(GetWorld(), PlayerEntryWidgetClass);
            if (EntryWidget)
            {
                // EntryWidget 내부에 NameText, ReadyText를 찾아 데이터를 주입
                // C++ 클래스를 상속한 경우 캐스팅하여 주입 가능, 일단 텍스트블록 바인딩 등으로 직접 접근 처리
                UTextBlock* NameText = Cast<UTextBlock>(EntryWidget->GetWidgetFromName(TEXT("PlayerNameTextBlock")));
                UTextBlock* ReadyText = Cast<UTextBlock>(EntryWidget->GetWidgetFromName(TEXT("ReadyStatusTextBlock")));

                if (NameText)
                {
                    NameText->SetText(FText::FromString(PlayerNames[i]));
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
        CountdownTextBlock->SetVisibility(ESlateVisibility::Visible);
        CountdownTextBlock->SetText(FText::FromString(FString::Printf(TEXT("MATCH STARTS IN %d..."), RemainingSeconds)));
    }
    else
    {
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

void USpartaLobbyWidget::UpdateCharacterPreview(ECharacterType CharacterType)
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
    case ECharacterType::CharacterA:
        RangeText = TEXT("폭발 범위 : 5");
        BombText = TEXT("폭탄 갯수 : 3");
        SpeedText = TEXT("이동 속도 : 3");
        Description = TEXT("화력 특화형 - 폭발 범위 1단계를 더 갖고 시작합니다.");
        break;
    case ECharacterType::CharacterB:
        RangeText = TEXT("폭발 범위 : 2");
        BombText = TEXT("폭탄 갯수 : 2");
        SpeedText = TEXT("이동 속도 : 4");
        Description = TEXT("기동 특화형 - 이동 속도 1단계를 더 갖고 시작합니다.");
        break;
    case ECharacterType::CharacterC:
        RangeText = TEXT("폭발 범위 : 2");
        BombText = TEXT("폭탄 갯수 : 5");
        SpeedText = TEXT("이동 속도 : 2");
        Description = TEXT("보유량 특화형 - 최대 설치 폭탄 수 1개를 더 갖고 시작합니다.");
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
    UpdateCharacterPreview(ECharacterType::CharacterA);

    // 네트워크 컨트롤러에 선택 정보 전송
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        // PC->GetHUD() 혹은 커스텀 컨트롤러 캐스팅을 통해 RPC 호출을 위임
        // e.g. PC->ServerSelectCharacter(ECharacterType::CharacterA);
    }
}

void USpartaLobbyWidget::OnCharacterBClicked()
{
    UpdateCharacterPreview(ECharacterType::CharacterB);

    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        // e.g. PC->ServerSelectCharacter(ECharacterType::CharacterB);
    }
}

void USpartaLobbyWidget::OnCharacterCClicked()
{
    UpdateCharacterPreview(ECharacterType::CharacterC);

    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        // e.g. PC->ServerSelectCharacter(ECharacterType::CharacterC);
    }
}

void USpartaLobbyWidget::OnReadyClicked()
{
    bIsReady = !bIsReady;

    if (ReadyButton)
    {
        UTextBlock* BtnText = Cast<UTextBlock>(ReadyButton->GetChildAt(0));
        if (BtnText)
        {
            BtnText->SetText(FText::FromString(bIsReady ? TEXT("CANCEL READY") : TEXT("READY")));
        }
    }

    // 네트워크 컨트롤러에 준비 변경 이벤트 알림
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        // e.g. PC->ServerToggleReady();
    }
}

void USpartaLobbyWidget::OnStartClicked()
{
    // 호스트의 게임 시작 요청 처리
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        // e.g. PC->ServerStartMatch();
    }
}
