#include "SpartaArcadePlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "SpartaArcadeCharacter.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Blueprint/UserWidget.h"
#include "UI/Public/SpartaHUDWidget.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

ASpartaArcadePlayerController::ASpartaArcadePlayerController()
{
}

void ASpartaArcadePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	FInputModeGameOnly GameOnly;
	SetInputMode(GameOnly);

	// 테스트용 HUD UI 위젯 생성 및 뷰포트에 추가
	if(IsValid(HUDUIWidgetClass))
	{
		HUDUIWidgetInstance = CreateWidget<UUserWidget>(this, HUDUIWidgetClass);
		if (IsValid(HUDUIWidgetInstance))
		{
			HUDUIWidgetInstance->AddToViewport();
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
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' EIC 인식 실패"), *GetNameSafe(this));
	}
}

// WASD 이동 처리 함수
void ASpartaArcadePlayerController::OnMoveTriggered(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn != nullptr)
	{
		//  컨트롤러 회전 방향에 영향받지 않고 일관되게 상/하/좌/우(월드 기준)로 이동하도록 변경
		ControlledPawn->AddMovementInput(FVector::ForwardVector, MovementVector.Y);
		ControlledPawn->AddMovementInput(FVector::RightVector, MovementVector.X);
	}
}

// 로컬 입력 트리거 시 서버 RPC를 호출하도록 연결
void ASpartaArcadePlayerController::OnPlaceBombTriggered()
{
	ServerPlaceBomb();
}

void ASpartaArcadePlayerController::OnKickBombTriggered()
{
	ServerKickBomb();
}

void ASpartaArcadePlayerController::OnUseFirstAidKitTriggered()
{
	ServerUseFirstAidKit();
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
