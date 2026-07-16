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
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "InGame/SpartaGameMode.h"

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

    // 엔진 기본 메쉬 로드 및 TileVisualMap 기본 설정
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneF(TEXT("/Engine/BasicShapes/Plane.Plane"));

    UStaticMesh* DefaultCube = CubeF.Succeeded() ? CubeF.Object : nullptr;
    UStaticMesh* DefaultPlane = PlaneF.Succeeded() ? PlaneF.Object : nullptr;

    if (DefaultPlane)
    {
        FloorPlane->SetStaticMesh(DefaultPlane);
        FloorISM->SetStaticMesh(DefaultPlane);
        IceISM->SetStaticMesh(DefaultPlane);
        MudWaterISM->SetStaticMesh(DefaultPlane);
        BushISM->SetStaticMesh(DefaultPlane);
    }

    if (DefaultCube)
    {
        WallISM->SetStaticMesh(DefaultCube);
        BoxISM->SetStaticMesh(DefaultCube);
        PillarISM->SetStaticMesh(DefaultCube);
    }

    // 기본 타일 비주얼 맵 구성 (디폴트 설정)
    if (DefaultCube)
    {
        FSpartaArcadeTileVisualInfo WallInfo;
        WallInfo.Mesh = DefaultCube;
        WallInfo.BlockoutColor = FLinearColor(0.46f, 0.47f, 0.50f);
        TileVisualMap.Add(ESpartaArcadeTileType::FixedWall, WallInfo);

        FSpartaArcadeTileVisualInfo BoxInfo;
        BoxInfo.Mesh = DefaultCube;
        BoxInfo.BlockoutColor = FLinearColor(0.85f, 0.42f, 0.12f);
        BoxInfo.ScaleMultiplier = FVector(0.8f, 0.8f, 0.6f);
        BoxInfo.Offset = FVector(0.f, 0.f, -20.f); // 박스의 기존 Z 오프셋은 S*0.3f이었으나 100uu 규격으로 환산
        TileVisualMap.Add(ESpartaArcadeTileType::DestructibleBox, BoxInfo);
    }

    if (DefaultPlane)
    {
        FSpartaArcadeTileVisualInfo FloorInfo;
        FloorInfo.Mesh = DefaultPlane;
        FloorInfo.BlockoutColor = FLinearColor(0.18f, 0.34f, 0.24f);
        TileVisualMap.Add(ESpartaArcadeTileType::Empty, FloorInfo);
        TileVisualMap.Add(ESpartaArcadeTileType::Floor, FloorInfo);

        FSpartaArcadeTileVisualInfo IceInfo;
        IceInfo.Mesh = DefaultPlane;
        IceInfo.BlockoutColor = FLinearColor(0.72f, 0.85f, 0.95f);
        TileVisualMap.Add(ESpartaArcadeTileType::Ice, IceInfo);

        FSpartaArcadeTileVisualInfo MudInfo;
        MudInfo.Mesh = DefaultPlane;
        MudInfo.BlockoutColor = FLinearColor(0.28f, 0.44f, 0.54f);
        TileVisualMap.Add(ESpartaArcadeTileType::MudWater, MudInfo);

        FSpartaArcadeTileVisualInfo BushInfo;
        BushInfo.Mesh = DefaultPlane;
        BushInfo.BlockoutColor = FLinearColor(0.24f, 0.55f, 0.28f);
        TileVisualMap.Add(ESpartaArcadeTileType::Bush, BushInfo);

        FSpartaArcadeTileVisualInfo VoidInfo;
        VoidInfo.Mesh = DefaultPlane;
        VoidInfo.BlockoutColor = FLinearColor(0.04f, 0.05f, 0.07f);
        TileVisualMap.Add(ESpartaArcadeTileType::Void, VoidInfo);
        TileVisualMap.Add(ESpartaArcadeTileType::FloorPlane, VoidInfo);

        // 빠져 있던 ZoneBorder(자기장 경계선) 타일 비주얼 기본값 추가
        FSpartaArcadeTileVisualInfo ZoneBorderInfo;
        ZoneBorderInfo.Mesh = DefaultPlane;
        ZoneBorderInfo.BlockoutColor = FLinearColor(0.80f, 0.05f, 0.05f); // 자기장 경계 경고 빨간색
        TileVisualMap.Add(ESpartaArcadeTileType::ZoneBorder, ZoneBorderInfo);

        // 기본 장애물 클래스로 블루프린트 버전(BP_MovingObstacle)을 FClassFinder로 자동 탐색하여 할당
        // 디테일 패널 세팅이 누락되어 C++ 버전이 스폰되며 메시가 보이지 않는 현상을 방어
        static ConstructorHelpers::FClassFinder<AActor> ObstacleClassBP(TEXT("/Game/Level/BP_MovingObstacle.BP_MovingObstacle_C"));
        if (ObstacleClassBP.Succeeded())
        {
            ObstacleClass = ObstacleClassBP.Class;
        }
    }
}

