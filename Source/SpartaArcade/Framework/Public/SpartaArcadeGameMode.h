#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SpartaArcadeGameMode.generated.h"

UCLASS(minimalapi)
class ASpartaArcadeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASpartaArcadeGameMode();

	// AGameModeBase 오버라이드
	virtual void PostLogin(APlayerController* NewPlayer) override; // 입장 시 팀 배분을 위해 PostLogin 오버라이드 추가

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpartaArcade|Rules")
	bool bIsTeamMode = false; // 팀전 여부를 나타내는 플래그 추가 (에디터 노출)

protected:
	virtual void BeginPlay() override; // 매치 시작 시 팀 밸런싱 수행을 위해 BeginPlay 추가

private:
	void BalanceTeams(); // 팀 불균형 강제 조율 헬퍼 함수 선언
};
