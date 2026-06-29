#include "SpartaArcadeBlock.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "SpartaArcadeItem.h"

ASpartaArcadeBlock::ASpartaArcadeBlock()
{
	PrimaryActorTick.bCanEverTick = false;

	// Modified: 데디케이티드 서버를 위한 상자 블록 복제 활성화
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	// 플레이어 및 폭발을 블로킹할 수 있도록 콜리전 프로필 설정
	MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
	
	// 기본 확률 값 설정
	ItemSpawnChance = 0.4f;
}

void ASpartaArcadeBlock::BeginPlay()
{
	Super::BeginPlay();
}

// 아이템 랜덤 스폰
void ASpartaArcadeBlock::DestroyBlock()
{
	if (ItemSpawnClass)
	{
		float Roll = FMath::FRandRange(0.f, 100.f);
		ESpartaArcadeItemType ChosenType = ESpartaArcadeItemType::ExtraBomb;
		bool bShouldSpawn = false;

		// 38% 빈 상자, 25% 폭탄, 30.4% 범위, 3.4% 구급약, 1.6% 속도, 1.6% 실드
		if (Roll < 38.0f)
		{
			bShouldSpawn = false;
		}
		else if (Roll < 63.0f)
		{
			ChosenType = ESpartaArcadeItemType::ExtraBomb;
			bShouldSpawn = true;
		}
		else if (Roll < 93.4f)
		{
			ChosenType = ESpartaArcadeItemType::IncreaseRange;
			bShouldSpawn = true;
		}
		else if (Roll < 96.8f)
		{
			ChosenType = ESpartaArcadeItemType::FirstAidKit;
			bShouldSpawn = true;
		}
		else if (Roll < 98.4f)
		{
			ChosenType = ESpartaArcadeItemType::SpeedBoost;
			bShouldSpawn = true;
		}
		else
		{
			ChosenType = ESpartaArcadeItemType::Shield;
			bShouldSpawn = true;
		}

		if (bShouldSpawn)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			
			FVector SpawnLocation = GetActorLocation();
			FRotator SpawnRotation = FRotator::ZeroRotator;
			
			AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(ItemSpawnClass, SpawnLocation, SpawnRotation, SpawnParams);
			ASpartaArcadeItem* Item = Cast<ASpartaArcadeItem>(SpawnedActor);
			if (Item)
			{
				Item->SetItemType(ChosenType);
			}
		}
	}

	Destroy();
}
