#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpartaArcadeMapGrid.h"
#include "SpartaArcadeMapBuilder.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;

/**
 * 서버 권위 절차적 맵 빌더 (정렬 방식 + ISM 렌더링).
 *
 * 흐름(효율적 네트워크 패턴):
 *   1) 서버가 그리드 데이터를 생성(시드 기반) → 작은 그리드 하나만 복제
 *   2) 서버·각 클라가 복제된 그리드를 읽어 자기 쪽에서 벽/박스를 ISM 인스턴스로 로컬 생성
 *
 * 벽·바닥은 정적이라 양쪽이 똑같이 그리면 되니 복제 불필요.
 * (지금은 블록아웃: 박스도 ISM 시각화. 폭탄에 부서지는 진짜 박스는 통합 단계에서 게임 시스템 액터로 교체.)
 */
UCLASS()
class SPARTAARCADE_API ASpartaArcadeMapBuilder : public AActor
{
    GENERATED_BODY()

public:
    ASpartaArcadeMapBuilder();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Map")
    void BuildMap();

    /** 칸 (X,Y)의 월드 좌표(이 액터 위치 기준). */
    UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Map")
    FVector TileToWorld(int32 X, int32 Y) const;

    /** 생성된 4모서리 스폰 월드 좌표를 반환(서버 권위). */
    UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Spawns")
    TArray<FVector> GetSpawnWorldLocations() const { return SpawnWorldLocations; }

    /** 이동 장애물 스폰 월드 좌표(중앙·스폰 제외). GameMode가 장애물 액터 스폰에 사용. */
    UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Obstacles")
    TArray<FVector> GetObstacleSpawnLocations() const { return ObstacleSpawnWorldLocations; }

    /** 칸이 통과 가능한지(빈 바닥/변형 타일=가능, 벽·박스·기둥·void=불가). 이동 장애물이 조회. */
    UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Map")
    bool IsCellWalkable(int32 X, int32 Y) const;

    /** 월드 좌표 → 타일 좌표(TileToWorld의 역). 격자 밖이면 false. */
    UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Map")
    bool WorldToTile(const FVector& World, int32& OutX, int32& OutY) const;

    UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Map")
    int32 GetGridWidth() const { return MapGrid.Width; }

    UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Map")
    int32 GetGridHeight() const { return MapGrid.Height; }

    UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Map")
    float GetTileSize() const { return TileSize; }

    /** 중앙 아레나 bbox(자기장 최종 지대). 서버 생성 후 복제됨. */
    UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Map")
    void GetCenterBounds(FIntPoint& OutMin, FIntPoint& OutMax) const { OutMin = CenterMin; OutMax = CenterMax; }

protected:
    virtual void BeginPlay() override;

    // ---- 맵 설정 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Map")
    int32 Seed = 1;                 // 같은 값=같은 맵. 0이면 랜덤(로그에 시드 출력)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Map")
    int32 GridWidth = 100;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Map")
    int32 GridHeight = 100;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Map")
    float TileSize = 100.f;         // 한 칸 월드 크기(uu). 100 = 1m

    // ---- 방 생성(정렬) ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Rooms", meta = (ClampMin = "2"))
    int32 SectorCols = 8;           // 가로 슬롯 수

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Rooms", meta = (ClampMin = "2"))
    int32 SectorRows = 8;           // 세로 슬롯 수

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Rooms", meta = (ClampMin = "1"))
    int32 Gap = 1;                  // 슬롯 사이 벽/문 두께(칸)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Rooms", meta = (ClampMin = "0"))
    int32 VoidSlots = 10;           // 제거할 작은 방 최대 개수(클수록 덜 참)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Rooms", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MergeChance = 0.75f;      // 직사각형/ㄴ자 방으로 합칠 확률

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Rooms", meta = (ClampMin = "1"))
    int32 CenterSlots = 2;          // 중앙 정사각형 크기(슬롯). 2 = 2x2

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Rooms", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ExtraOpenChance = 0.18f;  // 뚫린 길 추가(순환로) 확률. 높이면 덜 미로 같아짐

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Rooms", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BreakableWallChance = 0.3f; // 부술 수 있는 벽(폭탄 지름길) 빈도

    // ---- 방 내부 채우기(기둥/박스) ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Interior", meta = (ClampMin = "2"))
    int32 InteriorBlock = 5;          // 구역 블록 크기(칸). 클수록 큼직하게 정돈

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Interior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BoxDensity = 0.4f;          // 기둥 주변 박스 양

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Interior", meta = (ClampMin = "0"))
    int32 DoorClearRadius = 1;        // 열린 문 주변 비울 반경

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Interior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EmptyStyleWeight = 0.30f;   // 구역이 '빈 곳'일 비율

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Interior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RegularStyleWeight = 0.40f; // 구역이 '규칙 격자'일 비율(나머지는 어질러짐)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Interior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MessyPillarChance = 0.16f;  // 어질러짐 구역 랜덤 기둥 확률

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Interior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EmptyBoxChance = 0.05f;     // 빈 곳 구역 드문 박스 확률

