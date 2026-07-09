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
#include "TimerManager.h"

ASpartaArcadeMapBuilder::ASpartaArcadeMapBuilder()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;             // MapGrid 복제용

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

    // 실내 기둥 — 그리드상 FixedWall이지만 시각적으로 벽과 구분(색·두께). 충돌은 벽과 동일.
    PillarISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("PillarISM"));
    PillarISM->SetupAttachment(SceneRoot);
    PillarISM->SetCollisionProfileName(TEXT("BlockAll"));

    // 변형 타일 — 바닥 타일처럼 시각용(충돌 없음). 효과는 게임플레이 레이어에서.
    IceISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("IceISM"));
    IceISM->SetupAttachment(SceneRoot);
    IceISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    MudWaterISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("MudWaterISM"));
    MudWaterISM->SetupAttachment(SceneRoot);
    MudWaterISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    BushISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BushISM"));
    BushISM->SetupAttachment(SceneRoot);
    BushISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 엔진 기본 메쉬 자동 로드 + 컴포넌트에 바로 할당(에디터에서 머티리얼 슬롯이 보이게).
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeF.Succeeded())
    {
        WallMesh = CubeF.Object; BoxMesh = CubeF.Object;
        WallISM->SetStaticMesh(WallMesh);
        BoxISM->SetStaticMesh(BoxMesh);
        PillarISM->SetStaticMesh(WallMesh);
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

    // 그리드 생성은 서버에서만. 클라는 OnRep_MapGrid에서 비주얼만.
    if (HasAuthority())
    {
        BuildMap();
        BuildVisuals();   // 서버(리슨/스탠드얼론) 로컬 비주얼

        // [테스트] 주기적 박스 파괴로 그리드 갱신·복제 확인(기본 꺼짐, 디테일에서 켜기).
        if (bDestroyBoxesForTest)
        {
            GetWorldTimerManager().SetTimer(TestDestroyTimerHandle, this,
                &ASpartaArcadeMapBuilder::TestDestroyRandomBox,
                FMath::Max(0.05f, TestDestroyInterval), true);
        }
    }
}

void ASpartaArcadeMapBuilder::BuildMap()
{
    if (!HasAuthority())
    {
        return;
    }

    GenerateGridData();   // 순수 데이터 생성(그리드·스폰 좌표·연결성 검증)
    SpawnTestActors();    // 테스트 액터 스폰(런타임 전용)
}

