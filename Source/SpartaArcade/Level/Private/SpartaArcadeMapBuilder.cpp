#include "SpartaArcadeMapBuilder.h"
#include "SpartaArcadeRoomGenerator.h"
#include "SpartaArcadeMovingObstacle.h"
#include "SpartaArcadeZoneManager.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ASpartaArcadeMapBuilder::ASpartaArcadeMapBuilder()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;            

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    FloorPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorPlane"));
    FloorPlane->SetupAttachment(SceneRoot);
    FloorPlane->SetCollisionProfileName(TEXT("BlockAll"));

    FloorISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("FloorISM"));
    FloorISM->SetupAttachment(SceneRoot);
    FloorISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);   // 룸 바닥 타일은 시각용

    WallISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WallISM"));
    WallISM->SetupAttachment(SceneRoot);
    WallISM->SetCollisionProfileName(TEXT("BlockAll"));

    BoxISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BoxISM"));
    BoxISM->SetupAttachment(SceneRoot);
    BoxISM->SetCollisionProfileName(TEXT("BlockAll"));

    IceISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("IceISM"));
    IceISM->SetupAttachment(SceneRoot);
    IceISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    MudWaterISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("MudWaterISM"));
    MudWaterISM->SetupAttachment(SceneRoot);
    MudWaterISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    BushISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BushISM"));
    BushISM->SetupAttachment(SceneRoot);
    BushISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeF.Succeeded())
    {
        WallMesh = CubeF.Object; BoxMesh = CubeF.Object;
        WallISM->SetStaticMesh(WallMesh);
        BoxISM->SetStaticMesh(BoxMesh);
    }
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneF(TEXT("/Engine/BasicShapes/Plane.Plane"));
    if (PlaneF.Succeeded())
    {
        FloorMesh = PlaneF.Object;
        FloorPlane->SetStaticMesh(FloorMesh);
        FloorISM->SetStaticMesh(FloorMesh);
        IceISM->SetStaticMesh(FloorMesh);
        MudWaterISM->SetStaticMesh(FloorMesh);
        BushISM->SetStaticMesh(FloorMesh);
    }
}

void ASpartaArcadeMapBuilder::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ASpartaArcadeMapBuilder, MapGrid);
    DOREPLIFETIME(ASpartaArcadeMapBuilder, CenterMin);
    DOREPLIFETIME(ASpartaArcadeMapBuilder, CenterMax);
}

void ASpartaArcadeMapBuilder::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        BuildMap();
        BuildVisuals();   
    }
}

