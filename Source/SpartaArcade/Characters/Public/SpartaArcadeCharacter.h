#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Templates/SubclassOf.h"
#include "Net/UnrealNetwork.h"
#include "SpartaArcadeCharacter.generated.h"

// 컴포넌트 의존 관계 설정을 위한 전방 선언
class UStatComponent;
class UCombatComponent;
class UBombPlacerComponent;
class UDataTable;
class ASpartaPlayerState;

// 캐릭터 스탯 특화 선택을 위한 타입 열거형
UENUM(BlueprintType)
enum class ESpartaArcadeCharacterType : uint8
{
	Explosive      UMETA(DisplayName = "폭발형"),
	Speed          UMETA(DisplayName = "속도형"),
	BombCount      UMETA(DisplayName = "폭탄 갯수형")
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

	void InitializeCharacterComponents();

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

	// UI 및 HUD 연동을 위한 Getter 함수
	UFUNCTION(BlueprintPure, Category = "Gameplay")
	float GetHP() const;

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	float GetMaxHP() const;

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	bool IsShielded() const;

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	bool IsStunned() const;

protected:
	// 컴포넌트 초기화를 위한 데이터 테이블 구조 노출
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes|Setup")
	TObjectPtr<UDataTable> CharacterStatTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes|Setup")
	TObjectPtr<UDataTable> CombatStatTable;

	// 중복 코드 및 의존 관계 정리를 위해 컴포넌트 추가
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStatComponent> StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBombPlacerComponent> BombPlacerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<ASpartaPlayerState> SpartaPlayerState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Movement")
	float BaseMovementSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	TSubclassOf<class ASpartaArcadeBomb> BombClass;

	int32 MaxInitializedComponentsCount;
	int32 InitializedComponentsCount;

	// CombatComponent 측 기절 및 무적 타이머로 통합
	// FTimerHandle StunTimerHandle;
	// FTimerHandle InvulnerableTimerHandle;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;
	
	FVector GetSnappedKickDirection() const;

	// CombatComponent 이벤트 대응 핸들러 추가
	UFUNCTION()
	void HandleOnStun();

	UFUNCTION()
	void HandleOnRevived();

	UFUNCTION()
	void HandleOnSelfRevive();

	UFUNCTION()
	void HandleOnEliminated();
};
