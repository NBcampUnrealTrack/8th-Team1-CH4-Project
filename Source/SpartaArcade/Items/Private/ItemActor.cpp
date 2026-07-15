#include "ItemActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ItemDropComponent.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "SpartaArcadeCharacter.h"

AItemActor::AItemActor()
{
    // Aesthetics 연출을 위해 Tick을 활성화
    PrimaryActorTick.bCanEverTick = true;

    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    SetRootComponent(CollisionSphere);
    CollisionSphere->SetSphereRadius(50.f);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CollisionSphere->SetMobility(EComponentMobility::Movable);

    ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    ItemMesh->SetupAttachment(CollisionSphere);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    bReplicates = true;

    FloatSpeed = 2.0f;
    FloatHeight = 15.0f;
    RotationSpeed = 45.0f;
}

void AItemActor::BeginPlay()
{
    if (CollisionSphere)
    {
        CollisionSphere->SetMobility(EComponentMobility::Movable);
    }

    Super::BeginPlay();
    StartLocation = GetActorLocation();
}

void AItemActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FVector NewLocation = StartLocation;
    NewLocation.Z += FMath::Sin(GetWorld()->GetTimeSeconds() * FloatSpeed) * FloatHeight;
    SetActorLocation(NewLocation);

    AddActorLocalRotation(FRotator(0.f, RotationSpeed * DeltaTime, 0.f));
}

void AItemActor::InitializeItem(FName InItemRowName, UDataTable* InItemDataTable)
{
    ItemRowName = InItemRowName;
    ItemDataTable = InItemDataTable;

    UpdateItemVisual();
}

void AItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    // 데이터 복제를 위해 LifetimeReplicated 속성 등록
    DOREPLIFETIME(AItemActor, ItemRowName);
    DOREPLIFETIME(AItemActor, ItemDataTable);
}

void AItemActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
    Super::NotifyActorBeginOverlap(OtherActor);

    if (!HasAuthority()) return;
    if (!IsValid(ItemDataTable)) return;

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
    case EBomberItemType::BombCount:
    case EBomberItemType::MoveSpeed:
        if (const TSubclassOf<UGameplayEffect>* FoundEffectClass = ItemEffectMap.Find(Row->ItemType))
        {
            if (IsValid(*FoundEffectClass))
            {
                UItemDropComponent::ApplyItemEffect(OtherActor, *FoundEffectClass);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[ItemActor] ItemEffectMap에 %s 타입은 있지만 GameplayEffect가 비어있음"),
                    *StaticEnum<EBomberItemType>()->GetNameStringByValue(static_cast<int64>(Row->ItemType)));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[ItemActor] ItemEffectMap에 %s 타입이 등록되어 있지 않음"),
                *StaticEnum<EBomberItemType>()->GetNameStringByValue(static_cast<int64>(Row->ItemType)));
        }
        break;

    case EBomberItemType::MedKit:
        if (ASpartaArcadeCharacter* Character = Cast<ASpartaArcadeCharacter>(OtherActor))
        {
            Character->AddFirstAidKit();
        }
        break;

    case EBomberItemType::Shield:
        if (ASpartaArcadeCharacter* Character = Cast<ASpartaArcadeCharacter>(OtherActor))
        {
            Character->AddShield();
        }
        break;
        
    case EBomberItemType::KickUnlock:
        if (ASpartaArcadeCharacter* Character = Cast<ASpartaArcadeCharacter>(OtherActor))
        {
            Character->UnlockKickBomb();
        }   
        break;

    default:
        break;
    }

    // 디버그 메시지 주석 처리
    // if (GEngine)
    // {
    //     const FString ItemTypeName = StaticEnum<EBomberItemType>()->GetNameStringByValue(static_cast<int64>(Row->ItemType));
    //     GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
    //         FString::Printf(TEXT("%s 아이템 획득! (%s)"), *ItemTypeName, *OtherActor->GetName()));
    // }

    Destroy();
}

// 클라이언트에서 아이템 데이터 복제 수신 시 비주얼 동적 갱신
void AItemActor::OnRep_ItemRowName()
{
    UpdateItemVisual();
}

// 데이터 테이블과 Row명에 맞추어 아이템 메시, 머티리얼, 스케일 값을 찾고 적용하는 헬퍼 함수 구현
void AItemActor::UpdateItemVisual()
{
    if (!ItemMesh) return;

    //기본 스태틱 메시가 None 아이템 상태일 때 노출되지 않도록 nullptr로 초기화
    ItemMesh->SetStaticMesh(nullptr);

    if (IsValid(ItemDataTable) && !ItemRowName.IsNone())
    {
        FItemDataRow* Row = ItemDataTable->FindRow<FItemDataRow>(ItemRowName, TEXT("UpdateItemVisual"));
        if (Row && Row->ItemType != EBomberItemType::None)
        {
            EBomberItemType ItemType = Row->ItemType;
            if (ItemVisualMap.Contains(ItemType))
            {
                const FItemActorVisualInfo& Info = ItemVisualMap[ItemType];
                if (Info.Mesh)
                {
                    ItemMesh->SetStaticMesh(Info.Mesh);
                }
                if (Info.Material)
                {
                    ItemMesh->SetMaterial(0, Info.Material);
                }
                ItemMesh->SetRelativeScale3D(Info.Scale);
            }
        }
    }
}