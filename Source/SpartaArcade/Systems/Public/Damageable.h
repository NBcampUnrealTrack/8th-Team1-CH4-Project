// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Damageable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UDamageable : public UInterface
{
	GENERATED_BODY()
};

class SPARTAARCADE_API IDamageable
{
	GENERATED_BODY()

public:
	// 폭발에 의한 피해 처리 (플레이어는 CombatComponent로, 박스는 파괴 로직으로 분기)
	UFUNCTION(BlueprintNativeEvent)
	void TakeExplosionDamage();

	// 현재 피해를 받을 수 있는 상태인지
	UFUNCTION(BlueprintNativeEvent)
	bool CanTakeDamage() const;

	// 이 액터가 폭발을 막는지 여부
	UFUNCTION(BlueprintNativeEvent)
	bool BlocksExplosion() const;
};