void ASpartaArcadeMapBuilder::BuildMap()
{
    if (!HasAuthority())
    {
        return;
    }

    const int32 UsedSeed = (Seed != 0) ? Seed : FMath::Rand();
    if (Seed == 0)
    {
        UE_LOG(LogTemp, Display, TEXT("[MapBuilder] Random seed this match: %d"), UsedSeed);
    }

    FSpartaArcadeRoomGenParams P;
    P.Width = GridWidth;
    P.Height = GridHeight;
    P.SectorCols = SectorCols;
    P.SectorRows = SectorRows;
    P.Gap = Gap;
    P.VoidSlots = VoidSlots;
    P.MergeChance = MergeChance;
    P.CenterSlots = CenterSlots;
    P.ExtraOpenChance = ExtraOpenChance;
    P.BreakableWallChance = BreakableWallChance;
    P.InteriorBlock = InteriorBlock;
    P.BoxDensity = BoxDensity;
    P.DoorClearRadius = DoorClearRadius;
    P.EmptyStyleWeight = EmptyStyleWeight;
    P.RegularStyleWeight = RegularStyleWeight;
    P.MessyPillarChance = MessyPillarChance;
    P.EmptyBoxChance = EmptyBoxChance;
    P.SafeRadius = SafeRadius;
    P.VariantCoverage = VariantCoverage;
    P.bVariantsInCenter = bVariantsInCenter;
    P.NumObstacles = NumObstacles;

    TArray<FIntPoint> SpawnCells;
    TArray<FIntPoint> ObstacleCells;
    FSpartaArcadeRoomGenerator::Generate(MapGrid, UsedSeed, P, &SpawnCells, &ObstacleCells, &CenterMin, &CenterMax);

    // 스폰 칸 → 월드 좌표(GameMode가 플레이어 스폰에 사용).
    SpawnWorldLocations.Reset();
    for (const FIntPoint& C : SpawnCells)
        SpawnWorldLocations.Add(TileToWorld(C.X, C.Y));
    UE_LOG(LogTemp, Display, TEXT("[MapBuilder] Spawn points: %d"), SpawnWorldLocations.Num());

    // 이동 장애물 스폰 칸 → 월드 좌표.
    ObstacleSpawnWorldLocations.Reset();
    for (const FIntPoint& C : ObstacleCells)
        ObstacleSpawnWorldLocations.Add(TileToWorld(C.X, C.Y));
    UE_LOG(LogTemp, Display, TEXT("[MapBuilder] Obstacle spawns: %d"), ObstacleSpawnWorldLocations.Num());

    // 테스트용 장애물 스폰(서버). 이전 것 제거 후 재스폰. (실제 게임은 GameMode가 스폰해도 됨.)
    for (AActor* O : SpawnedObstacles) if (IsValid(O)) O->Destroy();
    SpawnedObstacles.Reset();
    if (bSpawnObstaclesForTest && ObstacleClass)
    {
        for (const FVector& L : ObstacleSpawnWorldLocations)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AActor* Obs = GetWorld()->SpawnActor<AActor>(ObstacleClass,
                FTransform(FRotator::ZeroRotator, L), SpawnParams);
            if (Obs)
            {
                if (ASpartaArcadeMovingObstacle* MO = Cast<ASpartaArcadeMovingObstacle>(Obs))
                    MO->SetMapBuilder(this);
                SpawnedObstacles.Add(Obs);
            }
        }
        UE_LOG(LogTemp, Display, TEXT("[MapBuilder] Spawned %d test obstacles"), SpawnedObstacles.Num());
    }

    // 테스트용 자기장 매니저 스폰(서버). 이전 것 제거 후 재스폰. (실제 게임은 GameMode가 스폰해도 됨.)
    if (IsValid(SpawnedZone)) SpawnedZone->Destroy();
    SpawnedZone = nullptr;
    if (bSpawnZoneForTest && ZoneManagerClass)
    {
        const FTransform ZoneXf(FRotator::ZeroRotator, GetActorLocation());
        AActor* Zone = GetWorld()->SpawnActorDeferred<AActor>(ZoneManagerClass, ZoneXf);
        if (Zone)
        {
            if (ASpartaArcadeZoneManager* ZM = Cast<ASpartaArcadeZoneManager>(Zone))
            {
                ZM->SetMapBuilder(this);
                if (!ZM->BlockoutMaterial) ZM->BlockoutMaterial = BlockoutMaterial;
            }
            Zone->FinishSpawning(ZoneXf);
            SpawnedZone = Zone;
            UE_LOG(LogTemp, Display, TEXT("[MapBuilder] Spawned zone manager"));
        }
    }

    // 연결성 검증: 벽/빈공간이 아닌 칸(바닥+박스)이 한 덩어리로 이어졌는지.
    int32 TotalOpen = 0;
    FIntPoint Start(-1, -1);
    for (int32 i = 0; i < MapGrid.Tiles.Num(); ++i)
    {
        const ESpartaArcadeTileType T = MapGrid.Tiles[i];
        if (T != ESpartaArcadeTileType::FixedWall && T != ESpartaArcadeTileType::Void)
        {
            ++TotalOpen;
            if (Start.X < 0) { Start.X = i % MapGrid.Width; Start.Y = i / MapGrid.Width; }
        }
    }
    const int32 Reach = (Start.X >= 0)
        ? FSpartaArcadeRoomGenerator::CountReachableNonWall(MapGrid, Start.X, Start.Y)
        : 0;

    UE_LOG(LogTemp, Display,
        TEXT("[MapBuilder] Built %dx%d grid (seed %d). Connectivity: %d/%d reachable %s"),
        MapGrid.Width, MapGrid.Height, UsedSeed, Reach, TotalOpen,
        (Reach == TotalOpen) ? TEXT("(OK)") : TEXT("(DISCONNECTED!)"));
}

void ASpartaArcadeMapBuilder::OnRep_MapGrid()
{
    BuildVisuals();  
}

