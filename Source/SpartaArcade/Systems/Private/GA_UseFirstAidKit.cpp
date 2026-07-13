// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_UseFirstAidKit.h"
#include "SpartaArcadeCharacter.h"

UGA_UseFirstAidKit::UGA_UseFirstAidKit()
{
	// 기절 여부에 따라 자력 부활/치료로 내부 분기하므로 기절 상태를 막지 않음

	// 서버에서만
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_UseFirstAidKit::ActivateAbility(
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
		Character->PerformUseFirstAidKit();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
