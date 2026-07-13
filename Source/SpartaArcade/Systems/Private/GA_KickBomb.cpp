// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_KickBomb.h"
#include "BomberGameplayTags.h"
#include "SpartaArcadeCharacter.h"

UGA_KickBomb::UGA_KickBomb()
{
	// 기절 중엔 발동 X
	ActivationBlockedTags.AddTag(BomberGameplayTags::State_Stunned);

	// 서버에서만
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_KickBomb::ActivateAbility(
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
		Character->PerformKickBomb();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
