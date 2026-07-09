// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_PlaceBomb.generated.h"

class ASpartaArcadeBomb;

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
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Bomb")
	TSubclassOf<ASpartaArcadeBomb> BombClass;
};
