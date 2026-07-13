// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_PlaceBomb.h"
#include "SpartaArcadeBomb.h"
#include "BomberGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "BomberAttributeSet.h"
#include "SpartaArcadeCharacter.h"

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
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!IsValid(AvatarActor) || !IsValid(BombClass))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	ASpartaArcadeCharacter* Character = Cast<ASpartaArcadeCharacter>(AvatarActor);
	if (!IsValid(Character))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	// AttributeSet에서 폭발 범위 읽어오기
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const UBomberAttributeSet* AttrSet = ASC->GetSet<UBomberAttributeSet>();
	if (!IsValid(AttrSet))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	int32 BombRange = FMath::RoundToInt(AttrSet->GetBombRange());

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASpartaArcadeBomb* NewBomb = AvatarActor->GetWorld()->SpawnActor<ASpartaArcadeBomb>(
		BombClass,
		AvatarActor->GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (IsValid(NewBomb))
	{
		NewBomb->InitializeBomb(Character, BombRange);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}