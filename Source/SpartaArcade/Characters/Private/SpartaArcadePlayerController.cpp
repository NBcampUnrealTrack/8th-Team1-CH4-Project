#include "SpartaArcadePlayerController.h"

#include "BomberTypes.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "SpartaArcadeCharacter.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Blueprint/UserWidget.h"
#include "UI/Public/SpartaHUDWidget.h"
#include "EOSGameInstanceSubsystem.h"
#include "SessionService.h"
#include "GameFlow/TravelGameInstanceSubsystem.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"
#include "UI/Public/SpartaMenuFlowWidget.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

ASpartaArcadePlayerController::ASpartaArcadePlayerController()
{
}

void ASpartaArcadePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	bIsSpectating = false;

	if (IsLocalController() == false)
	{
		return;
	}

	// Removed: 중복 AddMappingContext 호출 제거 (SetupInputComponent에서 처리됨)
	// if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	// {
	// 	Subsystem->AddMappingContext(DefaultMappingContext, 0);
	// }

	FInputModeGameOnly GameOnly;
	SetInputMode(GameOnly);
	bShowMouseCursor = false;

	// 테스트용 HUD UI 위젯 생성 및 뷰포트에 추가
	if(IsValid(HUDUIWidgetClass))
	{
		HUDUIWidgetInstance = CreateWidget<UUserWidget>(this, HUDUIWidgetClass);
		if (IsValid(HUDUIWidgetInstance))
		{
			HUDUIWidgetInstance->AddToViewport();
		}
	}

	// 메인 메뉴 위젯 생성 및 뷰포트에 추가, 초기 상태는 숨김
	if (IsValid(MainMenuWidgetClass))
	{
		MainMenuWidgetInstance = CreateWidget<USpartaMenuFlowWidget>(this, MainMenuWidgetClass);
		if (IsValid(MainMenuWidgetInstance))
		{
			MainMenuWidgetInstance->AddToViewport();
			MainMenuWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 세션 파괴 완료 이벤트 바인딩 추가
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UEOSGameInstanceSubsystem* EOSSubsystem = GameInstance->GetSubsystem<UEOSGameInstanceSubsystem>())
		{
			if (USessionService* SessionService = EOSSubsystem->GetSessionService())
			{
				SessionService->OnDestroySessionCompleteEvent.AddUObject(this, &ASpartaArcadePlayerController::HandleDestroySessionComplete);
			}
		}
	}
}

void ASpartaArcadePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Enhanced Input 컨텍스트 등록
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	// 입력 액션 이벤트 및 핸들러 등록
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// WASD 이동 인풋 액션 바인딩
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASpartaArcadePlayerController::OnMoveTriggered);
		}

		// 캐릭터 폭탄 설치 액션
		if (PlaceBombAction)
		{
			EnhancedInputComponent->BindAction(PlaceBombAction, ETriggerEvent::Started, this, &ASpartaArcadePlayerController::OnPlaceBombTriggered);
		}

		// 캐릭터 폭탄 차기 액션 연동
		if (KickBombAction)
		{
			EnhancedInputComponent->BindAction(KickBombAction, ETriggerEvent::Started, this, &ASpartaArcadePlayerController::OnKickBombTriggered);
		}

		// 캐릭터 구급상자 소모 액션 연동
		if (UseFirstAidKitAction)
		{
			EnhancedInputComponent->BindAction(UseFirstAidKitAction, ETriggerEvent::Started, this, &ASpartaArcadePlayerController::OnUseFirstAidKitTriggered);
		}
		
		if (UseShieldAction)
		{
			EnhancedInputComponent->BindAction(UseShieldAction, ETriggerEvent::Started, this, &ASpartaArcadePlayerController::OnUseShieldTriggered);
		}

		if (SpectateNextAction)
		{
			EnhancedInputComponent->BindAction(SpectateNextAction, ETriggerEvent::Started, this, &ASpartaArcadePlayerController::OnSpectateNextTriggered);
		}

		if (SpectatePrevAction)
		{
			EnhancedInputComponent->BindAction(SpectatePrevAction, ETriggerEvent::Started, this, &ASpartaArcadePlayerController::OnSpectatePrevTriggered);
		}

		// ESC 메뉴 토글 액션
		if (ToggleMenuAction)
		{
			EnhancedInputComponent->BindAction(ToggleMenuAction, ETriggerEvent::Started, this, &ASpartaArcadePlayerController::OnToggleMenuTriggered);
		}
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' EIC 인식 실패"), *GetNameSafe(this));
	}
}

// WASD 이동 처리 함수
void ASpartaArcadePlayerController::OnMoveTriggered(const FInputActionValue& Value)
{
	if (bIsSpectating)
	{
		return;
	}

	FVector2D MovementVector = Value.Get<FVector2D>();

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn != nullptr)
	{
		// 캐릭터가 기절 상태(Stunned)일 경우 모든 이동 조작을 차단
		ASpartaArcadeCharacter* ArcadeChar = Cast<ASpartaArcadeCharacter>(ControlledPawn);
		if (ArcadeChar && ArcadeChar->IsStunned())
		{
			return;
		}

		// 정통 2D/2.5D 탑뷰 방식의 절대축 이동 적용 (카메라 앵글 무시)
		ControlledPawn->AddMovementInput(FVector::ForwardVector, MovementVector.Y);
		ControlledPawn->AddMovementInput(FVector::RightVector, MovementVector.X);
	}
}

