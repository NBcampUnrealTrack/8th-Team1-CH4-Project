// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_PlaceBomb.h"
#include "SpartaArcadeBomb.h"
#include "BomberGameplayTags.h"
#include "AbilitySystemComponent.h"

UGA_PlaceBomb::UGA_PlaceBomb()
{
	// 기절 중엔 발동 X
	ActivationBlockedTags.AddTag(BomberGameplayTags::State_Stunned);

	// 서버에서만
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_PlaceBomb::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!IsValid(AvatarActor) || !IsValid(BombClass))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASpartaArcadeBomb* NewBomb = AvatarActor->GetWorld()->SpawnActor<ASpartaArcadeBomb>(
		BombClass,
		AvatarActor->GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams
	);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}