void ASpartaArcadeMapBuilder::GenerateGridData()
{
    // 런타임(매치)에서는 DT가 할당돼 있으면 수치를 DT에서 로드 — 밸런싱은 DT 한 곳에서.
    // 에디터 프리뷰는 자동 로드하지 않음(라이브 튜닝과 충돌 방지, bLoadFromTable로 수동 확인).
    if (bUseTableAtRuntime && MapGenTable && GetWorld() && GetWorld()->IsGameWorld())
    {
        ApplyMapGenRow();
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

void ASpartaArcadeMapBuilder::SpawnTestActors()
{
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;   // 에디터 월드에서는 액터를 스폰하지 않음(프리뷰 안전)
    }

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
            // BeginPlay 전에 맵빌더 + 색칠 머티리얼 전달(즉시 색 적용되게).
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
}

void ASpartaArcadeMapBuilder::OnRep_MapGrid()
{
    if (bVisualsBuilt)
    {
        // 런타임 변경(박스 파괴)은 증분 갱신 — 박스 하나 때문에 전체(수천 인스턴스) 재구축하지 않음.
        RefreshDestroyedBoxVisuals();
        return;
    }
    BuildVisuals();   // 최초 수신: 복제된 그리드로 로컬 비주얼 생성
}

void ASpartaArcadeMapBuilder::BuildVisuals()
{
    if (MapGrid.Tiles.Num() == 0)
    {
        return;
    }

    // 메쉬는 생성자에서 할당됨. 혹시 비어있을 때만 채움(안전망).
    if (WallMesh && !WallISM->GetStaticMesh())   WallISM->SetStaticMesh(WallMesh);
    if (WallMesh && !PillarISM->GetStaticMesh()) PillarISM->SetStaticMesh(WallMesh);
    if (BoxMesh && !BoxISM->GetStaticMesh())    BoxISM->SetStaticMesh(BoxMesh);
    if (FloorMesh && !FloorPlane->GetStaticMesh()) FloorPlane->SetStaticMesh(FloorMesh);
    if (FloorMesh && !FloorISM->GetStaticMesh())  FloorISM->SetStaticMesh(FloorMesh);
    if (FloorMesh && !IceISM->GetStaticMesh())      IceISM->SetStaticMesh(FloorMesh);
    if (FloorMesh && !MudWaterISM->GetStaticMesh()) MudWaterISM->SetStaticMesh(FloorMesh);
    if (FloorMesh && !BushISM->GetStaticMesh())     BushISM->SetStaticMesh(FloorMesh);

    WallISM->ClearInstances();
    PillarISM->ClearInstances();
    BoxISM->ClearInstances();
    FloorISM->ClearInstances();
    IceISM->ClearInstances();
    MudWaterISM->ClearInstances();
    BushISM->ClearInstances();
    BoxCellToInstance.Reset();   // 박스 셀→인스턴스 매핑도 처음부터

    const float S = TileSize;
    const float CubeUU = 100.f;            // 엔진 큐브/플레인 기본 크기(1m)
    const float WallScale = S / CubeUU;

    // 배경 바닥 플레인: 맵 전체를 덮게(= 빈 공간 색). 룸은 그 위에 밝은 타일로 따로.
    if (FloorMesh)
    {
        FloorPlane->SetRelativeScale3D(FVector(GridWidth * S / CubeUU, GridHeight * S / CubeUU, 1.f));
        FloorPlane->SetRelativeLocation(FVector((GridWidth - 1) * S * 0.5f, (GridHeight - 1) * S * 0.5f, 0.f));
    }

    // 타일별 트랜스폼을 먼저 전부 모은 뒤 '한 번에' 추가(배치).
    // 개별 AddInstance 1만 2천 번은 에디터에서 호출마다 내비/이벤트 갱신이 붙어 수 초 멈춤 → 배치로 컴포넌트당 1번.
    TArray<FTransform> FloorXfs, WallXfs, PillarXfs, BoxXfs, IceXfs, MudXfs, BushXfs;
    TArray<int32> BoxCells;   // BoxXfs와 같은 순서의 셀 인덱스(Y*W+X)
    const int32 TotalCells = MapGrid.Width * MapGrid.Height;
    FloorXfs.Reserve(TotalCells);
    WallXfs.Reserve(TotalCells / 4);
    PillarXfs.Reserve(TotalCells / 8);
    BoxXfs.Reserve(TotalCells / 4);
    BoxCells.Reserve(TotalCells / 4);

    // 기둥/벽 시각 분류(렌더 전용 — 그리드 의미는 동일한 FixedWall):
    // 벽은 선으로 이어져 벽 이웃이 보통 2개 이상, 기둥은 홀로 서거나 벽에 살짝 붙어 최대 1개.
    // 복제된 그리드만으로 계산하므로 서버·클라가 항상 같은 결과.
    auto CountFixedNeighbors = [this](int32 CX, int32 CY)
        {
            int32 N = 0;
            const int32 DX[4] = { 1, -1, 0, 0 };
            const int32 DY[4] = { 0, 0, 1, -1 };
            for (int32 i = 0; i < 4; ++i)
            {
                if (MapGrid.GetTile(CX + DX[i], CY + DY[i]) == ESpartaArcadeTileType::FixedWall)   // 맵 밖=FixedWall
                {
                    ++N;
                }
            }
            return N;
        };

    for (int32 Y = 0; Y < MapGrid.Height; ++Y)
    {
        for (int32 X = 0; X < MapGrid.Width; ++X)
        {
            const FVector Pos(X * S, Y * S, 0.f);
            switch (MapGrid.GetTile(X, Y))
            {
            case ESpartaArcadeTileType::Empty:
                // 룸 바닥 타일 — 배경보다 1uu 위로 올려 z-fighting 방지.
                FloorXfs.Emplace(FRotator::ZeroRotator, Pos + FVector(0, 0, 1.f), FVector(WallScale));
                break;
            case ESpartaArcadeTileType::FixedWall:
            {
                if (CountFixedNeighbors(X, Y) <= 1)
                {
                    // 실내 기둥 — 살짝 좁게(0.85) 세워 모서리 스침이 부드럽고 벽과 한눈에 구분.
                    PillarXfs.Emplace(FRotator::ZeroRotator, Pos + FVector(0, 0, S * 0.5f),
                        FVector(WallScale * 0.85f, WallScale * 0.85f, WallScale));
                }
                else
                {
                    WallXfs.Emplace(FRotator::ZeroRotator, Pos + FVector(0, 0, S * 0.5f), FVector(WallScale));
                }
                break;
            }
            case ESpartaArcadeTileType::DestructibleBox:
                // 박스는 살짝 작고 낮게 → 벽과 시각적으로 구분.
                BoxXfs.Emplace(FRotator::ZeroRotator, Pos + FVector(0, 0, S * 0.3f),
                    FVector(WallScale * 0.8f, WallScale * 0.8f, WallScale * 0.6f));
                BoxCells.Add(MapGrid.IndexOf(X, Y));
                break;
            case ESpartaArcadeTileType::Ice:
                IceXfs.Emplace(FRotator::ZeroRotator, Pos + FVector(0, 0, 2.f), FVector(WallScale));
                break;
            case ESpartaArcadeTileType::MudWater:
                MudXfs.Emplace(FRotator::ZeroRotator, Pos + FVector(0, 0, 2.f), FVector(WallScale));
                break;
            case ESpartaArcadeTileType::Bush:
                BushXfs.Emplace(FRotator::ZeroRotator, Pos + FVector(0, 0, 2.f), FVector(WallScale));
                break;
            default:
                break;   // Void(빈 공간)는 인스턴스 없음 → 어두운 배경 플레인이 보임
            }
        }
    }

    FloorISM->AddInstances(FloorXfs, /*bShouldReturnIndices=*/false);
    WallISM->AddInstances(WallXfs, /*bShouldReturnIndices=*/false);
    PillarISM->AddInstances(PillarXfs, /*bShouldReturnIndices=*/false);
    IceISM->AddInstances(IceXfs, /*bShouldReturnIndices=*/false);
    MudWaterISM->AddInstances(MudXfs, /*bShouldReturnIndices=*/false);
    BushISM->AddInstances(BushXfs, /*bShouldReturnIndices=*/false);

    // 박스는 셀→인스턴스 매핑이 필요해 인덱스를 받아서 기록(파괴 시 증분 숨김용).
    const TArray<int32> BoxIndices = BoxISM->AddInstances(BoxXfs, /*bShouldReturnIndices=*/true);
    for (int32 i = 0; i < BoxCells.Num(); ++i)
    {
        BoxCellToInstance.Add(BoxCells[i], BoxIndices.IsValidIndex(i) ? BoxIndices[i] : i);
    }

    // 색칠: 컴포넌트별 다이내믹 머티리얼 인스턴스로 색 지정.
    // (ISM은 BlockoutMaterial에 "Used with Instanced Static Meshes" 플래그가 켜져 있어야 색이 보임.)
    if (BlockoutMaterial)
    {
        auto ColorComp = [this](UMeshComponent* Comp, const FLinearColor& C)
            {
                if (!Comp) return;
                if (UMaterialInstanceDynamic* M = Comp->CreateDynamicMaterialInstance(0, BlockoutMaterial))
                {
                    M->SetVectorParameterValue(TEXT("Color"), C);      // 파라미터 이름이 "Color"
                    M->SetVectorParameterValue(TEXT("BaseColor"), C);  // 또는 "BaseColor"
                }
            };
        ColorComp(FloorPlane, VoidColor);   // 배경 = 빈 공간
        ColorComp(FloorISM, FloorColor);   // 룸 바닥
        ColorComp(WallISM, WallColor);    // 부술 수 없는 벽
        ColorComp(PillarISM, PillarColor);  // 실내 기둥(청회색)
        ColorComp(BoxISM, BoxColor);     // 부술 수 있는 벽
        ColorComp(IceISM, IceColor);      // 얼음
        ColorComp(MudWaterISM, MudWaterColor); // 물·진흙
        ColorComp(BushISM, BushColor);     // 덤불

        FloorISM->MarkRenderStateDirty();
        WallISM->MarkRenderStateDirty();
        PillarISM->MarkRenderStateDirty();
        BoxISM->MarkRenderStateDirty();
        IceISM->MarkRenderStateDirty();
        MudWaterISM->MarkRenderStateDirty();
        BushISM->MarkRenderStateDirty();
    }

    bVisualsBuilt = true;   // 이후 OnRep은 증분 갱신 경로로
}

FVector ASpartaArcadeMapBuilder::TileToWorld(int32 X, int32 Y) const
{
    return GetActorLocation() + FVector(X * TileSize, Y * TileSize, 0.f);
}

bool ASpartaArcadeMapBuilder::IsCellWalkable(int32 X, int32 Y) const
{
    if (X < 0 || Y < 0 || X >= MapGrid.Width || Y >= MapGrid.Height) return false;
    const ESpartaArcadeTileType T = MapGrid.GetTile(X, Y);
    // 통과 가능 = 빈 바닥 + 변형 타일(효과만 있고 막지 않음). 벽·박스·기둥·void·자기장은 불가.
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

// ---- 런타임 그리드 갱신 (E 연동: 박스 파괴 → 칸 뚫림) ----

bool ASpartaArcadeMapBuilder::NotifyTileDestroyed(int32 X, int32 Y)
{
    if (!HasAuthority()) return false;   // 그리드 변경은 서버 권위(치트 방지)
    if (!MapGrid.IsInside(X, Y))
    {
        UE_LOG(LogTemp, Warning, TEXT("[MapBuilder] NotifyTileDestroyed: (%d,%d)는 격자 밖"), X, Y);
        return false;
    }
    if (MapGrid.GetTile(X, Y) != ESpartaArcadeTileType::DestructibleBox)
    {
        return false;   // 이미 뚫렸거나 박스가 아닌 칸 — 중복 호출에도 안전(멱등)
    }

    MapGrid.SetTile(X, Y, ESpartaArcadeTileType::Empty);   // 복제됨 → 클라 OnRep에서 증분 반영
    RefreshDestroyedBoxVisuals();                          // 서버(리슨) 로컬 비주얼 즉시 갱신
    return true;
}

bool ASpartaArcadeMapBuilder::NotifyTileDestroyedAtWorld(const FVector& WorldPos)
{
    int32 X = 0, Y = 0;
    if (!WorldToTile(WorldPos, X, Y)) return false;
    return NotifyTileDestroyed(X, Y);
}

void ASpartaArcadeMapBuilder::RefreshDestroyedBoxVisuals()
{
    if (!BoxISM || MapGrid.Tiles.Num() == 0) return;

    // 그리드와 대조: 더 이상 박스가 아닌 칸 수집(맵 순회 중 수정 방지 위해 먼저 모음).
    TArray<int32> RemovedCells;
    for (const TPair<int32, int32>& P : BoxCellToInstance)
    {
        if (!MapGrid.Tiles.IsValidIndex(P.Key)
            || MapGrid.Tiles[P.Key] != ESpartaArcadeTileType::DestructibleBox)
        {
            RemovedCells.Add(P.Key);
        }
    }

    for (const int32 CellIdx : RemovedCells)
    {
        const int32 InstIdx = BoxCellToInstance[CellIdx];

        // 인스턴스 '제거' 대신 '숨김'(멀리 아래로 이동 + 축소).
        // RemoveInstance는 다른 인스턴스 인덱스를 밀어 매핑이 깨지므로, 이동 방식이 안전하고 O(1).
        // BlockAll 충돌도 플레이 영역 밖으로 함께 치워짐.
        FTransform Xf;
        if (BoxISM->GetInstanceTransform(InstIdx, Xf, /*bWorldSpace=*/false))
        {
            Xf.AddToTranslation(FVector(0, 0, -100000.f));
            Xf.SetScale3D(Xf.GetScale3D() * 0.01f);
            BoxISM->UpdateInstanceTransform(InstIdx, Xf, /*bWorldSpace=*/false,
                /*bMarkRenderStateDirty=*/true, /*bTeleport=*/true);
        }
        BoxCellToInstance.Remove(CellIdx);

        // 칸 변경 알림(로컬) — UI 미니맵 등 구독자용.
        OnTileChanged.Broadcast(CellIdx % MapGrid.Width, CellIdx / MapGrid.Width);
    }
}

void ASpartaArcadeMapBuilder::TestDestroyRandomBox()
{
    if (!HasAuthority()) return;

    // 임의 박스 칸 찾기: 무작위 64회 시도 → 실패 시 전수 탐색(박스가 거의 없을 때).
    int32 FoundX = -1, FoundY = -1;
    for (int32 Try = 0; Try < 64 && FoundX < 0; ++Try)
    {
        const int32 X = FMath::RandRange(0, MapGrid.Width - 1);
        const int32 Y = FMath::RandRange(0, MapGrid.Height - 1);
        if (MapGrid.GetTile(X, Y) == ESpartaArcadeTileType::DestructibleBox)
        {
            FoundX = X; FoundY = Y;
        }
    }
    if (FoundX < 0)
    {
        for (int32 i = 0; i < MapGrid.Tiles.Num(); ++i)
        {
            if (MapGrid.Tiles[i] == ESpartaArcadeTileType::DestructibleBox)
            {
                FoundX = i % MapGrid.Width;
                FoundY = i / MapGrid.Width;
                break;
            }
        }
    }
    if (FoundX < 0)
    {
        UE_LOG(LogTemp, Display, TEXT("[MapBuilder] 테스트 파괴: 남은 박스 없음 — 타이머 종료"));
        GetWorldTimerManager().ClearTimer(TestDestroyTimerHandle);
        return;
    }

    NotifyTileDestroyed(FoundX, FoundY);
    UE_LOG(LogTemp, Verbose, TEXT("[MapBuilder] 테스트 파괴: (%d,%d) 박스 → Empty"), FoundX, FoundY);
}

bool ASpartaArcadeMapBuilder::ApplyMapGenRow()
{
    if (!MapGenTable) return false;

    const FSpartaArcadeMapGenRow* Row =
        MapGenTable->FindRow<FSpartaArcadeMapGenRow>(MapGenRowName, TEXT("ApplyMapGenRow"));
    if (!Row)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MapBuilder] MapGenTable에 Row '%s'가 없어요 — BP 수치 그대로 사용"),
            *MapGenRowName.ToString());
        return false;
    }

    GridWidth = Row->GridWidth;
    GridHeight = Row->GridHeight;
    SectorCols = Row->SectorCols;
    SectorRows = Row->SectorRows;
    Gap = Row->Gap;
    VoidSlots = Row->VoidSlots;
    MergeChance = Row->MergeChance;
    CenterSlots = Row->CenterSlots;
    ExtraOpenChance = Row->ExtraOpenChance;
    BreakableWallChance = Row->BreakableWallChance;
    InteriorBlock = Row->InteriorBlock;
    BoxDensity = Row->BoxDensity;
    DoorClearRadius = Row->DoorClearRadius;
    EmptyStyleWeight = Row->EmptyStyleWeight;
    RegularStyleWeight = Row->RegularStyleWeight;
    MessyPillarChance = Row->MessyPillarChance;
    EmptyBoxChance = Row->EmptyBoxChance;
    SafeRadius = Row->SafeRadius;
    VariantCoverage = Row->VariantCoverage;
    bVariantsInCenter = Row->bVariantsInCenter;
    NumObstacles = Row->NumObstacles;

    UE_LOG(LogTemp, Display, TEXT("[MapBuilder] DT 수치 적용: %s / Row '%s'"),
        *MapGenTable->GetName(), *MapGenRowName.ToString());
    return true;
}

