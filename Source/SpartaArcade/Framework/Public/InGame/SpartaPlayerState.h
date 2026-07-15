#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SpartaPlayerState.generated.h"

// 체력
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeartsChangedSignature, int32, CurrentHearts, int32, MaxHearts);

// 기절
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStunStateChangedSignature, bool, bIsActive);

// 구급약 보유 개수 변화 통지용 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFirstAidKitsChangedSignature, int32, NewCount);

//실드
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShieldsChangedSignature, int32, NewCount);

// 발차기 활성화 여부 변경 통지용 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKickUnlockedChangedSignature, bool, bIsUnlocked);

enum class ESpartaArcadeCharacterType : uint8;
enum class EBomberPlayerState : uint8;

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
	void SetShields(int32 NewCount);  
	UFUNCTION(BlueprintCallable, Category = "Character")
	int32 GetShields() const;

	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetFirstAidKits(int32 NewCount);
	UFUNCTION(BlueprintCallable, Category = "Character")
	int32 GetFirstAidKits() const;

	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetTeamID(int32 NewTeamID);
	UFUNCTION(BlueprintCallable, Category = "Character")
	int32 GetTeamID() const;

	// 발차기 활성화 함수 및 반환 함수 추가
	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetKickUnlocked(bool bUnlocked);
	UFUNCTION(BlueprintPure, Category = "Character")
	bool IsKickUnlocked() const;

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
	void OnRep_Shields();
	UFUNCTION()
	void OnRep_CurrentState();
	UFUNCTION()
	void OnRep_FirstAidKits();
	
	// 발차기 상태 복제 노티파이 함수 추가
	UFUNCTION()
	void OnRep_bIsKickUnlocked();

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
	
	// 실드
	UPROPERTY(BlueprintAssignable, Category = "Events | UI")
	FOnShieldsChangedSignature OnShieldsChanged;

	// 발차기 활성화 변경 통지용 델리게이트 변수 추가
	UPROPERTY(BlueprintAssignable, Category = "Events | UI")
	FOnKickUnlockedChangedSignature OnKickUnlockedChanged;

protected:

	// -------------------------------------------------------
	// Characters
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Character")
	ESpartaArcadeCharacterType CharacterType;

	// 구급상자 초기 보유 개수
	UPROPERTY(ReplicatedUsing = OnRep_FirstAidKits, VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	int32 FirstAidKits = 0;
	
	UPROPERTY(ReplicatedUsing = OnRep_Shields, VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	int32 Shields = 0;

	// 팀전 구분을 위한 TeamID 속성
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Character")
	int32 TeamID;

	// 발차기 기능 잠금해제 여부 동기화 변수 추가
	UPROPERTY(ReplicatedUsing = OnRep_bIsKickUnlocked, VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	bool bIsKickUnlocked = false;

	// -------------------------------------------------------
	// CombatComponent
	UPROPERTY(ReplicatedUsing = OnRep_Hearts, VisibleAnywhere, BlueprintReadOnly, Category = "CombatComponent")
	int32 Hearts;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentState, VisibleAnywhere, BlueprintReadOnly, Category = "CombatComponent")
	EBomberPlayerState CurrentState;

	int32 StartHearts = 3;
	int32 SelfReviveHearts = 1;
};
