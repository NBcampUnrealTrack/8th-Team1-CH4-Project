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
	

	// 네트워크 속성 동기화 함수 선언
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// UI 및 HUD 연동을 위한 Getter 함수
	UFUNCTION(BlueprintPure, Category = "Gameplay")
	float GetHP() const;

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	float GetMaxHP() const;

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	bool IsShielded() const;

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	bool IsStunned() const;

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	FORCEINLINE int32 GetFirstAidKitCount() const { return FirstAidKits; }

protected:
	// 캐릭터의 하트 체력, 속도 레벨, 폭탄 소지 한도 및 기절 상태 속성
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Setup")
	ESpartaArcadeCharacterType CharacterType;

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


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Movement")
	float BaseMovementSpeed;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Attributes|Health")
	int32 FirstAidKits;

	// 팀전 구분을 위한 TeamID 속성
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Attributes|Team")
	int32 TeamID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	TSubclassOf<class ASpartaArcadeBomb> BombClass;

	// CombatComponent 측 기절 및 무적 타이머로 통합
	// FTimerHandle StunTimerHandle;
	// FTimerHandle InvulnerableTimerHandle;

private:
	// 연쇄 폭발 다단 히트 차단을 위한 콜리전 복구 타이머 핸들
	FTimerHandle CollisionRestoreTimerHandle;

	// 무시되었던 Visibility 콜리전 채널을 다시 Block 상태로 원상 복구하는 헬퍼 함수
	void RestoreCollisionResponse();
	
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
