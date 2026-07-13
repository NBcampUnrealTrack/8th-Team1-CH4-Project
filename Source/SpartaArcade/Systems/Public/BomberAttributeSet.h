// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BomberAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName) 

UCLASS()
class SPARTAARCADE_API UBomberAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UBomberAttributeSet();
	
	// 기존 StatComponent의 값

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_BombRange, Category="Stats")
	FGameplayAttributeData BombRange;
	ATTRIBUTE_ACCESSORS(UBomberAttributeSet, BombRange)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_BombCount, Category="Stats")
	FGameplayAttributeData BombCount;
	ATTRIBUTE_ACCESSORS(UBomberAttributeSet, BombCount)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MoveSpeed, Category="Stats")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UBomberAttributeSet, MoveSpeed)

	// 상한값도 Attribute로 관리 
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxBombRange, Category="Stats")
	FGameplayAttributeData MaxBombRange;
	ATTRIBUTE_ACCESSORS(UBomberAttributeSet, MaxBombRange)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxBombCount, Category="Stats")
	FGameplayAttributeData MaxBombCount;
	ATTRIBUTE_ACCESSORS(UBomberAttributeSet, MaxBombCount)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxMoveSpeed, Category="Stats")
	FGameplayAttributeData MaxMoveSpeed;
	ATTRIBUTE_ACCESSORS(UBomberAttributeSet, MaxMoveSpeed)

	// 상한 클램프 처리 
	virtual void PreAttributeChange(
		const FGameplayAttribute& Attribute, float& NewValue) override;

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable)
	void InitializeFromDataTable(UDataTable* InCharacterStatTable, FName RowName);

protected:
	UFUNCTION() void OnRep_BombRange(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_BombCount(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxBombRange(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxBombCount(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxMoveSpeed(const FGameplayAttributeData& OldValue);
};