void ASpartaArcadeMapBuilder::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ASpartaArcadeMapBuilder, MapGrid);
    DOREPLIFETIME(ASpartaArcadeMapBuilder, CenterMin);
    DOREPLIFETIME(ASpartaArcadeMapBuilder, CenterMax);
    DOREPLIFETIME(ASpartaArcadeMapBuilder, SpawnWorldLocations);
    DOREPLIFETIME(ASpartaArcadeMapBuilder, GridWidth);
    DOREPLIFETIME(ASpartaArcadeMapBuilder, GridHeight);
    DOREPLIFETIME(ASpartaArcadeMapBuilder, TileSize);
    DOREPLIFETIME(ASpartaArcadeMapBuilder, UsedSeed);
}

void ASpartaArcadeMapBuilder::BeginPlay()
{
    Super::BeginPlay();

    if (ObstacleClass == nullptr || ObstacleClass == ASpartaArcadeMovingObstacle::StaticClass())
    {
        UClass* LoadedBPClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Level/BP_MovingObstacle.BP_MovingObstacle_C"));
        if (LoadedBPClass)
        {
            ObstacleClass = LoadedBPClass;
        }
    }

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
    // 중복 생성 방지 및 선제 빌드 상태 기록
    if (bMapBuilt) return;
    bMapBuilt = true;

    if (!HasAuthority())
    {
        return;
    }

    GenerateGridData();   // 순수 데이터 생성(그리드·스폰 좌표·연결성 검증)
    // 파괴 가능한 상자 액터 스폰
    SpawnBreakableBoxes();
    SpawnTestActors();    // 테스트 액터 스폰(런타임 전용)

    // 스폰 포인트 겹침 구조물 제거 함수 호출
    ClearStructuresAtSpawns();

    // 맵 데이터 및 스폰 좌표 빌드가 완료 -> 대기실에 스폰해 있던 플레이어들을 고유 목적지로 즉시 텔레포트
    if (GetWorld())
    {
        ASpartaGameMode* GM = Cast<ASpartaGameMode>(GetWorld()->GetAuthGameMode());
        if (GM)
        {
            GM->TeleportPlayersToSpawns(SpawnWorldLocations);
        }
    }
}

