#include "SpartaArcadeGameMode.h"
#include "SpartaArcadePlayerController.h"
#include "SpartaArcadeCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASpartaArcadeGameMode::ASpartaArcadeGameMode()
{
	// 커스텀 플레이어 컨트롤러 클래스
	PlayerControllerClass = ASpartaArcadePlayerController::StaticClass();

	// Todo : 플레이어 폰 클래스 생성 후 지정
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	// Todo : 플레이어 컨트롤러 새로 생성 후 지정
	static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownPlayerController"));
	if(PlayerControllerBPClass.Class != NULL)
	{
		PlayerControllerClass = PlayerControllerBPClass.Class;
	}
}