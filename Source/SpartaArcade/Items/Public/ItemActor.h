#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BomberTypes.h"
#include "ItemActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UGameplayEffect;

// EBomberItemType별 비주얼 설정(메시, 머티리얼, 스케일 등)을 일괄 정의하기 위한 구조체 정의
USTRUCT(BlueprintType)
struct FItemActorVisualInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> Material = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector Scale = FVector(1.f, 1.f, 1.f);
};

UCLASS()
class SPARTAARCADE_API AItemActor : public AActor
{
	GENERATED_BODY()

public:
	AItemActor();

	virtual void Tick(float DeltaTime) override;

	void InitializeItem(FName InItemRowName, UDataTable* InItemDataTable);

	// BombRange/BombCount/MoveSpeed 등 스탯 강화 아이템 타입별로 적용할 GameplayEffect를 매핑
	// (타입별로 다른 GE를 적용해야 하므로 단일 필드가 아닌 ItemVisualMap과 동일한 TMap 패턴 사용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS")
	TMap<EBomberItemType, TSubclassOf<UGameplayEffect>> ItemEffectMap;

protected:
	virtual void BeginPlay() override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	// 아이템 타입별 비주얼 에셋을 관리할 TMap 추가
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TMap<EBomberItemType, FItemActorVisualInfo> ItemVisualMap;

	UPROPERTY(EditAnywhere, Category = "Aesthetics")
	float FloatSpeed;

	UPROPERTY(EditAnywhere, Category = "Aesthetics")
	float FloatHeight;

	// 제자리 회전 각속도
	UPROPERTY(EditAnywhere, Category = "Aesthetics")
	float RotationSpeed;

	FVector StartLocation;

private:
	// ReplicatedUsing을 적용하여 클라이언트에서도 아이템 데이터 복제 시 비주얼이 동적 갱신되도록 처리
	UPROPERTY(ReplicatedUsing = OnRep_ItemRowName)
	FName ItemRowName;

	// 데이터 테이블 정보도 복제하여 클라이언트 측 비주얼 업데이트를 지원
	UPROPERTY(Replicated)
	TObjectPtr<UDataTable> ItemDataTable;

	// 동적 동기화를 위한 OnRep 함수 및 비주얼 갱신 헬퍼 선언
	UFUNCTION()
	void OnRep_ItemRowName();

	void UpdateItemVisual();
};