void ASpartaArcadeMapBuilder::GenerateGridData()
{
    // 런타임(매치)에서는 DT가 할당돼 있으면 수치를 DT에서 로드 — 밸런싱은 DT 한 곳에서.
    // 에디터 프리뷰는 자동 로드하지 않음(라이브 튜닝과 충돌 방지, bLoadFromTable로 수동 확인).
    if (bUseTableAtRuntime && MapGenTable && GetWorld() && GetWorld()->IsGameWorld())
    {
        ApplyMapGenRow();
    }

    // 서버 권위 하에 최종 랜덤 시드를 설정하고 클라이언트로 동기화합니다.
    if (HasAuthority())
    {
        UsedSeed = (Seed != 0) ? Seed : FMath::Rand();
    }

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
    // 3x3 외곽 고정벽/Void 훼손을 최소화하면서 캡슐 충돌(Adjust Position)로 인해 
    // 캐릭터가 벽 위로 얹어지는 물리 현상을 방어하기 위해, 분면(Quadrant) 판정 기반으로 맵 안쪽 방향으로 25.f uu 오프셋 부여
    SpawnWorldLocations.Reset();
    const int32 HX = MapGrid.Width / 2;
    const int32 HY = MapGrid.Height / 2;

    for (const FIntPoint& C : SpawnCells)
    {
        // 구석으로부터 3x3 안전지대 공간의 정중앙 한가운데 타일 좌표로 스폰 위치 강제 교정하여 텔레포트 안착점 수립
        int32 CenterX = 1;
        int32 CenterY = 1;

        if (C.X < HX && C.Y < HY)
        {
            CenterX = 1;
            CenterY = 1;
        }
        else if (C.X >= HX && C.Y < HY)
        {
            CenterX = MapGrid.Width - 2;
            CenterY = 1;
        }
        else if (C.X < HX && C.Y >= HY)
        {
            CenterX = 1;
            CenterY = MapGrid.Height - 2;
        }
        else // C.X >= HX && C.Y >= HY
        {
            CenterX = MapGrid.Width - 2;
            CenterY = MapGrid.Height - 2;
        }

        FVector WorldLoc = TileToWorld(CenterX, CenterY);
        // Z축 높이를 지상에 가깝게 낮춤
        WorldLoc.Z = GetActorLocation().Z + 10.f;

        SpawnWorldLocations.Add(WorldLoc);
    }
    UE_LOG(LogTemp, Display, TEXT("[MapBuilder] Spawn points: %d"), SpawnWorldLocations.Num());

    // 이동 장애물 스폰 칸 → 월드 좌표.
    ObstacleSpawnWorldLocations.Reset();
    for (const FIntPoint& C : ObstacleCells)
    {
        // 시작하는 방에는 움직이는 장애물이 스폰되지 않도록 플레이어 스폰 위치로부터 반경 3칸 이내 영역을 정밀하게 필터링하여 스킵 처리합니다.
        bool bTooCloseToSpawn = false;
        for (const FIntPoint& SC : SpawnCells)
        {
            if (FMath::Abs(SC.X - C.X) <= 3 && FMath::Abs(SC.Y - C.Y) <= 3)
            {
                bTooCloseToSpawn = true;
                break;
            }
        }
        if (bTooCloseToSpawn)
        {
            UE_LOG(LogTemp, Warning, TEXT("[MapBuilder] 시작방 스폰지점과 근접한 (%d, %d) 구역의 장애물 스폰을 차단 스킵했습니다."), C.X, C.Y);
            continue;
        }

        ObstacleSpawnWorldLocations.Add(TileToWorld(C.X, C.Y));
    }
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

// 런타임에 진짜 파괴 가능한 상자 액터를 맵 데이터 위치에 맞춰 스폰하는 함수 구현
void ASpartaArcadeMapBuilder::SpawnBreakableBoxes()
{
    if (!HasAuthority() || !GetWorld() || !GetWorld()->IsGameWorld()) return;

    // 기존 상자들 제거
    for (AActor* B : SpawnedBoxes)
    {
        if (IsValid(B)) B->Destroy();
    }
    SpawnedBoxes.Reset();

    if (!BreakableBoxClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MapBuilder] BreakableBoxClass가 지정되지 않았습니다!"));
        return;
    }

    const float S = TileSize;
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    for (int32 Y = 0; Y < MapGrid.Height; ++Y)
    {
        for (int32 X = 0; X < MapGrid.Width; ++X)
        {
            if (MapGrid.GetTile(X, Y) == ESpartaArcadeTileType::DestructibleBox)
            {
                FVector SpawnLoc = TileToWorld(X, Y);
                SpawnLoc.Z = GetActorLocation().Z; // 지면에 딱 스냅 고정

                AActor* BoxActor = GetWorld()->SpawnActor<AActor>(BreakableBoxClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
                if (BoxActor)
                {
                    SpawnedBoxes.Add(BoxActor);
                }
            }
        }
    }
    UE_LOG(LogTemp, Display, TEXT("[MapBuilder] Spawned %d breakable box actors"), SpawnedBoxes.Num());
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

// 스폰 위치 좌표 정보가 복제 완료되었을 때 호출되어 비주얼을 정확하게 다시 렌더링
void ASpartaArcadeMapBuilder::OnRep_SpawnWorldLocations()
{
    if (bVisualsBuilt)
    {
        BuildVisuals();
    }
}

void ASpartaArcadeMapBuilder::BuildVisuals()
{
    if (MapGrid.Tiles.Num() == 0)
    {
        return;
    }

    // Floor 및 FloorPlane 데이터 양방향 동기화 보완
    auto SyncVisualInfo = [this](ESpartaArcadeTileType PrimaryType, ESpartaArcadeTileType AliasType)
    {
        bool bHasPrimary = TileVisualMap.Contains(PrimaryType);
        bool bHasAlias = TileVisualMap.Contains(AliasType);

        if (bHasAlias && (!bHasPrimary || !TileVisualMap[PrimaryType].Mesh))
        {
            TileVisualMap.Add(PrimaryType, TileVisualMap[AliasType]);
        }
        else if (bHasPrimary && (!bHasAlias || !TileVisualMap[AliasType].Mesh))
        {
            TileVisualMap.Add(AliasType, TileVisualMap[PrimaryType]);
        }
    };

    SyncVisualInfo(ESpartaArcadeTileType::Empty, ESpartaArcadeTileType::Floor);
    SyncVisualInfo(ESpartaArcadeTileType::Void, ESpartaArcadeTileType::FloorPlane);

    // TileVisualMap에 들어있는 비주얼 데이터를 추출하여 각 ISM 컴포넌트에 동적 반영
    auto ApplyVisualToISM = [this](ESpartaArcadeTileType TileType, UHierarchicalInstancedStaticMeshComponent* ISMComp)
    {
        if (!ISMComp) return;
        if (TileVisualMap.Contains(TileType))
        {
            const FSpartaArcadeTileVisualInfo& Info = TileVisualMap[TileType];
            if (Info.Mesh)
            {
                ISMComp->SetStaticMesh(Info.Mesh);
            }
        }
    };

    ApplyVisualToISM(ESpartaArcadeTileType::Empty, FloorISM);
    ApplyVisualToISM(ESpartaArcadeTileType::FixedWall, WallISM);
    ApplyVisualToISM(ESpartaArcadeTileType::FixedWall, PillarISM); // 기둥도 FixedWall 메쉬 쉐어
    ApplyVisualToISM(ESpartaArcadeTileType::DestructibleBox, BoxISM);
    ApplyVisualToISM(ESpartaArcadeTileType::Ice, IceISM);
    ApplyVisualToISM(ESpartaArcadeTileType::MudWater, MudWaterISM);
    ApplyVisualToISM(ESpartaArcadeTileType::Bush, BushISM);

    if (TileVisualMap.Contains(ESpartaArcadeTileType::Empty) && TileVisualMap[ESpartaArcadeTileType::Empty].Mesh)
    {
        FloorPlane->SetStaticMesh(TileVisualMap[ESpartaArcadeTileType::Empty].Mesh);
    }

    WallISM->ClearInstances();
    PillarISM->ClearInstances();
    BoxISM->ClearInstances();
    FloorISM->ClearInstances();
    IceISM->ClearInstances();
    MudWaterISM->ClearInstances();
    BushISM->ClearInstances();
    BoxCellToInstance.Reset();   // 박스 셀→인스턴스 매핑도 처음부터

    // 런타임 게임월드에서는 진짜 액터가 스폰되므로 중복 렌더링/충돌 방지를 위해 BoxISM 생성을 차단 (에디터 프리뷰 모드에서만 그리도록 설계)
    bool bShouldDrawBoxISM = true;
    if (GetWorld() && GetWorld()->IsGameWorld())
    {
        bShouldDrawBoxISM = false;
    }

    const float S = TileSize;
    const float CubeUU = 100.f;            // 엔진 큐브/플레인 기본 크기(1m)
    const float WallScale = S / CubeUU;

    // 배경 바닥 플레인: 맵 전체를 덮게(= 빈 공간 색). 룸은 그 위에 밝은 타일로 따로.
    if (FloorPlane->GetStaticMesh())
    {
        // 클라이언트 갱신(Replicate) 시에도 올바른 바닥 크기 및 위치 동기화를 보장하기 위해 GridWidth/Height 대신 MapGrid.Width/Height 사용
        FloorPlane->SetRelativeScale3D(FVector(MapGrid.Width * S / CubeUU, MapGrid.Height * S / CubeUU, 1.f));
        FloorPlane->SetRelativeLocation(FVector((MapGrid.Width - 1) * S * 0.5f, (MapGrid.Height - 1) * S * 0.5f, 0.f));
    }

    // 타일별 트랜스폼을 먼저 전부 모은 뒤 '한 번에' 추가(배치).
    // 개별 AddInstance 1만 2천 번은 에디터에서 호출마다 내비/이벤트 갱신이 붙어 수 초 멈춤 → 배치로 컴포넌트당 1번.
    TArray<FTransform> FloorXfs, WallXfs, PillarXfs, BoxXfs, IceXfs, MudXfs, BushXfs;
    TArray<int32> BoxCells;   // BoxXfs와 같은 순서의 셀 인덱(Y*W+X)
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

    // TMap 기반의 트랜스폼 연산을 위해 타입별 비주얼 설정(스케일, 오프셋) 로드
    auto GetVisualInfo = [this](ESpartaArcadeTileType TileType, FVector DefaultScale, FVector DefaultOffset)
    {
        FVector Scale = DefaultScale;
        FVector Offset = DefaultOffset;
        if (TileVisualMap.Contains(TileType))
        {
            const FSpartaArcadeTileVisualInfo& Info = TileVisualMap[TileType];
            Scale = DefaultScale * Info.ScaleMultiplier;
            Offset = DefaultOffset + Info.Offset;
        }
        return TPair<FVector, FVector>(Scale, Offset);
    };

    TPair<FVector, FVector> EmptyVis = GetVisualInfo(ESpartaArcadeTileType::Empty, FVector(WallScale), FVector(0.f, 0.f, 1.f));
    TPair<FVector, FVector> WallVis = GetVisualInfo(ESpartaArcadeTileType::FixedWall, FVector(WallScale), FVector(0.f, 0.f, S * 0.5f));
    TPair<FVector, FVector> PillarVis = GetVisualInfo(ESpartaArcadeTileType::FixedWall, FVector(WallScale * 0.85f, WallScale * 0.85f, WallScale), FVector(0.f, 0.f, S * 0.5f));
    TPair<FVector, FVector> BoxVis = GetVisualInfo(ESpartaArcadeTileType::DestructibleBox, FVector(WallScale * 0.8f, WallScale * 0.8f, WallScale * 0.6f), FVector(0.f, 0.f, S * 0.3f));
    TPair<FVector, FVector> IceVis = GetVisualInfo(ESpartaArcadeTileType::Ice, FVector(WallScale), FVector(0.f, 0.f, 2.f));
    TPair<FVector, FVector> MudVis = GetVisualInfo(ESpartaArcadeTileType::MudWater, FVector(WallScale), FVector(0.f, 0.f, 2.f));
    TPair<FVector, FVector> BushVis = GetVisualInfo(ESpartaArcadeTileType::Bush, FVector(WallScale), FVector(0.f, 0.f, 2.f));

    // 동적 모퉁이 방 검색 결과로 유동 결정된 스폰 위치 좌표를 복제된 SpawnWorldLocations를 기반으로 정확히 겹침 판정
    auto IsSpawnCell = [this](int32 CX, int32 CY) -> bool
    {
        for (const FVector& SpawnLoc : SpawnWorldLocations)
        {
            int32 SpawnX = 0, SpawnY = 0;
            if (WorldToTile(SpawnLoc, SpawnX, SpawnY))
            {
                if (CX == SpawnX && CY == SpawnY)
                {
                    return true;
                }
            }
        }
        return false;
    };

    for (int32 Y = 0; Y < MapGrid.Height; ++Y)
    {
        for (int32 X = 0; X < MapGrid.Width; ++X)
        {
            const FVector Pos(X * S, Y * S, 0.f);
            switch (MapGrid.GetTile(X, Y))
            {
            case ESpartaArcadeTileType::Empty:
                FloorXfs.Emplace(FRotator::ZeroRotator, Pos + EmptyVis.Value, EmptyVis.Key);
                break;
            case ESpartaArcadeTileType::FixedWall:
            {
                // 스폰 위치와 겹치는 고정벽의 경우 인스턴스를 그리지 않고 바닥으로 안전하게 우회 대체
                if (IsSpawnCell(X, Y))
                {
                    FloorXfs.Emplace(FRotator::ZeroRotator, Pos + EmptyVis.Value, EmptyVis.Key);
                    break;
                }

                if (CountFixedNeighbors(X, Y) <= 1)
                {
                    PillarXfs.Emplace(FRotator::ZeroRotator, Pos + PillarVis.Value, PillarVis.Key);
                }
                else
                {
                    WallXfs.Emplace(FRotator::ZeroRotator, Pos + WallVis.Value, WallVis.Key);
                }
                break;
            }
            case ESpartaArcadeTileType::DestructibleBox:
                // 스폰 위치와 겹치는 상자의 경우 그리지 않고 바닥으로 우회 대체
                if (IsSpawnCell(X, Y))
                {
                    FloorXfs.Emplace(FRotator::ZeroRotator, Pos + EmptyVis.Value, EmptyVis.Key);
                    break;
                }
                if (bShouldDrawBoxISM)
                {
                    BoxXfs.Emplace(FRotator::ZeroRotator, Pos + BoxVis.Value, BoxVis.Key);
                    BoxCells.Add(MapGrid.IndexOf(X, Y));
                }
                break;
            case ESpartaArcadeTileType::Ice:
                IceXfs.Emplace(FRotator::ZeroRotator, Pos + IceVis.Value, IceVis.Key);
                break;
            case ESpartaArcadeTileType::MudWater:
                MudXfs.Emplace(FRotator::ZeroRotator, Pos + MudVis.Value, MudVis.Key);
                break;
            case ESpartaArcadeTileType::Bush:
                BushXfs.Emplace(FRotator::ZeroRotator, Pos + BushVis.Value, BushVis.Key);
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

    // 색칠: 컴포넌트별 다이내믹 머티리얼 인스턴스로 색 지정 또는 커스텀 머티리얼 바인딩.
    auto ColorComp = [this](UMeshComponent* Comp, ESpartaArcadeTileType TileType, FLinearColor DefaultColor)
    {
        if (!Comp) return;

        UMaterialInterface* TargetMat = nullptr;
        FLinearColor TargetColor = DefaultColor;

        if (TileVisualMap.Contains(TileType))
        {
            const FSpartaArcadeTileVisualInfo& Info = TileVisualMap[TileType];
            if (Info.Material)
            {
                TargetMat = Info.Material;
            }
            TargetColor = Info.BlockoutColor;
        }

        if (TargetMat)
        {
            Comp->SetMaterial(0, TargetMat);
        }
        else if (BlockoutMaterial)
        {
            if (UMaterialInstanceDynamic* M = Comp->CreateDynamicMaterialInstance(0, BlockoutMaterial))
            {
                M->SetVectorParameterValue(TEXT("Color"), TargetColor);
                M->SetVectorParameterValue(TEXT("BaseColor"), TargetColor);
            }
        }
    };

    FLinearColor TmpVoidColor = FLinearColor(0.04f, 0.05f, 0.07f);
    if (TileVisualMap.Contains(ESpartaArcadeTileType::Void))
    {
        TmpVoidColor = TileVisualMap[ESpartaArcadeTileType::Void].BlockoutColor;
    }

    ColorComp(FloorPlane, ESpartaArcadeTileType::Void, TmpVoidColor);
    ColorComp(FloorISM, ESpartaArcadeTileType::Empty, FLinearColor(0.18f, 0.34f, 0.24f));
    ColorComp(WallISM, ESpartaArcadeTileType::FixedWall, FLinearColor(0.46f, 0.47f, 0.50f));
    ColorComp(PillarISM, ESpartaArcadeTileType::FixedWall, FLinearColor(0.33f, 0.37f, 0.52f));
    ColorComp(BoxISM, ESpartaArcadeTileType::DestructibleBox, FLinearColor(0.85f, 0.42f, 0.12f));
    ColorComp(IceISM, ESpartaArcadeTileType::Ice, FLinearColor(0.72f, 0.85f, 0.95f));
    ColorComp(MudWaterISM, ESpartaArcadeTileType::MudWater, FLinearColor(0.28f, 0.44f, 0.54f));
    ColorComp(BushISM, ESpartaArcadeTileType::Bush, FLinearColor(0.24f, 0.55f, 0.28f));

    FloorISM->MarkRenderStateDirty();
    WallISM->MarkRenderStateDirty();
    PillarISM->MarkRenderStateDirty();
    BoxISM->MarkRenderStateDirty();
    IceISM->MarkRenderStateDirty();
    MudWaterISM->MarkRenderStateDirty();
    BushISM->MarkRenderStateDirty();

    bVisualsBuilt = true;   // 이후 OnRep은 증분 갱신 경로로

    // 스폰 포인트 겹침 구조물 제거 함수 호출
    ClearStructuresAtSpawns();
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

ESpartaArcadeTileType ASpartaArcadeMapBuilder::GetTileTypeAtWorldPosition(const FVector& WorldPos) const
{
    int32 TileX, TileY;
    if (WorldToTile(WorldPos, TileX, TileY))
    {
        return MapGrid.GetTile(TileX, TileY);
    }
    return ESpartaArcadeTileType::Void;
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

// 스폰 포인트에 겹치는 구조물 액터 및 ISM 인스턴스 강제 제거 함수 구현
void ASpartaArcadeMapBuilder::ClearStructuresAtSpawns()
{
    if (!GetWorld()) return;

    // 타일 크기 1칸 전체(90%)를 덮어 겹치는 모든 구조물 액터를 완전히 제거
    const float ClearRadius = TileSize * 0.90f; 

    for (const FVector& SpawnLoc : SpawnWorldLocations)
    {
        // 1. 해당 스폰 위치 반경 내에 존재하는 구조물/장애물 액터들 탐색 후 파괴
        TArray<AActor*> OverlappingActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), OverlappingActors);

        for (AActor* Actor : OverlappingActors)
        {
            if (!IsValid(Actor) || Actor == this || Actor->IsA(APlayerStart::StaticClass())) continue;

            FVector ActorLoc = Actor->GetActorLocation();
            float Distance2D = FVector::Dist2D(SpawnLoc, ActorLoc);
            float DistanceZ = FMath::Abs(SpawnLoc.Z - ActorLoc.Z);

            if (Distance2D < ClearRadius && DistanceZ < 200.f)
            {
                FString ClassName = Actor->GetClass()->GetName();
                if (ClassName.Contains(TEXT("Wall")) || 
                    ClassName.Contains(TEXT("Block")) || 
                    ClassName.Contains(TEXT("Box")) || 
                    ClassName.Contains(TEXT("Pillar")) ||
                    ClassName.Contains(TEXT("Obstacle")))
                {
                    UE_LOG(LogTemp, Warning, TEXT("[MapBuilder] 스폰 위치(%s)에 겹치는 구조물 액터 %s 를 파괴합니다."), *SpawnLoc.ToString(), *Actor->GetName());
                    Actor->Destroy();
                }
            }
        }

        // 2. ISM 인스턴스 (WallISM, PillarISM, BoxISM 등) 중 스폰 위치와 겹치는 인스턴스 제거
        // 언리얼 피지컬/월드 트랜스폼 지연 오류를 완벽히 우회하기 위해 로컬 좌표 기준으로 안전하고 정확하게 제거
        FVector LocalSpawnLoc = SpawnLoc - GetActorLocation();

        TArray<UHierarchicalInstancedStaticMeshComponent*> ISMComponents = { WallISM, PillarISM, BoxISM };
        for (UHierarchicalInstancedStaticMeshComponent* ISMComp : ISMComponents)
        {
            if (!ISMComp) continue;

            TArray<int32> InstancesToRemove;
            int32 InstanceCount = ISMComp->GetInstanceCount();
            for (int32 i = 0; i < InstanceCount; ++i)
            {
                FTransform InstTransform;
                if (ISMComp->GetInstanceTransform(i, InstTransform, false)) // 로컬 스페이스 획득
                {
                    FVector InstLoc = InstTransform.GetLocation();
                    float Dist2D = FVector::Dist2D(LocalSpawnLoc, InstLoc);
                    float DistZ = FMath::Abs(LocalSpawnLoc.Z - InstLoc.Z);

                    if (Dist2D < ClearRadius && DistZ < 200.f)
                    {
                        InstancesToRemove.Add(i);
                    }
                }
            }

            for (int32 i = InstancesToRemove.Num() - 1; i >= 0; --i)
            {
                UE_LOG(LogTemp, Warning, TEXT("[MapBuilder] 스폰 로컬 위치(%s)에 겹치는 ISM 인스턴스를 제거합니다. (%s, 인덱스: %d)"), *LocalSpawnLoc.ToString(), *ISMComp->GetName(), InstancesToRemove[i]);
                ISMComp->RemoveInstance(InstancesToRemove[i]);
            }
        }
    }
}