// 로컬 입력 트리거 시 서버 RPC를 호출하도록 연결
void ASpartaArcadePlayerController::OnPlaceBombTriggered()
{
	if (bIsSpectating)
	{
		return;
	}
	ServerPlaceBomb();
}

void ASpartaArcadePlayerController::OnKickBombTriggered()
{
	if (bIsSpectating)
	{
		return;
	}
	ServerKickBomb();
}

void ASpartaArcadePlayerController::OnUseFirstAidKitTriggered()
{
	if (bIsSpectating)
	{
		return;
	}
	ServerUseFirstAidKit();
}

void ASpartaArcadePlayerController::OnUseShieldTriggered()
{
	if (bIsSpectating)
	{
		return;
	}
	ServerUseShield();
}

void ASpartaArcadePlayerController::OnSpectateNextTriggered()
{
	SpectateNext();
}

void ASpartaArcadePlayerController::OnSpectatePrevTriggered()
{
	SpectatePrev();
}

void ASpartaArcadePlayerController::OnToggleMenuTriggered()
{
	if (!IsLocalController() || !IsValid(MainMenuWidgetInstance))
	{
		return;
	}

	const bool bMenuVisible = MainMenuWidgetInstance->IsVisible();
	if (bMenuVisible)
	{
		MainMenuWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
		FInputModeGameOnly GameOnly;
		SetInputMode(GameOnly);
		bShowMouseCursor = false;
		return;
	}

	MainMenuWidgetInstance->ShowPauseMenu();
	MainMenuWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(MainMenuWidgetInstance->GetCachedWidget());
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	bShowMouseCursor = true;
}

void ASpartaArcadePlayerController::ServerUseShield_Implementation()
{
	if (ASpartaArcadeCharacter* ArcadeCharacter = Cast<ASpartaArcadeCharacter>(GetPawn()))
	{
		ArcadeCharacter->UseShield();
	}
}


// 서버 측에서 실제 캐릭터 행동을 집행하는 Server RPC 구현부 정의
void ASpartaArcadePlayerController::ServerPlaceBomb_Implementation()
{
	ASpartaArcadeCharacter* ArcadeCharacter = Cast<ASpartaArcadeCharacter>(GetPawn());
	if (ArcadeCharacter)
	{
		ArcadeCharacter->PlaceBomb();
	}
}

void ASpartaArcadePlayerController::ServerKickBomb_Implementation()
{
	ASpartaArcadeCharacter* ArcadeCharacter = Cast<ASpartaArcadeCharacter>(GetPawn());
	if (ArcadeCharacter)
	{
		ArcadeCharacter->KickBomb();
	}
}

void ASpartaArcadePlayerController::ServerUseFirstAidKit_Implementation()
{
	ASpartaArcadeCharacter* ArcadeCharacter = Cast<ASpartaArcadeCharacter>(GetPawn());
	if (ArcadeCharacter)
	{
		ArcadeCharacter->UseFirstAidKit();
	}
}

void ASpartaArcadePlayerController::ClientShowMatchResult_Implementation(const FMatchResultData& InMatchResultData)
{
	if(IsValid(GetWorld()) == false)
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(ResultUITimerHandle);

	MatchResultData = InMatchResultData;
	FTimerDelegate TimerDelegate;
	GetWorld()->GetTimerManager().SetTimer(ResultUITimerHandle, this, &ASpartaArcadePlayerController::ShowMatchResult, 2.0f, false);
}

void ASpartaArcadePlayerController::ClientShowKillLog_Implementation(const FString& KillerName, const FString& VictimName, EDeathReason Reason)
{
	if (IsValid(HUDUIWidgetInstance))
	{
		if (USpartaHUDWidget* HUDWidget = Cast<USpartaHUDWidget>(HUDUIWidgetInstance))
		{
			HUDWidget->AddKillLog(KillerName, VictimName, Reason);
		}
	}
}

void ASpartaArcadePlayerController::ShowMatchResult()
{
	if (IsValid(MainMenuWidgetInstance))
	{
		MainMenuWidgetInstance->SetVisibility(ESlateVisibility::Visible);
		MainMenuWidgetInstance->ShowMatchResult(MatchResultData);
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(MainMenuWidgetInstance->GetCachedWidget());
		SetInputMode(Mode);
		bShowMouseCursor = true;
	}
}

