#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SpartaPlayerState.generated.h"

// 체력
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeartsChangedSignature, int32, CurrentHearts, int32, MaxHearts);

// 기절
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStunStateChangedSignature, bool, bIsActive);

// Modified: 구급약 보유 개수 변화 통지용 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFirstAidKitsChangedSignature, int32, NewCount);

enum class ESpartaArcadeCharacterType : uint8;
enum class EBomberPlayerState : uint8;
/**
 * 
 */
UCLASS()
class SPARTAARCADE_API ASpartaPlayerState : public APlayerState
{
	GENERATED_BODY()
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// -------------------------------------------------------
	// Characters
	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetCharacterType(ESpartaArcadeCharacterType NewType);
	UFUNCTION(BlueprintCallable, Category = "Character")
	ESpartaArcadeCharacterType GetCharacterType() const;

	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetFirstAidKits(int32 NewCount);
	UFUNCTION(BlueprintCallable, Category = "Character")
	int32 GetFirstAidKits() const;

	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetTeamID(int32 NewTeamID);
	UFUNCTION(BlueprintCallable, Category = "Character")
	int32 GetTeamID() const;

	// -------------------------------------------------------
	// CombatComponent
	UFUNCTION(BlueprintCallable, Category = "CombatComponent")
	void SetHearts(int32 NewHearts);
	UFUNCTION(BlueprintCallable, Category = "CombatComponent")
	int32 GetHearts() const;

	UFUNCTION(BlueprintCallable, Category = "CombatComponent")
	void SetCurrentState(EBomberPlayerState NewState);
	UFUNCTION(BlueprintCallable, Category = "CombatComponent")
	EBomberPlayerState GetCurrentState() const;

	UFUNCTION(BlueprintCallable, Category = "CombatComponent")
	void SetStartHearts(int32 NewStartHearts);
	UFUNCTION(BlueprintCallable, Category = "CombatComponent")
	int32 GetStartHearts() const;

	UFUNCTION(BlueprintCallable, Category = "CombatComponent")
	void SetSelfReviveHearts(int32 NewSelfReviveHearts);
	UFUNCTION(BlueprintCallable, Category = "CombatComponent")
	int32 GetSelfReviveHearts() const;

	UFUNCTION()
	void OnRep_Hearts();
	UFUNCTION()
	void OnRep_CurrentState();
	UFUNCTION()
	void OnRep_FirstAidKits();

	void BroadcastCurrentState();

	// -------------------------------------------------------
	// Delegates
	// 체력 
	UPROPERTY(BlueprintAssignable, Category = "Events | UI")
	FOnHeartsChangedSignature OnHeartsChanged;

	// 기절
	UPROPERTY(BlueprintAssignable, Category = "Events | UI")
	FOnStunStateChangedSignature OnStunStateChanged;

	// 구급약 개수 변동 델리게이트 변수 정의
	UPROPERTY(BlueprintAssignable, Category = "Events | UI")
	FOnFirstAidKitsChangedSignature OnFirstAidKitsChanged;

protected:

	// -------------------------------------------------------
	// Characters
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	ESpartaArcadeCharacterType CharacterType;

	// 구급상자 초기 보유 개수
	UPROPERTY(ReplicatedUsing = OnRep_FirstAidKits, VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	int32 FirstAidKits = 0;

	// 팀전 구분을 위한 TeamID 속성
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Character")
	int32 TeamID;

	// -------------------------------------------------------
	// CombatComponent
	UPROPERTY(ReplicatedUsing = OnRep_Hearts, VisibleAnywhere, BlueprintReadOnly, Category = "CombatComponent")
	int32 Hearts;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentState, VisibleAnywhere, BlueprintReadOnly, Category = "CombatComponent")
	EBomberPlayerState CurrentState;

	int32 StartHearts = 3;
	int32 SelfReviveHearts = 1;
};
