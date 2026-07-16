#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Templates/SubclassOf.h"
#include "Net/UnrealNetwork.h"
#include "Damageable.h"
#include "AbilitySystemInterface.h"
#include "UI/Public/SpartaUIDefs.h" 
#include "Components/WidgetComponent.h"
#include "SpartaArcadeCharacter.generated.h"

// 컴포넌트 의존 관계 설정을 위한 전방 선언
class UCombatComponent;
class UDeathDropComponent;
class UDataTable;
class ASpartaPlayerState;
class UAbilitySystemComponent;   
class UBomberAttributeSet;       
class UGameplayAbility;          
class UWidgetComponent;

// 캐릭터 스탯 특화 선택을 위한 타입 열거형
UENUM(BlueprintType)
enum class ESpartaArcadeCharacterType : uint8
{
	Explosive      UMETA(DisplayName = "화력광"),
	Speed          UMETA(DisplayName = "속도광"),
	BombCount      UMETA(DisplayName = "폭탄광")
};

// 폭탄 연계 대미지 처리를 위한 IDamageable 인터페이스 상속 추가
UCLASS(Blueprintable)
class SPARTAARCADE_API ASpartaArcadeCharacter : public ACharacter, public IDamageable, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASpartaArcadeCharacter();

	// 매 프레임 캐릭터 및 씬 캡쳐 상태 제어를 위한 Tick 오버라이드
	virtual void Tick(float DeltaSeconds) override;

	// IDamageable 인터페이스 구현 선언
	virtual void TakeExplosionDamage_Implementation() override;
	virtual bool CanTakeDamage_Implementation() const override;
	virtual bool BlocksExplosion_Implementation() const override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	// 기절 구출(아군) 및 처치(적군) 처리를 위한 충돌 오버랩
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	virtual void BeginPlay() override;
	
	virtual void PossessedBy(AController* NewController) override;

	// 피격 시 블루프린트에서 깜빡임 등 시각 연출을 처리할 수 있도록 선언한 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Sparta|Effects")
	void OnHitFlash();
	
	void InitializeCharacterComponents();
	
	virtual void OnRep_PlayerState() override;

public:
	// 하트 라이프, 실드 방어, 기절 처리를 위한 TakeDamage 오버라이드
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	// 폭탄 설치, 구급상자 사용, 폭탄 차기 함수 추가
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void PlaceBomb();

	void PlayPlaceBombAnim();

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void UseFirstAidKit();
	
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void UseShield();
	
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void KickBomb();

	// 아이템 획득 및 쿨다운 처리를 위한 상태 함수 추가
	void AddFirstAidKit();
	void AddShield();
	void OnBombExploded();
	void PerformUseShield();
	void UnlockKickBomb();

	void UpdateNickname(); 

	void ShowMatchResultUI(EMatchResult Result);

	// UI 및 HUD 연동을 위한 Getter 함수
	UFUNCTION(BlueprintPure, Category = "Gameplay")
	float GetHP() const;

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	float GetMaxHP() const;

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	bool IsShielded() const;

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	bool IsStunned() const;

	// GA_KickBomb / GA_UseFirstAidKit 어빌리티가 호출하는 실제 로직 (CombatComponent 등 protected 멤버 접근을 위해 Character에 유지)
	void PerformKickBomb();
	void PerformUseFirstAidKit();

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
	TObjectPtr<UCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDeathDropComponent> DeathDropComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> NicknameWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<ASpartaPlayerState> SpartaPlayerState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Movement")
	float BaseMovementSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes|Health")
	int32 FirstAidKits;

	// 팀전 구분을 위한 TeamID 속성
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Team")
	int32 TeamID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	TSubclassOf<class ASpartaArcadeBomb> BombClass;

	// 애니메이션 몽타주 에셋 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes|Animation")
	TObjectPtr<UAnimMontage> PlaceBombMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes|Animation")
	TObjectPtr<UAnimMontage> KickBombMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBomberAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> PlaceBombAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> KickBombAbilityClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> UseShieldAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> UseFirstAidKitAbilityClass;

protected:
	// 지형 타일 효과를 이동에 반영하는 처리 함수
	void ApplyTileEffectToMovement(float DeltaSeconds);

	// 맵 빌더 캐싱용 포인터
	UPROPERTY()
	class ASpartaArcadeMapBuilder* CachedMapBuilder;

	// 캐릭터의 기본 무브먼트 값 백업용
	float DefaultMaxWalkSpeed;
	float DefaultGroundFriction;
	float DefaultBrakingDeceleration;

	// 지형 효과 밸런싱 변수 (디렉토리 디테일 패널 개방)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|TileEffects")
	float MudSpeedMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|TileEffects")
	float IceFriction = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|TileEffects")
	float IceBrakingDeceleration = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|TileEffects")
	float ConveyorPushSpeed = 250.f;

	// 지연 사망 소멸 처리용 변수 및 함수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes|Death")
	float DestroyDelay = 1.5f;

	FTimerHandle DestroyTimerHandle;

	// 타이머 만료 시 최종적으로 Destroy()를 호출할 함수
	void EliminateDestroy();
	
	// 폭발형 전용 머티리얼 인스턴스 (Red)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals|Material")
	TObjectPtr<UMaterialInstance> ExplosiveMaterial;

	// 속도형 전용 머티리얼 인스턴스 (Blue)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals|Material")
	TObjectPtr<UMaterialInstance> SpeedMaterial;

	// 폭탄갯수형 전용 머티리얼 인스턴스 (Green)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals|Material")
	TObjectPtr<UMaterialInstance> BombCountMaterial;

private:
	// 연쇄 폭발 다단 히트 차단을 위한 콜리전 복구 타이머 핸들
	FTimerHandle CollisionRestoreTimerHandle;

	bool bMatchResultShown = false;

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

	int32 MaxInitializedComponentsCount;
	int32 InitializedComponentsCount;

	bool bComponentsInitialized = false;
};
