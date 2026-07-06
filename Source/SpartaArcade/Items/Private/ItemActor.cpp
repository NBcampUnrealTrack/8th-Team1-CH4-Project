#include "ItemActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "StatComponent.h"

AItemActor::AItemActor()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    SetRootComponent(CollisionSphere);
    CollisionSphere->SetSphereRadius(50.f);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    ItemMesh->SetupAttachment(CollisionSphere);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    bReplicates = true;
}

void AItemActor::BeginPlay()
{
    Super::BeginPlay();
}

void AItemActor::InitializeItem(FName InItemRowName, UDataTable* InItemDataTable)
{
    ItemRowName = InItemRowName;
    ItemDataTable = InItemDataTable;

    // ItemType에 따라 ItemMesh의 StaticMesh를 다르게 설정
}

void AItemActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
    Super::NotifyActorBeginOverlap(OtherActor);

    if (!HasAuthority()) return;

    if (!IsValid(ItemDataTable)) return;

    UStatComponent* StatComp = OtherActor->FindComponentByClass<UStatComponent>();
    if (!IsValid(StatComp)) return;

    FItemDataRow* Row = ItemDataTable->FindRow<FItemDataRow>(
        ItemRowName, TEXT("NotifyActorBeginOverlap"));

    if (Row == nullptr) return;

    if (Row->ItemType == EBomberItemType::None)
    {
        Destroy();
        return;
    }

    switch (Row->ItemType)
    {
    case EBomberItemType::BombRange:
        StatComp->GrowStat(EBomberStatType::BombRange);
        break;

    case EBomberItemType::BombCount:
        StatComp->GrowStat(EBomberStatType::BombCount);
        break;

    case EBomberItemType::MoveSpeed:
        StatComp->GrowStat(EBomberStatType::MoveSpeed);
        break;

    case EBomberItemType::MedKit:
        // ombatComponent 만들면 하트 회복 로직 연결
        break;

    case EBomberItemType::Shield:
        // CombatComponent 만들면 방어막 부여 로직 연결
        break;

    default:
        break;
    }

    Destroy();
}