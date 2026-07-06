#include "SpartaArcadeItem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "SpartaArcadeCharacter.h"

ASpartaArcadeItem::ASpartaArcadeItem()
{
	PrimaryActorTick.bCanEverTick = true;

	// 데디케이티드 서버를 위한 아이템 복제 활성화
	bReplicates = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->SetSphereRadius(50.f);
	
	// 오버랩 충돌 프로필 활성화
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ItemType = ESpartaArcadeItemType::ExtraBomb;
	
	FloatSpeed = 2.0f;
	FloatHeight = 15.0f;
	RotationSpeed = 45.0f;
}

void ASpartaArcadeItem::BeginPlay()
{
	Super::BeginPlay();
	
	StartLocation = GetActorLocation();
	
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASpartaArcadeItem::OnOverlapBegin);
}

void ASpartaArcadeItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 제자리 둥둥 뜨기 연출
	FVector NewLocation = StartLocation;
	NewLocation.Z += FMath::Sin(GetWorld()->GetTimeSeconds() * FloatSpeed) * FloatHeight;
	SetActorLocation(NewLocation);

	// 제자리 자전 연출
	AddActorLocalRotation(FRotator(0.f, RotationSpeed * DeltaTime, 0.f));
}

void ASpartaArcadeItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpartaArcadeItem, ItemType);
}

void ASpartaArcadeItem::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
                                      bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasAuthority() && OtherActor && OtherActor != this)
	{
		ASpartaArcadeCharacter* Character = Cast<ASpartaArcadeCharacter>(OtherActor);
		if (Character)
		{
			// 수집 아이템에 따른 스탯 버프 반영
			switch (ItemType)
			{
			case ESpartaArcadeItemType::SpeedBoost:
				Character->AddSpeedBoost();
				break;
			case ESpartaArcadeItemType::ExtraBomb:
				Character->AddExtraBomb();
				break;
			case ESpartaArcadeItemType::IncreaseRange:
				Character->IncreaseExplosionRange();
				break;
			case ESpartaArcadeItemType::FirstAidKit:
				Character->AddFirstAidKit();
				break;
			case ESpartaArcadeItemType::Shield:
				Character->AddShield();
				break;
			default:
				break;
			}
			Destroy();
		}
	}
}

// 블록에서 아이템 생성 시 생성 아이템의 종류를 지정하기 위한 Setter 함수 구현
void ASpartaArcadeItem::SetItemType(ESpartaArcadeItemType InType)
{
	if (HasAuthority())
	{
		ItemType = InType;
	}
}