void ASpartaArcadeMapBuilder::BuildVisuals()
{
    if (MapGrid.Tiles.Num() == 0)
    {
        return;
    }

    // 메쉬는 생성자에서 할당됨. 혹시 비어있을 때만 채움(안전망).
    if (WallMesh && !WallISM->GetStaticMesh())   WallISM->SetStaticMesh(WallMesh);
    if (BoxMesh && !BoxISM->GetStaticMesh())    BoxISM->SetStaticMesh(BoxMesh);
    if (FloorMesh && !FloorPlane->GetStaticMesh()) FloorPlane->SetStaticMesh(FloorMesh);
    if (FloorMesh && !FloorISM->GetStaticMesh())  FloorISM->SetStaticMesh(FloorMesh);
    if (FloorMesh && !IceISM->GetStaticMesh())      IceISM->SetStaticMesh(FloorMesh);
    if (FloorMesh && !MudWaterISM->GetStaticMesh()) MudWaterISM->SetStaticMesh(FloorMesh);
    if (FloorMesh && !BushISM->GetStaticMesh())     BushISM->SetStaticMesh(FloorMesh);

    WallISM->ClearInstances();
    BoxISM->ClearInstances();
    FloorISM->ClearInstances();
    IceISM->ClearInstances();
    MudWaterISM->ClearInstances();
    BushISM->ClearInstances();

    const float S = TileSize;
    const float CubeUU = 100.f;           
    const float WallScale = S / CubeUU;

    if (FloorMesh)
    {
        FloorPlane->SetRelativeScale3D(FVector(GridWidth * S / CubeUU, GridHeight * S / CubeUU, 1.f));
        FloorPlane->SetRelativeLocation(FVector((GridWidth - 1) * S * 0.5f, (GridHeight - 1) * S * 0.5f, 0.f));
    }

    for (int32 Y = 0; Y < MapGrid.Height; ++Y)
    {
        for (int32 X = 0; X < MapGrid.Width; ++X)
        {
            const FVector Pos(X * S, Y * S, 0.f);
            switch (MapGrid.GetTile(X, Y))
            {
            case ESpartaArcadeTileType::Empty:
            {
                const FTransform Xf(FRotator::ZeroRotator, Pos + FVector(0, 0, 1.f), FVector(WallScale));
                FloorISM->AddInstance(Xf);
                break;
            }
            case ESpartaArcadeTileType::FixedWall:
            {
                const FTransform Xf(FRotator::ZeroRotator, Pos + FVector(0, 0, S * 0.5f), FVector(WallScale));
                WallISM->AddInstance(Xf);
                break;
            }
            case ESpartaArcadeTileType::DestructibleBox:
            {
                const FTransform Xf(FRotator::ZeroRotator, Pos + FVector(0, 0, S * 0.3f),
                    FVector(WallScale * 0.8f, WallScale * 0.8f, WallScale * 0.6f));
                BoxISM->AddInstance(Xf);
                break;
            }
            case ESpartaArcadeTileType::Ice:
            {
                const FTransform Xf(FRotator::ZeroRotator, Pos + FVector(0, 0, 2.f), FVector(WallScale));
                IceISM->AddInstance(Xf);
                break;
            }
            case ESpartaArcadeTileType::MudWater:
            {
                const FTransform Xf(FRotator::ZeroRotator, Pos + FVector(0, 0, 2.f), FVector(WallScale));
                MudWaterISM->AddInstance(Xf);
                break;
            }
            case ESpartaArcadeTileType::Bush:
            {
                const FTransform Xf(FRotator::ZeroRotator, Pos + FVector(0, 0, 2.f), FVector(WallScale));
                BushISM->AddInstance(Xf);
                break;
            }
            default:
                break;   
            }
        }
    }

    if (BlockoutMaterial)
    {
        auto ColorComp = [this](UMeshComponent* Comp, const FLinearColor& C)
            {
                if (!Comp) return;
                if (UMaterialInstanceDynamic* M = Comp->CreateDynamicMaterialInstance(0, BlockoutMaterial))
                {
                    M->SetVectorParameterValue(TEXT("Color"), C);     
                    M->SetVectorParameterValue(TEXT("BaseColor"), C);  
                }
            };
        ColorComp(FloorPlane, VoidColor);   // 배경 = 빈 공간
        ColorComp(FloorISM, FloorColor);   // 룸 바닥
        ColorComp(WallISM, WallColor);    // 부술 수 없는 벽
        ColorComp(BoxISM, BoxColor);     // 부술 수 있는 벽
        ColorComp(IceISM, IceColor);      // 얼음
        ColorComp(MudWaterISM, MudWaterColor); // 물·진흙
        ColorComp(BushISM, BushColor);     // 덤불

        FloorISM->MarkRenderStateDirty();
        WallISM->MarkRenderStateDirty();
        BoxISM->MarkRenderStateDirty();
        IceISM->MarkRenderStateDirty();
        MudWaterISM->MarkRenderStateDirty();
        BushISM->MarkRenderStateDirty();
    }
}

FVector ASpartaArcadeMapBuilder::TileToWorld(int32 X, int32 Y) const
{
    return GetActorLocation() + FVector(X * TileSize, Y * TileSize, 0.f);
}

bool ASpartaArcadeMapBuilder::IsCellWalkable(int32 X, int32 Y) const
{
    if (X < 0 || Y < 0 || X >= MapGrid.Width || Y >= MapGrid.Height) return false;
    const ESpartaArcadeTileType T = MapGrid.GetTile(X, Y);
    return T == ESpartaArcadeTileType::Empty || T == ESpartaArcadeTileType::Ice
        || T == ESpartaArcadeTileType::MudWater || T == ESpartaArcadeTileType::Bush;
}

bool ASpartaArcadeMapBuilder::WorldToTile(const FVector& World, int32& OutX, int32& OutY) const
{
    const FVector Local = World - GetActorLocation();
    OutX = FMath::RoundToInt(Local.X / TileSize);
    OutY = FMath::RoundToInt(Local.Y / TileSize);
    return (OutX >= 0 && OutY >= 0 && OutX < MapGrid.Width && OutY < MapGrid.Height);
}