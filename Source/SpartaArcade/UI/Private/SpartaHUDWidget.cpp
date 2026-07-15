#include "SpartaHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "SpartaArcadeStatBar.h"
#include "SpartaArcadeStatSlot.h"
#include "Components/HorizontalBox.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"
#include "Systems/Public/StatComponent.h"
#include "Systems/Public/CombatComponent.h"
#include "Systems/Public/BombPlacerComponent.h"
#include "Systems/Public/BomberAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Level/Public/SpartaArcadeZoneManager.h"
#include "Characters/Public/SpartaArcadeCharacter.h"
#include "GameFramework/GameStateBase.h"

void USpartaHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    //초기 하트 체력 설정 (3/3) 및 기절 패널 비활성화
    UpdateHearts(3, 3);
    SetStunActive(false);
    
    UpdateBombStats(1, 1);
    UpdateCharacterStats(1.0f, 1.0f, false);
    UpdateGameStateInfo(0, 0);
}

void USpartaHUDWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

// 기절 게이지 실시간 갱신 및 경기 정보(생존 플레이어, 자기장 카운트다운) 실시간 업데이트
void USpartaHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 1. 기절 탈출 게이지 실시간 갱신
    if (CombatComponent && SpartaPlayerState && SpartaPlayerState->GetCurrentState() == EBomberPlayerState::Stunned)
    {
        float Progress = CombatComponent->GetStunProgressPercent();
        UpdateStunProgress(Progress);
    }

    // 2. 실시간 자기장 시간, 생존자 수, 우승 결과 연계 갱신
    UWorld* World = GetWorld();
    if (World)
    {
        // 2-1. 생존 플레이어 수 카운트
        int32 AliveCount = 0;
        if (AGameStateBase* GS = World->GetGameState())
        {
            for (APlayerState* PS : GS->PlayerArray)
            {
                if (ASpartaPlayerState* SPS = Cast<ASpartaPlayerState>(PS))
                {
                    if (SPS->GetCurrentState() != EBomberPlayerState::Eliminated)
                    {
                        AliveCount++;
                    }
                }
            }
        }

        // 2-2. 자기장 매니저 탐색 및 남은 시간(초) 산출
        static TWeakObjectPtr<ASpartaArcadeZoneManager> CachedZoneManager = nullptr;
        if (!CachedZoneManager.IsValid())
        {
            CachedZoneManager = Cast<ASpartaArcadeZoneManager>(UGameplayStatics::GetActorOfClass(World, ASpartaArcadeZoneManager::StaticClass()));
        }

        int32 MatchSeconds = 0;
        int32 ZonePhase = 0;

        if (CachedZoneManager.IsValid())
        {
            float Elapsed = CachedZoneManager->GetElapsed();
            float ActivationDelay = CachedZoneManager->ActivationDelay;
            float ShrinkDuration = CachedZoneManager->ShrinkDuration;

            if (Elapsed < ActivationDelay)
            {
                // 자기장 수축 전: 남은 작동 대기 시간 카운트다운
                MatchSeconds = FMath::Max(0, FMath::RoundToInt(ActivationDelay - Elapsed));
            }
            else
            {
                // 자기장 수축 중: 남은 수축 시간 카운트다운
                MatchSeconds = FMath::Max(0, FMath::RoundToInt((ActivationDelay + ShrinkDuration) - Elapsed));
            }
        }

        // 2-3. HUD 텍스트 컴포넌트 갱신
        UpdateGameStateInfo(AliveCount, MatchSeconds);

        // 2-4. 최후의 1인 우승(Victory) 결과창 연동 트리거
        if (AliveCount <= 1 && IsValid(SpartaPlayerState) && SpartaPlayerState->GetCurrentState() != EBomberPlayerState::Eliminated)
        {
            if (ASpartaArcadeCharacter* LocalChar = Cast<ASpartaArcadeCharacter>(GetOwningPlayerPawn()))
            {
                LocalChar->ShowMatchResultUI(EMatchResult::Victory);
            }
        }
    }
}

void USpartaHUDWidget::UpdateHearts(int32 CurrentHearts, int32 MaxHearts)
{
    // 하트 개수만큼 UI 슬롯에 하트 유닛 스폰 및 리스트업
    if (HeartHorizontalBox && HeartUnitWidgetClass)
    {
        HeartHorizontalBox->ClearChildren();
        for (int32 i = 0; i < CurrentHearts; ++i)
        {
            UUserWidget* HeartWidget = CreateWidget<UUserWidget>(this, HeartUnitWidgetClass);
            if (HeartWidget)
            {
                HeartHorizontalBox->AddChildToHorizontalBox(HeartWidget);
            }
        }
    }
}

