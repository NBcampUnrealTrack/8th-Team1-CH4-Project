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
#include "EOSGameInstanceSubsystem.h"
#include "SessionService.h"
#include "SpartaArcadeGameMode.h"
#include "GameFlow/TravelGameInstanceSubsystem.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"

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

// 세션 파괴 완료 후 타이틀 맵으로 복귀하는 콜백 구현
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

// 클라이언트가 로비 등에서 직접 팀(1 또는 2)을 선택해 서버로 변경을 요청하는 RPC 구현
bool ASpartaArcadePlayerController::ServerSetTeam_Validate(int32 NewTeamID)
{
	return (NewTeamID == 1 || NewTeamID == 2);
}

void ASpartaArcadePlayerController::ServerSetTeam_Implementation(int32 NewTeamID)
{
	if (ASpartaPlayerState* SPS = GetPlayerState<ASpartaPlayerState>())
	{
		// 서버 게임모드의 bIsTeamMode 여부를 검사해 팀전 모드일 때만 팀 선택 변경 허용
		bool bTeamModeActive = false;
		if (ASpartaArcadeGameMode* GM = Cast<ASpartaArcadeGameMode>(GetWorld()->GetAuthGameMode()))
		{
			bTeamModeActive = GM->bIsTeamMode;
		}

		if (bTeamModeActive)
		{
			SPS->SetTeamID(NewTeamID);
		}
		else
		{
			SPS->SetTeamID(0); // 개인전일 경우 무조건 팀 ID는 0으로 고정
		}

		// 플레이어 캐릭터 닉네임 색상 즉시 업데이트 연동
		if (ASpartaArcadeCharacter* ArcadeCharacter = Cast<ASpartaArcadeCharacter>(GetPawn()))
		{
			ArcadeCharacter->UpdateNickname();
		}
		UE_LOG(LogTemp, Warning, TEXT("[TeamSelection] %s 의 팀이 서버에서 Team %d 로 변경되었습니다."), *GetName(), SPS->GetTeamID());
	}
}


