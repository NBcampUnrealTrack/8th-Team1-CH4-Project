#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Templates/SubclassOf.h"
#include "Net/UnrealNetwork.h"
#include "SpartaArcadeCharacter.generated.h"

// 캐릭터 스탯 특화 선택을 위한 타입 열거형
UENUM(BlueprintType)
enum class ESpartaArcadeCharacterType : uint8
{
	Explosive      UMETA(DisplayName = "Explosive Specialized"),
	Speed          UMETA(DisplayName = "Speed Specialized"),
	BombCount      UMETA(DisplayName = "Bomb Count Specialized")
};

UCLASS(Blueprintable)
class SPARTAARCADE_API ASpartaArcadeCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASpartaArcadeCharacter();

	virtual void Tick(float DeltaSeconds) override;

	// 기절 구출(아군) 및 처치(적군) 처리를 위한 충돌 오버랩
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	virtual void BeginPlay() override;

public:
	// 하트 라이프, 실드 방어, 기절 처리를 위한 TakeDamage 오버라이드
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	// 폭탄 설치, 구급상자 사용, 폭탄 차기 함수 추가
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void PlaceBomb();

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void UseFirstAidKit();

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void KickBomb();

	// 아이템 획득 및 쿨다운 처리를 위한 상태 함수 추가
	void AddSpeedBoost();
	void AddExtraBomb();
	void IncreaseExplosionRange();
	void AddFirstAidKit();
	void AddShield();
	void OnBombExploded();

	// 기절 복귀 및 무적 처리 함수 추가
	void ReviveCharacter(int32 HealthToRestore);
	void HandleStunTimeout();
	void ResetInvulnerability();

	// 네트워크 속성 동기화 함수 선언
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// UI 및 HUD 연동을 위한 Getter 함수
	UFUNCTION(BlueprintPure, Category = "Gameplay")
	FORCEINLINE float GetHP() const { return (float)Hearts; }

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	FORCEINLINE float GetMaxHP() const { return (float)MaxHearts; }

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	FORCEINLINE bool IsShielded() const { return bIsShielded; }

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	FORCEINLINE bool IsStunned() const { return bIsStunned; }

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	FORCEINLINE int32 GetFirstAidKitCount() const { return FirstAidKits; }

protected:
	// 캐릭터의 하트 체력, 속도 레벨, 폭탄 소지 한도 및 기절 상태 속성
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Setup")
	ESpartaArcadeCharacterType CharacterType;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Attributes|Health")
	int32 Hearts;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Attributes|Health")
	int32 MaxHearts;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Attributes|Movement")
	int32 SpeedLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Movement")
	float BaseMovementSpeed;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Attributes|Bombs")
	int32 MaxBombs;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Attributes|Bombs")
	int32 CurrentActiveBombs;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Attributes|Bombs")
	int32 BombRange;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Attributes|Defense")
	bool bIsShielded;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Attributes|Health")
	int32 FirstAidKits;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Attributes|State")
	bool bIsStunned;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Attributes|State")
	bool bIsInvulnerable;

	// 팀전 구분을 위한 TeamID 속성
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Attributes|Team")
	int32 TeamID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	TSubclassOf<class ASpartaArcadeBomb> BombClass;

	FTimerHandle StunTimerHandle;
	FTimerHandle InvulnerableTimerHandle;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;
};