void USpartaHUDWidget::SetStunActive(bool bIsActive)
{
    // 기절 오버레이 활성 상태 토글
    if (StunOverlayPanel)
    {
        StunOverlayPanel->SetVisibility(bIsActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void USpartaHUDWidget::UpdateStunProgress(float Percent)
{
    //  탈출 게이지 충전률 갱신
    if (StunProgressBar)
    {
        StunProgressBar->SetPercent(Percent);
    }
}

void USpartaHUDWidget::UpdateBombStats(int32 CurrentBombs, int32 MaxBombs)
{
    if (CurrentBombCountText)
    {
        CurrentBombCountText->SetText(FText::AsNumber(CurrentBombs));
    }

    if (MaxBombCountText)
    {
        MaxBombCountText->SetText(FText::AsNumber(MaxBombs));
    }
}

void USpartaHUDWidget::UpdateCharacterStats(float ExplosionRange, float MoveSpeed, bool bHasShield)
{
    if (ShieldStatusText)
    {
        ShieldStatusText->SetText(FText::FromString(bHasShield ? TEXT("ACTIVE") : TEXT("NONE")));
    }
}

void USpartaHUDWidget::UpdateGameStateInfo(int32 AlivePlayers, int32 MatchSeconds)
{
    if (AlivePlayersText)
    {
        AlivePlayersText->SetText(FText::FromString(FString::Printf(TEXT("%d 명 생존"), AlivePlayers)));
    }

    if (MatchTimeText)
    {
        MatchTimeText->SetText(FText::FromString(FormatTime(MatchSeconds)));
    }
}

// OnHit 델리게이트 호출 시 대미지 위젯 화면 표시 로직 구현
void USpartaHUDWidget::HandleOnHit()
{
    if (DamageTextWidgetClass)
    {
        UUserWidget* DamageWidget = CreateWidget<UUserWidget>(GetWorld(), DamageTextWidgetClass);
        if (DamageWidget)
        {
            DamageWidget->AddToViewport();
        }
    }
}

void USpartaHUDWidget::HandleOnItemPickup(EItemType ItemType, int32 NewCount)
{
    // 아이템 습득 시 능력치 갱신 이벤트 대응 (로직 소유 안 함, 수치 적용은 캐릭터/스탯 시스템의 몫)
    // 여기서는 화면 갱신만 처리
    switch (ItemType)
    {
    case EItemType::BombCount:
        // 폭탄 개수 갱신은 StatSystem 이벤트를 통해 UpdateBombStats 로 반영되거나
        // 획득 연출 팝업 처리
        break;
    case EItemType::ExplosionRange:
    case EItemType::MoveSpeed:
    case EItemType::Shield:
        // 스탯 변동 위젯 갱신
        break;
    }
}

void USpartaHUDWidget::HandleOnShieldBlock()
{
    // 방어막 피격 연출 처리
    if (ShieldStatusText)
    {
        // 쉴드 깨짐 또는 방어 피드백 표시
        ShieldStatusText->SetText(FText::FromString(TEXT("BLOCKED!")));
    }
}

FString USpartaHUDWidget::FormatTime(int32 TotalSeconds) const
{
    int32 Minutes = TotalSeconds / 60;
    int32 Seconds = TotalSeconds % 60;
    return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}

// --------------------------------------------
// Bomb, Explosion, MoveSpeed, Shield 등 스탯 갱신 함수 구현
void USpartaHUDWidget::UpdateCurrentBombs(int32 CurrentBombs)
{
    if (CurrentBombCountText)
    {
        CurrentBombCountText->SetText(FText::AsNumber(CurrentBombs));
    }
}

void USpartaHUDWidget::UpdateStats(int32 BombCount, float ExplosionRange, float MoveSpeed)
{
    if (MaxBombCountText)
    {
        MaxBombCountText->SetText(FText::AsNumber(BombCount));
    }

    // C++ 스탯 바 인스턴스가 위젯에 얹어져 있다면 실시간으로 칸 개수 연산 동기화
    if (BombCountBar)
    {
        BombCountBar->UpdateStatBar(BombCount);
    }
    if (RangeBar)
    {
        RangeBar->UpdateStatBar(FMath::RoundToInt(ExplosionRange));
    }
    if (SpeedBar)
    {
        SpeedBar->UpdateStatBar(FMath::RoundToInt(MoveSpeed));
    }
}

void USpartaHUDWidget::UpdateHasShield(bool bHasShield)
{

}

void USpartaHUDWidget::InitializeHUD(ASpartaPlayerState* PlayerState, UBomberAttributeSet* InAttributeSet, UCombatComponent* CombatComp, UBombPlacerComponent* BombPlacerComp)
{
    // 멤버 컴포넌트 캐싱 추가
    SpartaPlayerState = PlayerState;
    BomberAttributeSet = InAttributeSet;
    CombatComponent = CombatComp;
    BombPlacerComponent = BombPlacerComp;

    // 스탯 바 위젯 초기화 (최대치 개수로 칸 동적 생성)
    if (StatSlotWidgetClass)
    {
        if (BombCountBar) BombCountBar->InitializeBar(8, StatSlotWidgetClass); // 최대 8칸
        if (RangeBar) RangeBar->InitializeBar(5, StatSlotWidgetClass);       // 최대 5칸
        if (SpeedBar) SpeedBar->InitializeBar(5, StatSlotWidgetClass);       // 최대 5칸
    }

    if (PlayerState)
    {
		PlayerState->OnHeartsChanged.AddDynamic(this, &USpartaHUDWidget::UpdateHearts);
		PlayerState->OnStunStateChanged.AddDynamic(this, &USpartaHUDWidget::SetStunActive);
		PlayerState->OnFirstAidKitsChanged.AddDynamic(this, &USpartaHUDWidget::UpdateMedKitStatus);
		PlayerState->OnShieldsChanged.AddDynamic(this, &USpartaHUDWidget::UpdateShieldItemStatus);

		// 초기 가시성 상태 세팅
		UpdateMedKitStatus(PlayerState->GetFirstAidKits());
		UpdateShieldItemStatus(PlayerState->GetShields());
    }
    

    if (InAttributeSet)
    {
        InAttributeSet->OnStatsChanged.AddDynamic(this, &USpartaHUDWidget::UpdateStats);

        // 시작 시점의 초기 스탯치 값으로 바 가시성 세팅 강제 트리거
        UpdateStats(
            FMath::RoundToInt(InAttributeSet->GetBombCount()),
            InAttributeSet->GetBombRange(),
            InAttributeSet->GetMoveSpeed()
        );

        // CurrentPlacedBombs가 변경될 때 HUD 폭탄 수량 실시간 갱신 바인딩 추가
        if (UAbilitySystemComponent* ASC = InAttributeSet->GetOwningAbilitySystemComponent())
        {
            ASC->GetGameplayAttributeValueChangeDelegate(UBomberAttributeSet::GetCurrentPlacedBombsAttribute())
                .AddLambda([this](const FOnAttributeChangeData& Data)
                {
                    UpdateCurrentBombs(FMath::RoundToInt(Data.NewValue));
                });
        }
        UpdateCurrentBombs(FMath::RoundToInt(InAttributeSet->GetCurrentPlacedBombs()));
    }

    if(CombatComp)
    {
        CombatComp->OnShieldBlock.AddDynamic(this, &USpartaHUDWidget::HandleOnShieldBlock);
        // OnHit 델리게이트 바인딩 추가
        CombatComp->OnHit.AddDynamic(this, &USpartaHUDWidget::HandleOnHit);
		// 쉴드 활성 여부 델리게이트 구독 연동
		CombatComp->OnbHasShieldChanged.AddDynamic(this, &USpartaHUDWidget::UpdateShieldStatus);

		// 초기 가시성 상태 세팅
		UpdateShieldStatus(CombatComp->IsShielded());
	}
}

// 쉴드 활성/비활성(실제 방어 중인지) 상태는 텍스트로만 표시
void USpartaHUDWidget::UpdateShieldStatus(bool bHasShield)
{
	if (ShieldStatusText)
	{
		ShieldStatusText->SetText(FText::FromString(bHasShield ? TEXT("ACTIVE") : TEXT("NONE")));
	}
}

// 쉴드 아이템 보유 여부(획득했는지)는 아이콘으로 표시 — 구급상자(MedImg)와 동일한 패턴
void USpartaHUDWidget::UpdateShieldItemStatus(int32 ShieldCount)
{
	if (ShieldImg)
	{
		ShieldImg->SetVisibility(ShieldCount > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void USpartaHUDWidget::UpdateMedKitStatus(int32 MedKitCount)
{
	if (MedImg)
	{
		MedImg->SetVisibility(MedKitCount > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}