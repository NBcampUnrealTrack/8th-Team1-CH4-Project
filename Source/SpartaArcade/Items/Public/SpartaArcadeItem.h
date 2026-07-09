#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpartaArcadeItem.generated.h"

class USphereComponent;

UENUM(BlueprintType)
enum class ESpartaArcadeItemType : uint8
{
	SpeedBoost     UMETA(DisplayName = "Speed Boost"),
	ExtraBomb      UMETA(DisplayName = "Extra Bomb Count"),
	IncreaseRange  UMETA(DisplayName = "Increase Explosion Range"),
	FirstAidKit    UMETA(DisplayName = "First Aid Kit"),
	Shield         UMETA(DisplayName = "Shield Armor")
};

UCLASS()
class SPARTAARCADE_API ASpartaArcadeItem : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpartaArcadeItem();

protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay", Replicated)
	ESpartaArcadeItemType ItemType;

	// 위아래 둥둥 뜨는 둔탁한 상하 이동 주파수 및 높이 정의
	UPROPERTY(EditAnywhere, Category = "Aesthetics")
	float FloatSpeed;

	UPROPERTY(EditAnywhere, Category = "Aesthetics")
	float FloatHeight;

	// 제자리 회전 각속도
	UPROPERTY(EditAnywhere, Category = "Aesthetics")
	float RotationSpeed;

	FVector StartLocation;

public:	
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
	                    bool bFromSweep, const FHitResult& SweepResult);

	FORCEINLINE ESpartaArcadeItemType GetItemType() const { return ItemType; }

	// 상자 파괴 시 동적으로 아이템 타입을 부여할 수 있도록 Setter 정의
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void SetItemType(ESpartaArcadeItemType InType);
};
