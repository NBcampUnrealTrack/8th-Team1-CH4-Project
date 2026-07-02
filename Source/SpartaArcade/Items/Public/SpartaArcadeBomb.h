#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpartaArcadeBomb.generated.h"

class ASpartaArcadeCharacter;
class UNiagaraSystem;

UCLASS()
class SPARTAARCADE_API ASpartaArcadeBomb : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpartaArcadeBomb();
	
	virtual void Tick(float DeltaTime) override;

	// 캐릭터에 의해 발차기 되었을 때 굴러가기 시작하는 제어 함수
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void Kick(const FVector& Direction);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	FORCEINLINE bool IsRolling() const { return bIsRolling; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	// 폭발 대기 타이머 초
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float ExplosionDelay;

	// 폭풍 화염 사거리 칸 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	int32 FirePower;

	// 격자 한 칸의 가로/세로 규격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float GridSize;

	// 폭발 데미지 (하트 1개 차감 기준)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float ExplosionDamage;

	// 폭발 나이아가라 이펙트 에셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	UNiagaraSystem* ExplosionVFX;

	FTimerHandle ExplosionTimerHandle;

	// 폭탄을 배치한 주동자 캐릭터
	UPROPERTY()
	ASpartaArcadeCharacter* InstigatorCharacter;

	// 굴리기 운동 상태를 추적하기 위한 변수 추가
	bool bIsRolling;
	FVector RollDirection;
	
	// 중복 폭발로 인한 Stack Overflow 방지 플래그
	bool bIsExploded;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay|Rolling")
	float RollSpeed;

	// 특정 방향으로 폭발 광선을 전파하여 데미지/파괴 처리하는 서브 로직
	void PerformExplosionDirection(const FVector& Direction);
	
	void ApplyCenterDamage(const FVector& Center);
	bool HandleExplosionHit(AActor* HitActor);

public:
	//  유폭 연쇄 처리를 위한 폭발 실행 함수
	void Explode();

	void InitializeBomb(ASpartaArcadeCharacter* InInstigator, int32 InFirePower);
};
