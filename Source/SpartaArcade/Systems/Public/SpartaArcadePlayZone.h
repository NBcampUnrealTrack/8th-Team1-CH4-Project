#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/SubclassOf.h"
#include "SpartaArcadePlayZone.generated.h"

UCLASS()
class SPARTAARCADE_API ASpartaArcadePlayZone : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpartaArcadePlayZone();

protected:
	virtual void BeginPlay() override;

	// 나선형 즉사 압사 자기장 변수들 추가
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayZone|Rules")
	TSubclassOf<AActor> DeathBlockClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayZone|Rules")
	TSubclassOf<AActor> WarningDecalClass; // 빨간색 경고 바닥용 액터 클래스

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayZone|Rules")
	float WarningDuration; // 경고 후 블록 투하까지의 딜레이 타임

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayZone|Rules")
	float StepInterval; // 다음 타일 경고 스폰까지의 주기

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayZone|Rules")
	int32 GridWidth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayZone|Rules")
	int32 GridHeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayZone|Rules")
	float TileSize;

	// 나선형 2D 경로 계산점 리스트
	TArray<FIntPoint> SpiralPath;
	int32 CurrentSpiralIndex;
	FTimerHandle SpiralStepTimerHandle;

	// 나선형 좌표계 초기화 헬퍼 함수
	void InitializeSpiralPath();

	// 나선형 진행 타이머 틱 핸들러
	void AdvanceSpiralStep();

	// 특정 격자 좌표에 즉사 블록 투하 및 내부 액터(플레이어/상자 등) 파괴 처리 함수
	void DropDeathBlockAtTile(FIntPoint GridCoord, AActor* WarningActor);

public:	
	virtual void Tick(float DeltaTime) override;

	// 외부에서 나선형 서든데스를 시작시킬 수 있도록 함수 노출
	UFUNCTION(BlueprintCallable, Category = "PlayZone")
	void StartSpiralSuddenDeath();
	
};
