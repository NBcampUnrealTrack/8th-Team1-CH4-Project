// Fill out your copyright notice in the Description page of Project Settings.

#include "GA_UseShield.h"
#include "SpartaArcadeCharacter.h"

UGA_UseShield::UGA_UseShield()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_UseShield::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ASpartaArcadeCharacter* Character = Cast<ASpartaArcadeCharacter>(ActorInfo->AvatarActor.Get()))
	{
		Character->PerformUseShield();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