// 세션을 파괴하고 타이틀 맵으로 퇴장하는 함수 구현
void ASpartaArcadePlayerController::LeaveGame()
{
	if (IsLocalController() == false)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UEOSGameInstanceSubsystem* EOSSubsystem = GameInstance->GetSubsystem<UEOSGameInstanceSubsystem>())
		{
			if (USessionService* SessionService = EOSSubsystem->GetSessionService())
			{
				SessionService->DestroySession();
				return;
			}
		}
	}

	// 세션 서비스 접근 실패 시 차선책으로 직접 트래블 시도
	UTravelGameInstanceSubsystem* TravelSubsystem = GetGameInstance()->GetSubsystem<UTravelGameInstanceSubsystem>();
	if (IsValid(TravelSubsystem))
	{
		TravelSubsystem->TravelToTitleMap();
	}
}

void ASpartaArcadePlayerController::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UTravelGameInstanceSubsystem* TravelSubsystem = GetGameInstance()->GetSubsystem<UTravelGameInstanceSubsystem>();
		if (IsValid(TravelSubsystem))
		{
			TravelSubsystem->TravelToTitleMap();
		}
	}
}

void ASpartaArcadePlayerController::StartSpectating()
{
	bIsSpectating = true;
	SpectateTargets.Empty();

	ASpartaArcadeCharacter* MyPawn = Cast<ASpartaArcadeCharacter>(GetPawn());

	for (TActorIterator<ASpartaArcadeCharacter> It(GetWorld()); It; ++It)
	{
		ASpartaArcadeCharacter* character = *It;
		// 자기 자신은 관전 목록에서 제외함
		if (!IsValid(character) || character == MyPawn) continue;

		if (ASpartaPlayerState* PS = character->GetPlayerState<ASpartaPlayerState>())
		{
			if (PS->GetCurrentState() == EBomberPlayerState::Alive)
			{
				SpectateTargets.Add(character);
			}
		}
	}

	if (SpectateTargets.Num() > 0)
	{
		CurrentSpectateIndex = 0;

		// 카메라 옮기기
		SpectateTarget();
	}
}

void ASpartaArcadePlayerController::BeginInactiveState()
{
	Super::BeginInactiveState();

	// Super가 내부적으로 SetViewTarget(this)를 호출해 카메라를 되돌리므로,
	// 관전 중이라면 그 직후 다시 관전 대상으로 되돌린다
	if (bIsSpectating && SpectateTargets.IsValidIndex(CurrentSpectateIndex))
	{
		SpectateTarget();
	}
}

void ASpartaArcadePlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 엔진이 여러 경로(BeginInactiveState 외에도)에서 ViewTarget을 컨트롤러 자신으로
	// 되돌리는 경우가 있어, 관전 중에는 매 틱마다 관전 대상으로 강제 유지한다
	/*
	if (bIsSpectating && SpectateTargets.IsValidIndex(CurrentSpectateIndex))
	{
		ASpartaArcadeCharacter* Target = SpectateTargets[CurrentSpectateIndex];
		if (IsValid(Target) && GetViewTarget() != Target)
		{
			SetViewTarget(Target);
		}
	}
	*/
}

void ASpartaArcadePlayerController::ClientSpectating_Implementation()
{
	StartSpectating();
}

bool ASpartaArcadePlayerController::MoveSpectateTargetIndex(int32 Direction)
{
	if (!bIsSpectating || SpectateTargets.IsEmpty())
	{
		return false;
	}

	int32 CheckCount = SpectateTargets.Num();
	while (CheckCount--)
	{
		CurrentSpectateIndex = (CurrentSpectateIndex + Direction + SpectateTargets.Num()) % SpectateTargets.Num();
		if (IsValid(SpectateTargets[CurrentSpectateIndex]))
		{
			return true;
		}

		SpectateTargets.RemoveAt(CurrentSpectateIndex);
		if (SpectateTargets.IsEmpty())
		{
			return false;
		}
		CurrentSpectateIndex %= SpectateTargets.Num();
	}

	return false;
}

void ASpartaArcadePlayerController::SpectateNext()
{
	if(MoveSpectateTargetIndex(1))
	{
		SpectateTarget();
	}
}

void ASpartaArcadePlayerController::SpectatePrev()
{
	if (MoveSpectateTargetIndex(-1))
	{
		SpectateTarget();
	}
}

void ASpartaArcadePlayerController::SpectateTarget()
{
	if (!bIsSpectating || !SpectateTargets.IsValidIndex(CurrentSpectateIndex)) 
	{
		return;
	}

	ASpartaArcadeCharacter* TargetCharacter = SpectateTargets[CurrentSpectateIndex];
	if(!IsValid(TargetCharacter))
	{
		return;
	}

	if (IsValid(HUDUIWidgetInstance))
	{
		if(USpartaHUDWidget* HUDWidget = Cast<USpartaHUDWidget>(HUDUIWidgetInstance))
		{
			ASpartaPlayerState* PS = TargetCharacter->GetPlayerState<ASpartaPlayerState>();
			UBomberAttributeSet* AttributeSet = TargetCharacter->GetAttributeSet();
			UCombatComponent* CombatComp = TargetCharacter->GetCombatComponent();

			// 함수 내부에서 검증 후 바인딩 처리하므로 여기서는 단순 호출
			HUDWidget->BindToTarget(PS, AttributeSet, CombatComp);
		}
	}
	SetViewTarget(TargetCharacter);
}