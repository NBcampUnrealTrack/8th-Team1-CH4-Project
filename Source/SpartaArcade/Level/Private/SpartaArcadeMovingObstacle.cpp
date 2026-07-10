#include "SpartaArcadeMovingObstacle.h"
#include "SpartaArcadeMapBuilder.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

ASpartaArcadeMovingObstacle::ASpartaArcadeMovingObstacle()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);   // 서버 이동 → 클라 위치 복제(네트워크 팀과 세부 조율 가능)

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(45.f);
    Collision->SetCollisionProfileName(TEXT("OverlapAllDynamic")); // 접촉 감지는 게임 시스템이 처리(물리 차단 X)
    SetRootComponent(Collision);

    ObstacleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObstacleMesh"));
    ObstacleMesh->SetupAttachment(Collision);
    ObstacleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ObstacleMesh->SetRelativeScale3D(FVector(0.8f));

    // 회전 제어 변수 기본값 초기화
    bOrientRotationToMovement = true;
    bUseSelfRotation = false;
    SelfRotationSpeed = 180.f;
}

void ASpartaArcadeMovingObstacle::SetMapBuilder(ASpartaArcadeMapBuilder* InMap)
{
    Map = InMap;

    // 맵 빌더가 런타임에 설정되더라도 안전하게 첫 번째 타겟 목표 칸을 결정하여 즉시 이동을 개시하도록 보완
    if (Map.IsValid() && !bHasTarget && HasAuthority())
    {
        int32 X = 0, Y = 0;
        Map->WorldToTile(GetActorLocation(), X, Y);
        CurCell = FIntPoint(X, Y);
        ChooseNextTarget();
    }
}

void ASpartaArcadeMovingObstacle::ApplyObstacleRow()
{
    if (!ObstacleTable) return;

    const FSpartaArcadeObstacleRow* Row =
        ObstacleTable->FindRow<FSpartaArcadeObstacleRow>(ObstacleRowName, TEXT("ApplyObstacleRow"));
    if (!Row)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Obstacle] ObstacleTable에 Row '%s'가 없어요 — 기본 수치 사용"),
            *ObstacleRowName.ToString());
        return;
    }

    MoveSpeed = Row->MoveSpeed;
    HoverHeight = Row->HoverHeight;
}

void ASpartaArcadeMovingObstacle::BeginPlay()
{
    Super::BeginPlay();
    ApplyObstacleRow();            // DT가 있으면 속도/부양 높이 로드(아래 PlaneZ 계산 전에)

    // 이전 위치 초기화 (서버/클라 모두 필요하므로 권한 체크 이전에 연산)
    PrevLocation = GetActorLocation();

    if (!HasAuthority()) return;   // 이동은 서버 권위(클라는 복제로 위치 수신)

    if (!Map.IsValid())
    {
        Map = Cast<ASpartaArcadeMapBuilder>(
            UGameplayStatics::GetActorOfClass(GetWorld(), ASpartaArcadeMapBuilder::StaticClass()));
    }

    // 지면에서 살짝 띄우고, 그 높이를 이동 평면으로 고정.
    PlaneZ = GetActorLocation().Z + HoverHeight;
    SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, PlaneZ));

    if (Map.IsValid())
    {
        int32 X = 0, Y = 0;
        Map->WorldToTile(GetActorLocation(), X, Y);
        CurCell = FIntPoint(X, Y);
        ChooseNextTarget();
    }
}

bool ASpartaArcadeMovingObstacle::CellOpen(const FIntPoint& C) const
{
    return Map.IsValid() && Map->IsCellWalkable(C.X, C.Y);
}

void ASpartaArcadeMovingObstacle::ChooseNextTarget()
{
    static const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };

    // 1) 직진 우선
    if (CellOpen(CurCell + Dir))
    {
        TargetCell = CurCell + Dir;
        bHasTarget = true;
        return;
    }

    // 2) 막힘 → 후진 제외한 열린 방향 중 랜덤 전환
    const FIntPoint Back(-Dir.X, -Dir.Y);
    TArray<FIntPoint> Open;
    for (const FIntPoint& D : Dirs)
        if (D != Back && CellOpen(CurCell + D)) Open.Add(D);

    // 3) 막다른 곳이면 후진 허용
    if (Open.Num() == 0 && CellOpen(CurCell + Back)) Open.Add(Back);

    if (Open.Num() > 0)
    {
        Dir = Open[FMath::RandRange(0, Open.Num() - 1)];
        TargetCell = CurCell + Dir;
        bHasTarget = true;
    }
    else
    {
        bHasTarget = false;   // 완전히 갇힘(드묾) — 정지
    }
}

void ASpartaArcadeMovingObstacle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bUseSelfRotation && ObstacleMesh)
    {
        ObstacleMesh->AddLocalRotation(FRotator(0.f, SelfRotationSpeed * DeltaSeconds, 0.f));
    }

    FVector CurLocation = GetActorLocation();

    // 서버와 클라이언트 모두 매 프레임 적용되므로 뚝뚝 끊기지 않고 부드러운 방향 회전을 보장합니다.
    if (bOrientRotationToMovement)
    {
        FVector DeltaMove = CurLocation - PrevLocation;
        DeltaMove.Z = 0.f; // 2D 평면상에서의 회전

        if (DeltaMove.SizeSquared() > 0.01f) // 미세 이동치 초과 시에만 방향 회전 적용
        {
            FRotator TargetRot = FRotationMatrix::MakeFromX(DeltaMove).Rotator();
            SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaSeconds, 15.f)); // 보간 속도 15.f
        }
    }
    PrevLocation = CurLocation; // 다음 프레임 연산을 위해 캐시 갱신

    if (!HasAuthority() || !Map.IsValid() || !bHasTarget) return;

    const FVector TargetW = Map->TileToWorld(TargetCell.X, TargetCell.Y);
    const FVector Cur = GetActorLocation();
    const FVector2D To(TargetW.X - Cur.X, TargetW.Y - Cur.Y);
    const float Dist = To.Size();
    const float Step = MoveSpeed * DeltaSeconds;

    if (Dist <= Step || Dist < KINDA_SMALL_NUMBER)
    {
        // 목표 칸 중심 도착 → 다음 방향 결정
        SetActorLocation(FVector(TargetW.X, TargetW.Y, PlaneZ));
        CurCell = TargetCell;
        ChooseNextTarget();
    }
    else
    {
        const FVector2D Move = To / Dist * Step;
        SetActorLocation(FVector(Cur.X + Move.X, Cur.Y + Move.Y, PlaneZ));
    }
}