// ---- 에디터 프리뷰(시드 QA·수치 튜닝용, PIE 불필요) ----

void ASpartaArcadeMapBuilder::EditorRegenerate()
{
    if (HasAnyFlags(RF_ClassDefaultObject)) return;   // 클래스 기본값(CDO)에서는 동작 금지
    const double T0 = FPlatformTime::Seconds();
    GenerateGridData();
    BuildVisuals();
    UE_LOG(LogTemp, Display, TEXT("[MapBuilder] 에디터 재생성 완료 (%.2f초)"), FPlatformTime::Seconds() - T0);
}

void ASpartaArcadeMapBuilder::EditorRandomSeed()
{
    if (HasAnyFlags(RF_ClassDefaultObject)) return;

    Modify();                                    // 에디터 undo/저장 더티 표시
    Seed = FMath::RandRange(1, 999999);          // 0은 '매판 랜덤' 모드라 피함
    UE_LOG(LogTemp, Display, TEXT("[MapBuilder] 에디터: 새 시드 %d"), Seed);
    GenerateGridData();
    BuildVisuals();
}

void ASpartaArcadeMapBuilder::EditorClearPreview()
{
    if (HasAnyFlags(RF_ClassDefaultObject)) return;

    auto ClearISM = [](UHierarchicalInstancedStaticMeshComponent* C) { if (C) C->ClearInstances(); };
    ClearISM(WallISM);
    ClearISM(PillarISM);
    ClearISM(BoxISM);
    ClearISM(FloorISM);
    ClearISM(IceISM);
    ClearISM(MudWaterISM);
    ClearISM(BushISM);

    if (FloorPlane)
    {
        FloorPlane->SetRelativeScale3D(FVector(1.f));
        FloorPlane->SetRelativeLocation(FVector::ZeroVector);
    }

    BoxCellToInstance.Reset();
    bVisualsBuilt = false;
    MapGrid = FSpartaArcadeMapGrid();
    SpawnWorldLocations.Reset();
    ObstacleSpawnWorldLocations.Reset();
}

