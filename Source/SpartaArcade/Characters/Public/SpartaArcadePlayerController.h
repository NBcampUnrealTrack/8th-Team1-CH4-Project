#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "SpartaArcadePlayerController.generated.h"

class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS()
class ASpartaArcadePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASpartaArcadePlayerController();

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


	// 테스트를 위한 HUD UI 위젯 클래스 및 인스턴스
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = PlayerController, Meta = (AllowPrivateAccess))
	TSubclassOf<UUserWidget> HUDUIWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = PlayerController, Meta = (AllowPrivateAccess))
	TObjectPtr<UUserWidget> HUDUIWidgetInstance;

protected:

	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	
	//  WASD 이동 입력 처리 함수
	void OnMoveTriggered(const struct FInputActionValue& Value);

	// 폭탄 설치, 폭탄 차기, 구급상자 사용 입력 키 바인딩용 콜백 함수들
	void OnPlaceBombTriggered();
	void OnKickBombTriggered();
	void OnUseFirstAidKitTriggered();
	void OnUseShieldTriggered();

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