#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "SpartaUIDefs.h"
#include "SpartaArcadePlayerController.generated.h"

class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;
class USpartaMenuFlowWidget;
class ASpartaArcadeCharacter;
class USoundBase;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS()
class ASpartaArcadePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASpartaArcadePlayerController();

	// 이 플레이어 컨트롤러 세션에 영구 배정된 모서리 스폰 인덱스 (-1 이면 미배정)
	UPROPERTY(Transient)
	int32 AssignedSpawnIndex = -1;

	// IMC
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;
	
	// WASD 이동을 위한 인풋 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;
	
	// 폭탄 설치 인풋 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* PlaceBombAction;

	// 폭탄 차기 및 구급상자 사용 인풋 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* KickBombAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* UseFirstAidKitAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* UseShieldAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* SpectateNextAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* SpectatePrevAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* ToggleMenuAction;

	// 관전 모드 진입여부
	UPROPERTY(Transient)
	bool bIsSpectating = false;

	// 관전 가능 캐릭터
	TArray<class ASpartaArcadeCharacter*> SpectateTargets;

	// 현재 관전 중인 대상의 인덱스
	UPROPERTY(Transient)
	int32 CurrentSpectateIndex = -1;


	// 인게임 진입 시 재생할 배경음악
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sound", Meta = (AllowPrivateAccess))
	TObjectPtr<USoundBase> LevelBGM;

	// 테스트를 위한 HUD UI 위젯 클래스 및 인스턴스
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = PlayerController, Meta = (AllowPrivateAccess))
	TSubclassOf<UUserWidget> HUDUIWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = PlayerController, Meta = (AllowPrivateAccess))
	TObjectPtr<UUserWidget> HUDUIWidgetInstance;

	// 메인 메뉴 UI 위젯 클래스 및 인스턴스
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = PlayerController, Meta = (AllowPrivateAccess))
	TSubclassOf<USpartaMenuFlowWidget> MainMenuWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = PlayerController, Meta = (AllowPrivateAccess))
	TObjectPtr<USpartaMenuFlowWidget> MainMenuWidgetInstance;

	FTimerHandle ResultUITimerHandle;
	FMatchResultData MatchResultData;

public:
	// LeaveGame
	void LeaveGame();

	// HandleDestroySessionComplete
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	// 관전모드 진입
	void StartSpectating();
	
	UFUNCTION(Client, Reliable)
	void ClientSpectating();

	// 다음 / 이전 대상으로 전환
	void SpectateNext();
	void SpectatePrev();
	bool MoveSpectateTargetIndex(int32 Direction);

	// HUD 업데이트를 위한 관전 대상 전환 함수
	void SpectateTarget();

	UFUNCTION(Client, Reliable)
	void ClientShowMatchResult(const FMatchResultData& InMatchResultData);

	UFUNCTION(Client, Reliable)
	void ClientShowKillLog(const FString& KillerName, const FString& VictimName, EDeathReason Reason);

	void ShowMatchResult();
protected:

	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Pawn을 잃으면 엔진이 이 함수 안에서 자동으로 SetViewTarget(this)를 호출해
	// 카메라를 컨트롤러 자신으로 되돌리므로, 관전 중이라면 그 직후 다시 관전 대상으로 되돌린다
	virtual void BeginInactiveState() override;

	//  WASD 이동 입력 처리 함수
	void OnMoveTriggered(const struct FInputActionValue& Value);

	// 폭탄 설치, 폭탄 차기, 구급상자 사용 입력 키 바인딩용 콜백 함수들
	void OnPlaceBombTriggered();
	void OnKickBombTriggered();
	void OnUseFirstAidKitTriggered();
	void OnUseShieldTriggered();
	void OnSpectateNextTriggered();
	void OnSpectatePrevTriggered();
	void OnToggleMenuTriggered();

	// 서버 연산 주도를 위한 Server RPC 선언
	UFUNCTION(Server, Reliable)
	void ServerPlaceBomb();

	UFUNCTION(Server, Reliable)
	void ServerKickBomb();

	UFUNCTION(Server, Reliable)
	void ServerUseFirstAidKit();

	UFUNCTION(Server, Reliable)
	void ServerUseShield();

};