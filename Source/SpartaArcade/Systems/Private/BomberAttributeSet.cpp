// Fill out your copyright notice in the Description page of Project Settings.


#include "Systems/Public/BomberAttributeSet.h"
#include "Net/UnrealNetwork.h"


UBomberAttributeSet::UBomberAttributeSet()
{
	BombRange.SetBaseValue(2.f);
	BombRange.SetCurrentValue(2.f);

	BombCount.SetBaseValue(3.f);
	BombCount.SetCurrentValue(3.f);

	MoveSpeed.SetBaseValue(3.f);
	MoveSpeed.SetCurrentValue(3.f);

	MaxBombRange.SetBaseValue(5.f);
	MaxBombRange.SetCurrentValue(5.f);

	MaxBombCount.SetBaseValue(8.f);
	MaxBombCount.SetCurrentValue(8.f);

	MaxMoveSpeed.SetBaseValue(5.f);
	MaxMoveSpeed.SetCurrentValue(5.f);
}

void UBomberAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	// 상한을 넘지 않도록 클램프
	if (Attribute == GetBombRangeAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, MaxBombRange.GetCurrentValue());
	}
	else if (Attribute == GetBombCountAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, MaxBombCount.GetCurrentValue());
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, MaxMoveSpeed.GetCurrentValue());
	}
}

void UBomberAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UBomberAttributeSet, BombRange);
	DOREPLIFETIME(UBomberAttributeSet, BombCount);
	DOREPLIFETIME(UBomberAttributeSet, MoveSpeed);
	DOREPLIFETIME(UBomberAttributeSet, MaxBombRange);
	DOREPLIFETIME(UBomberAttributeSet, MaxBombCount);
	DOREPLIFETIME(UBomberAttributeSet, MaxMoveSpeed);
}

void UBomberAttributeSet::OnRep_BombRange(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBomberAttributeSet, BombRange, OldValue);
}

void UBomberAttributeSet::OnRep_BombCount(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBomberAttributeSet, BombCount, OldValue);
}

void UBomberAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBomberAttributeSet, MoveSpeed, OldValue);
}

void UBomberAttributeSet::OnRep_MaxBombRange(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBomberAttributeSet, MaxBombRange, OldValue);
}

void UBomberAttributeSet::OnRep_MaxBombCount(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBomberAttributeSet, MaxBombCount, OldValue);
}

void UBomberAttributeSet::OnRep_MaxMoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBomberAttributeSet, MaxMoveSpeed, OldValue);
}
