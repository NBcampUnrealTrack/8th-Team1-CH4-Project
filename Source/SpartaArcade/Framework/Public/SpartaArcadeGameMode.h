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

	// 4인 플레이어 모서리 스폰을 위한 ChoosePlayerStart 및 Logout 오버라이드 추가
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void Logout(AController* Exiting) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Spawn")
	void TeleportPlayersToSpawns(const TArray<FVector>& SpawnLocations);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpartaArcade|Rules")
	bool bIsTeamMode = false; // 팀전 여부를 나타내는 플래그 추가 (에디터 노출)

protected:
	virtual void BeginPlay() override; // 매치 시작 시 팀 밸런싱 수행을 위해 BeginPlay 추가

private:
	void BalanceTeams(); // 팀 불균형 강제 조율 헬퍼 함수 선언

	// 플레이어 컨트롤러별로 고유하게 배정된 스폰 인덱스(0~3)를 추적하는 맵
	UPROPERTY()
	TMap<class AController*, int32> AssignedSpawnIndices;
};
