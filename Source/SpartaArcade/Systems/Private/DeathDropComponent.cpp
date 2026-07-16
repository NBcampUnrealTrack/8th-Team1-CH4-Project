#include "DeathDropComponent.h"
#include "ItemActor.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "BomberTypes.h"
#include "Math/UnrealMathUtility.h"
#include "Level/Public/SpartaArcadeMapBuilder.h"
#include "Kismet/GameplayStatics.h"

UDeathDropComponent::UDeathDropComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false); // 스폰 자체는 서버에서만, 액터 복제로 클라에 전달됨
}

void UDeathDropComponent::DropDeathItems(FVector DropLocation)
{
    // 서버 전용 — 클라에서 호출해도 무시
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    if (!IsValid(ItemActorClass) || !IsValid(ItemDataTable)) 
    {
        UE_LOG(LogTemp, Warning, TEXT("[DeathDropComponent] ItemActorClass 또는 ItemDataTable이 배정되지 않았습니다!"));
        return;
    }

    // 사망 시 드롭할 아이템
    static const TArray<FName> DropRows = {
        FName("BombCount"),
        FName("BombRange"),
        FName("MoveSpeed"),
        FName("MedKit"),
    };

    // MapBuilder를 찾아 그리드 기반 유효 타일 탐색 준비
    ASpartaArcadeMapBuilder* MapBuilder = Cast<ASpartaArcadeMapBuilder>(
        UGameplayStatics::GetActorOfClass(GetWorld(), ASpartaArcadeMapBuilder::StaticClass()));

    const int32 Count = DropRows.Num();
    for (int32 i = 0; i < Count; ++i)
    {
        // DataTable에 해당 Row가 있는지 검증
        FItemDataRow* Row = ItemDataTable->FindRow<FItemDataRow>(DropRows[i], TEXT("DeathDropComponent"));
        if (!Row)
        {
            UE_LOG(LogTemp, Warning, TEXT("[DeathDropComponent] DataTable에 Row '%s' 가 없습니다. DT_Item을 확인하세요."), *DropRows[i].ToString());
            continue;
        }

        //  BFS로 가장 가까운 빈 타일을 타입에 하나씩 할당
        FVector SpawnLocation;
        if (IsValid(MapBuilder))
        {
            // 이미 사용된 타일 위치를 추적하여 같은 칸에 중복 스폰 방지
            const bool bFound = FindNearestWalkableTile(MapBuilder, DropLocation, UsedTileSet, SpawnLocation);
            if (!bFound)
            {
                UE_LOG(LogTemp, Warning, TEXT("[DeathDropComponent] '%s' 아이템에 대한 빈 타일을 찾지 못했습니다."), *DropRows[i].ToString());
                continue;
            }
        }
        else
        {
            // 폴백: MapBuilder 없으면 원형 오프셋 사용
            const float AngleDeg = (360.f / Count) * i;
            const float AngleRad = FMath::DegreesToRadians(AngleDeg);
            SpawnLocation = DropLocation + FVector(
                FMath::Cos(AngleRad) * SpreadRadius,
                FMath::Sin(AngleRad) * SpreadRadius,
                0.f);
        }

        SpawnItemAt(DropRows[i], SpawnLocation);
    }

    // 다음 사망 드롭을 대비해 사용된 세트 초기화
    UsedTileSet.Empty();
}

bool UDeathDropComponent::FindNearestWalkableTile(
    ASpartaArcadeMapBuilder* MapBuilder,
    const FVector& Origin,
    TSet<FIntPoint>& OutUsed,
    FVector& OutLocation) const
{
    // BFS — 원점에서 점점 넓혀지며 가장 가까운 walkable + 미사용 타일을 선택
    int32 StartX, StartY;
    if (!MapBuilder->WorldToTile(Origin, StartX, StartY))
    {
        return false;
    }

    // BFS 큐 (X, Y)
    TQueue<FIntPoint> Queue;
    TSet<FIntPoint> Visited;

    Queue.Enqueue({StartX, StartY});
    Visited.Add({StartX, StartY});

    // 상/하/좌/우 + 대각선 4방향
    static const FIntPoint Dirs[] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{-1,1},{1,-1},{-1,-1}};
    constexpr int32 MaxSearchRange = 8; // 최대 8타일 반경내에서만 탐색

    while (!Queue.IsEmpty())
    {
        FIntPoint Current;
        Queue.Dequeue(Current);

        const int32 Dist = FMath::Abs(Current.X - StartX) + FMath::Abs(Current.Y - StartY);
        if (Dist > MaxSearchRange) continue;

        if (MapBuilder->IsCellWalkable(Current.X, Current.Y) && !OutUsed.Contains(Current))
        {
            // 유효한 빈 타일 발견 → 요청자로 반환
            OutUsed.Add(Current);
            OutLocation = MapBuilder->TileToWorld(Current.X, Current.Y);
            // 아이템은 바닥 위에 륬 우리 (Z점 나중에 조정해도 됨)
            OutLocation.Z = Origin.Z;
            return true;
        }

        for (const FIntPoint& Dir : Dirs)
        {
            FIntPoint Next = {Current.X + Dir.X, Current.Y + Dir.Y};
            if (!Visited.Contains(Next))
            {
                Visited.Add(Next);
                Queue.Enqueue(Next);
            }
        }
    }

    return false;
}

void UDeathDropComponent::SpawnItemAt(FName RowName, FVector Location)
{
    FActorSpawnParameters SpawnParams;
    // 위치는 이미 walkable로 검증된 상태이지만, 안전망으로 AlwaysSpawn 유지
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AItemActor* SpawnedItem = GetWorld()->SpawnActor<AItemActor>(
        ItemActorClass,
        Location,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (IsValid(SpawnedItem))
    {
        SpawnedItem->InitializeItem(RowName, ItemDataTable);
    }
}
