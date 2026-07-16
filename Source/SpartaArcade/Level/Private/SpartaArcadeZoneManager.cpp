#include "SpartaArcadeZoneManager.h"
#include "SpartaArcadeMapBuilder.h"
#include "SpartaArcadeMovingObstacle.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Characters/Public/SpartaArcadeCharacter.h"
#include "Systems/Public/CombatComponent.h"

ASpartaArcadeZoneManager::ASpartaArcadeZoneManager()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    CrushISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CrushISM"));
    CrushISM->SetupAttachment(Root);
    CrushISM->SetCollisionProfileName(TEXT("BlockAll"));   // 압사 블록은 진입 차단

    WarningISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WarningISM"));
    WarningISM->SetupAttachment(Root);
    WarningISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeF.Succeeded()) CrushISM->SetStaticMesh(CubeF.Object);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneF(TEXT("/Engine/BasicShapes/Plane.Plane"));
    if (PlaneF.Succeeded()) WarningISM->SetStaticMesh(PlaneF.Object);
}

void ASpartaArcadeZoneManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ASpartaArcadeZoneManager, ShrinkProgress);
    DOREPLIFETIME(ASpartaArcadeZoneManager, Elapsed); 
}

void ASpartaArcadeZoneManager::SetMapBuilder(ASpartaArcadeMapBuilder* InMap)
{
    Map = InMap;
}

void ASpartaArcadeZoneManager::StartCountdown()
{
    bStarted = true;
    Elapsed = 0.f;
    ShrinkProgress = 0.f;
    RenderedCrush = 0;
    KillIndex = 0;
    if (CrushISM) CrushISM->ClearInstances();
    if (WarningISM) WarningISM->ClearInstances();
}

void ASpartaArcadeZoneManager::BeginPlay()
{
    Super::BeginPlay();

    ApplyZoneRow();   // DT가 있으면 타이밍/높이 로드 — 서버·클라 같은 에셋이라 항상 같은 값

    if (!Map.IsValid())
    {
        Map = Cast<ASpartaArcadeMapBuilder>(
            UGameplayStatics::GetActorOfClass(GetWorld(), ASpartaArcadeMapBuilder::StaticClass()));
    }

    ApplyColor(CrushISM, CrushColor);
    ApplyColor(WarningISM, WarningColor);

    if (HasAuthority())
    {
        bStarted = true;   // 테스트 자동 시작(GameMode가 StartCountdown으로 재시작 가능)
        Elapsed = 0.f;
    }
}

void ASpartaArcadeZoneManager::ApplyZoneRow()
{
    if (!ZoneTable) return;

    const FSpartaArcadeZoneRow* Row =
        ZoneTable->FindRow<FSpartaArcadeZoneRow>(ZoneRowName, TEXT("ApplyZoneRow"));
    if (!Row)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Zone] ZoneTable에 Row '%s'가 없어요 — 기본 수치 사용"),
            *ZoneRowName.ToString());
        return;
    }

    ActivationDelay = Row->ActivationDelay;
    ShrinkDuration = Row->ShrinkDuration;
    WarningLead = Row->WarningLead;
    BlockHeightTiles = Row->BlockHeightTiles;

    UE_LOG(LogTemp, Display, TEXT("[Zone] DT 수치 적용: %s / Row '%s'"),
        *ZoneTable->GetName(), *ZoneRowName.ToString());
}

void ASpartaArcadeZoneManager::ApplyColor(UHierarchicalInstancedStaticMeshComponent* Comp, const FLinearColor& Col)
{
    if (!Comp || !BlockoutMaterial) return;
    UMaterialInstanceDynamic* Dyn = Comp->CreateDynamicMaterialInstance(0, BlockoutMaterial);
    if (Dyn)
    {
        Dyn->SetVectorParameterValue(TEXT("Color"), Col);
        Dyn->SetVectorParameterValue(TEXT("BaseColor"), Col);
    }
}

// 맵이 준비되면(그리드+중앙 bbox 복제 후) 나선 순서를 한 번 계산.
void ASpartaArcadeZoneManager::TryBuildSpiral()
{
    if (bSpiralBuilt || !Map.IsValid()) return;
    const int32 W = Map->GetGridWidth(), H = Map->GetGridHeight();
    if (W <= 0 || H <= 0) return;
    FIntPoint CMin, CMax;
    Map->GetCenterBounds(CMin, CMax);
    if (CMax.X < CMin.X) return;   // 중앙 bbox 아직 미복제

    TArray<FIntPoint> Full;
    int32 t = 0, b = H - 1, l = 0, r = W - 1;
    while (t <= b && l <= r)   // 외곽부터 시계방향 나선(연속)
    {
        for (int32 x = l; x <= r; ++x) Full.Add(FIntPoint(x, t));
        for (int32 y = t + 1; y <= b; ++y) Full.Add(FIntPoint(r, y));
        if (t < b) for (int32 x = r - 1; x >= l; --x) Full.Add(FIntPoint(x, b));
        if (l < r) for (int32 y = b - 1; y > t; --y) Full.Add(FIntPoint(l, y));
        ++t; --b; ++l; --r;
    }

    SpiralCells.Reset();
    for (const FIntPoint& C : Full)   // 중앙 아레나(bbox) 안쪽은 제외 → 최종 안전지대
        if (!(C.X >= CMin.X && C.X <= CMax.X && C.Y >= CMin.Y && C.Y <= CMax.Y))
            SpiralCells.Add(C);

    bSpiralBuilt = true;
}

void ASpartaArcadeZoneManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    TryBuildSpiral();
    if (!bSpiralBuilt) return;

    if (HasAuthority() && bStarted)
    {
        Elapsed += DeltaSeconds;
        if (Elapsed >= ActivationDelay)
        {
            const float ShrinkT = Elapsed - ActivationDelay;
            ShrinkProgress = FMath::Clamp(ShrinkT / FMath::Max(1.f, ShrinkDuration), 0.f, 1.f);
            ProcessCrushKills();
        }
    }

    RefreshVisuals();   // 서버·클라 모두 진행도로 렌더
}

void ASpartaArcadeZoneManager::OnRep_Progress()
{
    RefreshVisuals();   // 클라: 복제 즉시 반영
}

void ASpartaArcadeZoneManager::RefreshVisuals()
{
    if (!Map.IsValid() || SpiralCells.Num() == 0) return;
    const int32 N = SpiralCells.Num();
    const float S = Map->GetTileSize();
    const float CubeScale = S / 100.f;   // 엔진 큐브/플레인 100단위 → 타일 크기
    const int32 DropIndex = FMath::Clamp(FMath::FloorToInt(ShrinkProgress * N), 0, N);

    // 낙하 블록 증분 추가(즉시 나타남 — 한 번 나오면 유지). 높이 = BlockHeightTiles 칸.
    const float HScale = CubeScale * FMath::Max(0.1f, BlockHeightTiles);
    const float HalfH = S * FMath::Max(0.1f, BlockHeightTiles) * 0.5f;   // 바닥에 딛고 위로
    while (RenderedCrush < DropIndex)
    {
        const FIntPoint C = SpiralCells[RenderedCrush];
        const FVector P = Map->TileToWorld(C.X, C.Y) + FVector(0, 0, HalfH);
        CrushISM->AddInstance(FTransform(FRotator::ZeroRotator, P, FVector(CubeScale, CubeScale, HScale)));
        ++RenderedCrush;
    }

    WarningISM->ClearInstances();
    if (Elapsed >= (ActivationDelay - WarningLead) && DropIndex < N)
    {
        const int32 WarnN = FMath::Max(1, FMath::FloorToInt(N * (WarningLead / FMath::Max(1.f, ShrinkDuration))));
        const int32 WarnEnd = FMath::Min(N, DropIndex + WarnN);
        for (int32 i = DropIndex; i < WarnEnd; ++i)
        {
            const FIntPoint C = SpiralCells[i];
            // 바닥 타일 머티리얼과 겹쳐서 깜빡이거나 묻히는 Z-fighting 방지를 위해 오프셋 높이를 10.f 로 상향 조정
            const FVector P = Map->TileToWorld(C.X, C.Y) + FVector(0, 0, 10.f);
            WarningISM->AddInstance(FTransform(FRotator::ZeroRotator, P, FVector(CubeScale)));
        }
    }
}

// 서버: 새로 낙하한 칸 위의 액터 처리(트리거만 — 사망 판정은 게임 시스템).
void ASpartaArcadeZoneManager::ProcessCrushKills()
{
    if (!Map.IsValid()) return;
    const int32 N = SpiralCells.Num();
    const int32 DropIndex = FMath::Clamp(FMath::FloorToInt(ShrinkProgress * N), 0, N);
    if (DropIndex <= KillIndex) return;

    const int32 W = Map->GetGridWidth();
    TSet<int32> NewCells;
    for (int32 i = KillIndex; i < DropIndex; ++i)
    {
        const FIntPoint C = SpiralCells[i];
        NewCells.Add(C.Y * W + C.X);
    }
    KillIndex = DropIndex;

    auto CellCrushed = [&](AActor* A) -> bool
        {
            int32 X = 0, Y = 0;
            Map->WorldToTile(A->GetActorLocation(), X, Y);
            return NewCells.Contains(Y * W + X);
        };

    // 플레이어(폰): 트리거 브로드캐스트 및 실질적 즉사(소멸) 처리 추가 (끼임 방지 및 즉사 메커니즘 확보)
    // 직접 Destroy() 호출 대신 CombatComponent->InstantEliminate()를 호출하여 사망 처리 정상화 및 HUD 갱신 연동
    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        if (CellCrushed(*It))
        {
            APawn* CrushedPawn = *It;
            OnActorCrushed.Broadcast(CrushedPawn);
            
            if (ASpartaArcadeCharacter* Character = Cast<ASpartaArcadeCharacter>(CrushedPawn))
            {
                if (UCombatComponent* CombatComp = Character->FindComponentByClass<UCombatComponent>())
                {
                    CombatComp->InstantEliminate();
                }
                else
                {
                    CrushedPawn->Destroy();
                }
            }
            else
            {
                CrushedPawn->Destroy();
            }
        }
    }

    // 이동 장애물: 자기장에 깔리면 소멸(반복 중 안전하게 모았다가 파괴)
    TArray<ASpartaArcadeMovingObstacle*> ToKill;
    for (TActorIterator<ASpartaArcadeMovingObstacle> It(GetWorld()); It; ++It)
        if (CellCrushed(*It)) ToKill.Add(*It);
    for (ASpartaArcadeMovingObstacle* O : ToKill)
    {
        OnActorCrushed.Broadcast(O);
        O->Destroy();
    }
}