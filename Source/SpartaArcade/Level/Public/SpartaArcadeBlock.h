#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/SubclassOf.h"
#include "SpartaArcadeBlock.generated.h"

UCLASS()
class SPARTAARCADE_API ASpartaArcadeBlock : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpartaArcadeBlock();

protected:
	virtual void BeginPlay() override;

public:
	// 폭발 화염 접촉 시 상자를 부수고 소멸시키는 인터페이스 함수
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void DestroyBlock();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	// 파괴될 시 생성시킬 기본 아이템 클래스 타입
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSubclassOf<AActor> ItemSpawnClass;

	// 아이템을 드랍시킬 확률 (0.0 ~ 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ItemSpawnChance;
};
