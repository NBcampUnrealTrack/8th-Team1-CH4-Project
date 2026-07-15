#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpartaArcadeBomb.generated.h"

class ASpartaArcadeCharacter;
class UNiagaraSystem;

// UBombPlacerComponent 등 외부 연동을 위한 폭발 델리게이트 선언
DECLARE_DELEGATE(FOnBombExploded);

UCLASS()
class SPARTAARCADE_API ASpartaArcadeBomb : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpartaArcadeBomb();

	// 폭발 완료 델리게이트 멤버 추가
	FOnBombExploded OnBombExploded;
	
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

	// 구형 케스케이드 파티클 시스템 멤버 속성
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	UParticleSystem* ExplosionCascadeVFX;

	// 폭발 시 재생할 사운드 에셋 속성
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	class USoundBase* ExplosionSound;

	// 폭발 좌표를 저장하여 해당 좌표에서 애니메이션 재생
	TArray<FVector> ExplosionLocations;

	// Modified: 2D 거리 수동 판정을 기반으로 충돌 무시를 관리할 캐릭터 목록 추가
	UPROPERTY()
	TArray<class ASpartaArcadeCharacter*> IgnoredCharacters;

	FTimerHandle ExplosionTimerHandle;

	// 폭탄을 배치한 주동자 캐릭터
	UPROPERTY()
	ASpartaArcadeCharacter* InstigatorCharacter;

	// 굴리기 운동 상태를 추적하기 위한 변수
	bool bIsRolling;
	FVector RollDirection;
	
	// 중복 폭발로 인한 Stack Overflow 방지 플래그
	bool bIsExploded;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay|Rolling")
	float RollSpeed;

	// 특정 방향으로 폭발 광선을 전파하여 데미지/파괴 처리하는 서브 로직
	void PerformExplosionDirection(const FVector& Direction);

private:
	// 공통 단일 스윕 피격 연산 및 대미지 적용 함수 (장애물 충돌 여부 반환)
	bool SweepAndApplyDamage(const FVector& Start, const FVector& End, float Radius);

	// 충돌한 액터의 타입에 따른 반응 분기 처리 함수 (확장 판단 전담)
	bool HandleExplosionHit(AActor* HitActor);

	// 폭발 이펙트 재생 멀티캐스트 RPC
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayExplosionEffects(const TArray<FVector>& Locations);

	// 이번 폭발에서 중복 피격을 방지하며 대상에게 대미지를 주는 통합 함수
	void ApplyExplosionDamage(AActor* Target);

	// 이번 폭발 과정에서 이미 대미지를 입은 액터들의 목록 (중복 피격 방지용)
	UPROPERTY()
	TArray<AActor*> DamagedActors;

public:
	//  유폭 연쇄 처리를 위한 폭발 실행 함수
	void Explode();

	void InitializeBomb(ASpartaArcadeCharacter* InInstigator, int32 InFirePower);
};