#if WITH_EDITOR
void ASpartaArcadeMapBuilder::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName Name = PropertyChangedEvent.GetPropertyName();

    // ---- 체크박스 트리거: 체크 → 실행 → 자동으로 꺼짐 ----
    if (Name == GET_MEMBER_NAME_CHECKED(ASpartaArcadeMapBuilder, bRegenerateMap) && bRegenerateMap)
    {
        bRegenerateMap = false;
        EditorRegenerate();
        return;
    }
    if (Name == GET_MEMBER_NAME_CHECKED(ASpartaArcadeMapBuilder, bNewRandomSeed) && bNewRandomSeed)
    {
        bNewRandomSeed = false;
        EditorRandomSeed();
        return;
    }
    if (Name == GET_MEMBER_NAME_CHECKED(ASpartaArcadeMapBuilder, bClearPreview) && bClearPreview)
    {
        bClearPreview = false;
        EditorClearPreview();
        return;
    }
    if (Name == GET_MEMBER_NAME_CHECKED(ASpartaArcadeMapBuilder, bLoadFromTable) && bLoadFromTable)
    {
        bLoadFromTable = false;
        if (ApplyMapGenRow())
        {
            EditorRegenerate();   // DT 수치를 프로퍼티로 당겨온 뒤 그 값으로 재생성
        }
        return;
    }

    // ---- 라이브 프리뷰: SpartaArcade 카테고리 수치가 바뀌면 즉시 재생성 ----
    if (bLivePreview && PropertyChangedEvent.Property)
    {
        // 슬라이더 드래그 중(Interactive)에는 재생성하지 않음(성능).
        if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive) return;

        const FString Cat = PropertyChangedEvent.Property->GetMetaData(TEXT("Category"));
        if (Cat.StartsWith(TEXT("SpartaArcade")) &&
            Name != GET_MEMBER_NAME_CHECKED(ASpartaArcadeMapBuilder, bLivePreview))
        {
            EditorRegenerate();
        }
    }
}
#endif