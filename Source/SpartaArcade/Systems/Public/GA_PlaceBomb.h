// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_PlaceBomb.generated.h"

class ASpartaArcadeBomb;
class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGAPlaceBombCountChangedSignature, int32, CurrentPlacedBombs);

UCLASS()
class SPARTAARCADE_API UGA_PlaceBomb : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_PlaceBomb();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// BombPlacerComponent에서 흡수한 UI 연동용 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Events | UI")
	FOnGAPlaceBombCountChangedSignature OnCurrentPlacedBombsChanged;

	UFUNCTION(BlueprintPure)
	int32 GetCurrentPlacedBombs() const;

	// HUD 초기 바인딩 직후 현재 상태를 강제로 브로드캐스트하기 위한 함수
	void BroadcastCurrentState();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Bomb")
	TSubclassOf<ASpartaArcadeBomb> BombClass;

private:
	// BombPlacerComponent::CanPlaceBomb 이식
	bool CanPlaceBomb(int32 CurrentCount, int32 MaxBombCount) const;

	// BombPlacerComponent::ServerPlaceBomb_Implementation의 스폰 위치 계산부 이식
	FVector CalcGridSnappedSpawnLocation(AActor* AvatarActor) const;

	// BombPlacerComponent::ServerPlaceBomb_Implementation의 중복 스폰 방지부 이식
	bool IsSpawnLocationOccupied(const FVector& SpawnLocation) const;

	// 스폰한 폭탄의 OnBombExploded 델리게이트에 바인딩되어 카운트를 감소시킴
	void HandleBombExploded();

	// CachedASC를 들고 있지 않으므로 매번 ActorInfo에서 가져와야 함 — HandleBombExploded 등에서 재사용하기 위해 캐싱
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
};
