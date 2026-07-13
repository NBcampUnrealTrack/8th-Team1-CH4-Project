// Fill out your copyright notice in the Description page of Project Settings.


#include "Systems/Public/BomberAttributeSet.h"
#include "BomberTypes.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"


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

	Health.SetBaseValue(3.f);
	Health.SetCurrentValue(3.f);

	MaxHealth.SetBaseValue(3.f);
	MaxHealth.SetCurrentValue(3.f);

	CurrentPlacedBombs.SetBaseValue(0.f);
	CurrentPlacedBombs.SetCurrentValue(0.f);
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
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, MaxHealth.GetCurrentValue());
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
	DOREPLIFETIME(UBomberAttributeSet, Health);
	DOREPLIFETIME(UBomberAttributeSet, MaxHealth);
	DOREPLIFETIME(UBomberAttributeSet, CurrentPlacedBombs);
}

void UBomberAttributeSet::InitializeFromDataTable(UDataTable* InCharacterStatTable, FName RowName)
{
	if (!IsValid(InCharacterStatTable))
	{
		UE_LOG(LogTemp, Warning, TEXT("BomberAttributeSet: CharacterStatTable이 없어요!"));
		return;
	}
	
	FCharacterStatRow* Row = InCharacterStatTable->FindRow<FCharacterStatRow>(RowName, TEXT("InitializeFromDataTable"));
	
	if (Row == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("BomberAttributeSet: Row [%s]를 찾을 수 없어요!"),
			*RowName.ToString());
		return;
	}
	
	//순서 고정
	InitMaxBombRange(Row->MaxBombRange);
	InitMaxBombCount(Row->MaxBombCount);
	InitMaxMoveSpeed(Row->MaxMoveSpeed);

	InitBombRange(Row->StartBombRange);
	InitBombCount(Row->StartBombCount);
	InitMoveSpeed(Row->StartMoveSpeed);

	// Init 함수는 PostGameplayEffectExecute를 거치지 않으므로 초기 이동속도를 직접 반영
	ApplyMoveSpeedToMovementComponent();
}

void UBomberAttributeSet::InitializeHealth(int32 StartHearts)
{
	InitMaxHealth(static_cast<float>(StartHearts));
	InitHealth(static_cast<float>(StartHearts));

	// Init 함수는 PostGameplayEffectExecute를 거치지 않으므로 초기 체력을 직접 미러링
	MirrorHealthToPlayerState();
}

void UBomberAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	UE_LOG(LogTemp, Warning, TEXT("[BomberAttributeSet] PostGameplayEffectExecute: %s = %f"),
		*Data.EvaluatedData.Attribute.GetName(), Data.EvaluatedData.Attribute.GetNumericValue(this));

	if (Data.EvaluatedData.Attribute == GetMoveSpeedAttribute())
	{
		ApplyMoveSpeedToMovementComponent();
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		MirrorHealthToPlayerState();
	}

	BroadcastCurrentState();
}

void UBomberAttributeSet::BroadcastCurrentState()
{
	OnStatsChanged.Broadcast(
		FMath::RoundToInt(BombCount.GetCurrentValue()),
		BombRange.GetCurrentValue(),
		MoveSpeed.GetCurrentValue());
}

void UBomberAttributeSet::ApplyMoveSpeedToMovementComponent() const
{
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwningActor()))
	{
		if (UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement())
		{
			const float BaseSpeed = 200.f;
			const float SpeedPerLevel = 100.f;
			MoveComp->MaxWalkSpeed = BaseSpeed + (MoveSpeed.GetCurrentValue() * SpeedPerLevel);
		}
	}
}

void UBomberAttributeSet::MirrorHealthToPlayerState() const
{
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwningActor()))
	{
		if (ASpartaPlayerState* PS = OwnerCharacter->GetPlayerState<ASpartaPlayerState>())
		{
			PS->SetHearts(FMath::RoundToInt(Health.GetCurrentValue()));
		}
	}
}

void UBomberAttributeSet::OnRep_BombRange(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBomberAttributeSet, BombRange, OldValue);
	BroadcastCurrentState();
}

void UBomberAttributeSet::OnRep_BombCount(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBomberAttributeSet, BombCount, OldValue);
	BroadcastCurrentState();
}

void UBomberAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBomberAttributeSet, MoveSpeed, OldValue);
	ApplyMoveSpeedToMovementComponent();
	BroadcastCurrentState();
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

void UBomberAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBomberAttributeSet, Health, OldValue);
	MirrorHealthToPlayerState();
}

void UBomberAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBomberAttributeSet, MaxHealth, OldValue);
}

void UBomberAttributeSet::OnRep_CurrentPlacedBombs(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBomberAttributeSet, CurrentPlacedBombs, OldValue);
}