    // ---- 스폰 + 안전구역 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Spawns", meta = (ClampMin = "0"))
    int32 SafeRadius = 3;             // 스폰 주변 비울 사각 반경(넉넉할수록 큰 시작 공간)

    /** 4모서리 스폰 월드 좌표(서버에서 생성 시 채워짐). GameMode가 플레이어 스폰에 사용. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Spawns")
    TArray<FVector> SpawnWorldLocations;

    // ---- 변형 타일(지형 효과) ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Variants", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float VariantCoverage = 0.25f;    // 바닥(빈 칸) 중 변형 타일 비율

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Variants")
    bool bVariantsInCenter = false;   // 중앙 아레나에도 변형 타일 배치할지

    // ---- 이동 장애물 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Obstacles", meta = (ClampMin = "0"))
    int32 NumObstacles = 10;          // 맵당 이동 장애물 수(디테일에서 조절)

    /** 이동 장애물 월드 스폰 좌표(서버 생성 시 채워짐). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Obstacles")
    TArray<FVector> ObstacleSpawnWorldLocations;

    /** 테스트용: 맵빌더가 직접 장애물을 스폰(서버). 실제 게임은 GameMode가 스폰해도 됨. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Obstacles")
    bool bSpawnObstaclesForTest = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Obstacles")
    TSubclassOf<AActor> ObstacleClass;

    /** 테스트 스폰된 장애물(재생성 시 정리용). */
    UPROPERTY()
    TArray<AActor*> SpawnedObstacles;

    // ---- 서든데스 자기장(중앙 bbox는 존매니저의 최종 지대) ----
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Zone")
    FIntPoint CenterMin = FIntPoint(0, 0);

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Zone")
    FIntPoint CenterMax = FIntPoint(0, 0);

    /** 테스트용: 맵빌더가 자기장 매니저를 직접 스폰(서버). 실제 게임은 GameMode가 스폰해도 됨. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Zone")
    bool bSpawnZoneForTest = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Zone")
    TSubclassOf<AActor> ZoneManagerClass;

    UPROPERTY()
    AActor* SpawnedZone = nullptr;

    // ---- 렌더용 메쉬/머티리얼 (비우면 엔진 기본 큐브/플레인 자동 사용) ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Visual")
    UStaticMesh* WallMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Visual")
    UStaticMesh* BoxMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Visual")
    UStaticMesh* FloorMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Visual")
    UMaterialInterface* BlockoutMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Visual|Colors")
    FLinearColor FloorColor = FLinearColor(0.18f, 0.34f, 0.24f);  // 룸 바닥(밝은 초록)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Visual|Colors")
    FLinearColor VoidColor = FLinearColor(0.04f, 0.05f, 0.07f);  // 빈 공간(어두운 배경)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Visual|Colors")
    FLinearColor WallColor = FLinearColor(0.46f, 0.47f, 0.50f);  // 부술 수 없는 벽(회색)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Visual|Colors")
    FLinearColor BoxColor = FLinearColor(0.85f, 0.42f, 0.12f);  // 부술 수 있는 벽/박스(주황)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Visual|Colors")
    FLinearColor IceColor = FLinearColor(0.72f, 0.85f, 0.95f); // 얼음(연파랑, 미끄럼)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Visual|Colors")
    FLinearColor MudWaterColor = FLinearColor(0.28f, 0.44f, 0.54f); // 물·진흙(파랑, 감속)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Visual|Colors")
    FLinearColor BushColor = FLinearColor(0.24f, 0.55f, 0.28f); // 덤불(초록, 은폐)

    // ---- 상태(복제) ----
    UPROPERTY(ReplicatedUsing = OnRep_MapGrid)
    FSpartaArcadeMapGrid MapGrid;

    UFUNCTION()
    void OnRep_MapGrid();

    // ---- 컴포넌트 ----
    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    USceneComponent* SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UStaticMeshComponent* FloorPlane;

    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UHierarchicalInstancedStaticMeshComponent* FloorISM;

    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UHierarchicalInstancedStaticMeshComponent* WallISM;

    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UHierarchicalInstancedStaticMeshComponent* BoxISM;

    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UHierarchicalInstancedStaticMeshComponent* IceISM;

    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UHierarchicalInstancedStaticMeshComponent* MudWaterISM;

    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UHierarchicalInstancedStaticMeshComponent* BushISM;

    void BuildVisuals();
};