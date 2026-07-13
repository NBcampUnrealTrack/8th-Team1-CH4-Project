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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStatsChangedSignature, int32, NewBombCount, float, NewBombRange, float, NewMoveSpeed);

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

	// 기존 SpartaPlayerState::Hearts를 대체하는 체력 Attribute
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="Combat")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UBomberAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category="Combat")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UBomberAttributeSet, MaxHealth)

	// GA_PlaceBomb에서 현재 설치된 폭탄 개수를 추적하기 위한 Attribute
	// (GameplayAbility 인스턴스에 Replicated 프로퍼티를 직접 두는 방식은 deprecated이므로 AttributeSet으로 이전)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentPlacedBombs, Category="Combat")
	FGameplayAttributeData CurrentPlacedBombs;
	ATTRIBUTE_ACCESSORS(UBomberAttributeSet, CurrentPlacedBombs)

	// 상한 클램프 처리
	virtual void PreAttributeChange(
		const FGameplayAttribute& Attribute, float& NewValue) override;

	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void InitializeFromDataTable(UDataTable* InCharacterStatTable, FName RowName);

	// CombatComponent가 CombatStatTable의 StartHearts로 체력 Attribute를 초기화할 때 호출
	UFUNCTION(BlueprintCallable)
	void InitializeHealth(int32 StartHearts);

	// HUD 초기 바인딩 직후 현재 상태를 강제로 브로드캐스트하기 위한 함수
	void BroadcastCurrentState();

	UPROPERTY(BlueprintAssignable, Category = "Events | UI")
	FOnStatsChangedSignature OnStatsChanged;

private:
	// MoveSpeed 값에 맞춰 CharacterMovementComponent의 MaxWalkSpeed를 갱신하는 헬퍼
	void ApplyMoveSpeedToMovementComponent() const;

	// Health 값을 기존 SpartaPlayerState::Hearts(HUD용)로 미러링하는 헬퍼
	void MirrorHealthToPlayerState() const;

protected:
	UFUNCTION() void OnRep_BombRange(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_BombCount(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxBombRange(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxBombCount(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxMoveSpeed(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_CurrentPlacedBombs(const FGameplayAttributeData& OldValue);
};
