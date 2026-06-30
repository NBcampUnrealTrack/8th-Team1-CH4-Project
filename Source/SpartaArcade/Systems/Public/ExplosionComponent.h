#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExplosionComponent.generated.h"

// 폭발 탐지 중 한 칸을 처리한 결과
enum class EExplosionHitResult : uint8
{
	None,     // 빈 칸, 계속 진행
	Hit,      // 무언가 맞았지만 계속 진행 가능
	Blocked   // 벽/박스 등으로 막힘, 이 방향 탐지 중단
};

UCLASS(ClassGroup=(Bomber), meta=(BlueprintSpawnableComponent))
class SPARTAARCADE_API UExplosionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UExplosionComponent();
	
	UFUNCTION(BlueprintCallable)
	void StartExplosion(int32 ExplosionRange, float TileSize);

private:
	void DetectInDirection(const FVector& Direction, int32 Range, float TileSize);

	// 특정 위치에서 겹치는 액터들을 찾아서 처리
	bool ProcessLocation(const FVector& Location);
};