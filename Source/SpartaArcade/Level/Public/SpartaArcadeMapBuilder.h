#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpartaArcadeMapGrid.h"
#include "SpartaArcadeMapTypes.h"
#include "SpartaArcadeMapBuilder.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UDataTable;

/** 런타임에 칸이 바뀔 때 알림(서버·클라 각자 로컬 브로드캐스트). UI 미니맵 갱신 등 구독용. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSpartaArcadeOnTileChanged, int32, TileX, int32, TileY);

/**
 * 서버 권위 절차적 맵 빌더 (정렬 방식 + ISM 렌더링).
 *
 * 흐름(효율적 네트워크 패턴):
 *   1) 서버가 그리드 데이터를 생성(시드 기반) → 작은 그리드 하나만 복제
 *   2) 서버·각 클라가 복제된 그리드를 읽어 자기 쪽에서 벽/박스를 ISM 인스턴스로 로컬 생성
 *      → 액터 수천 개를 복제하지 않음(대역폭 거의 0, 100x100도 가뿐)
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

    // 서버 측에서 맵 데이터 빌드가 완료되었는지 여부 (게임모드 선제 맵 빌드 감지용)
    UPROPERTY(Transient, BlueprintReadOnly, Category = "SpartaArcade|Map")
    bool bMapBuilt = false;

    /** 서버: 그리드 데이터 생성(복제됨). */
    UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Map")
    void BuildMap();

    /** 서버·클라 공통: 그리드를 읽어 벽/박스 인스턴스 + 바닥 배치(로컬, 복제 안 함) */
    void BuildVisuals();

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

    // ---- 런타임 그리드 갱신 (E 연동: 박스 파괴 → 칸 뚫림) ----

    /** [서버 전용] 부서지는 박스 칸을 Empty로 갱신. 그리드 복제로 전 클라에 자동 반영되고,
     *  장애물 길찾기(IsCellWalkable)·그리드 조회가 즉시 뚫린 칸을 인식.
     *  ▶ 게임 시스템: 박스 액터(ASpartaArcadeBlock) 파괴 시 이 함수(또는 월드 좌표 버전)를 호출.
     *  성공(실제로 박스였고 뚫림) 시 true. 격자 밖/박스 아님/중복 호출이면 false(안전). */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SpartaArcade|Map")
    bool NotifyTileDestroyed(int32 X, int32 Y);

    /** [서버 전용] 월드 좌표 버전 — 박스 액터가 자기 GetActorLocation()만 넘기면 됨. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SpartaArcade|Map")
    bool NotifyTileDestroyedAtWorld(const FVector& WorldPos);

    /** 런타임 칸 변경 알림(서버·클라 각자 로컬에서 발생). UI 미니맵 실시간 갱신 등 구독용. */
    UPROPERTY(BlueprintAssignable, Category = "SpartaArcade|Map")
    FSpartaArcadeOnTileChanged OnTileChanged;

    // ---- 에디터 프리뷰 버튼(시드 QA·수치 튜닝용, PIE 없이 즉시 재생성) ----

    /** [에디터] 현재 Seed로 맵 재생성 + 비주얼 갱신(액터 스폰 없음). Seed=0이면 임시 랜덤(사용 시드는 로그에 출력). */
    UFUNCTION(CallInEditor, Category = "SpartaArcade|Map", meta = (DisplayName = "Regenerate Map"))
    void EditorRegenerate();

    /** [에디터] 랜덤 시드를 뽑아 Seed 프로퍼티에 기록하고 재생성 — 마음에 든 맵의 시드가 그대로 남음.
     *  ★ 실제 매치를 매판 랜덤으로 돌리려면 커밋 전에 Seed를 다시 0으로. */
    UFUNCTION(CallInEditor, Category = "SpartaArcade|Map", meta = (DisplayName = "New Random Seed"))
    void EditorRandomSeed();

    /** [에디터] 프리뷰 비주얼·그리드 제거(레벨 저장 용량 절약용). */
    UFUNCTION(CallInEditor, Category = "SpartaArcade|Map", meta = (DisplayName = "Clear Preview"))
    void EditorClearPreview();

protected:
    virtual void BeginPlay() override;

#if WITH_EDITOR
    /** 에디터에서 프로퍼티 변경 감지 — 체크박스 트리거 실행 + 라이브 프리뷰. */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    // ---- 밸런싱 DataTable (팀 컨벤션과 동일 패턴) ----
    /** 맵 생성 수치 DT. 비우면 아래 BP 수치를 그대로 사용. Row 구조: FSpartaArcadeMapGenRow */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Data")
    UDataTable* MapGenTable = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Data")
    FName MapGenRowName = TEXT("Default");

    /** 켜면 매치(런타임) 시작 시 DT 수치를 로드해 아래 프로퍼티를 덮어씀(밸런싱 창구를 DT 하나로). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Data")
    bool bUseTableAtRuntime = true;

    /** [에디터] 체크 → DT 수치를 지금 프로퍼티로 불러오고 재생성(DT 값 확인용). */
    UPROPERTY(EditAnywhere, Transient, Category = "SpartaArcade|Data")
    bool bLoadFromTable = false;

    /** DT Row → 프로퍼티 복사. 성공 시 true(테이블 없음/Row 없음이면 false, BP 수치 유지). */
    bool ApplyMapGenRow();

    // ---- 맵 설정 ----
    // ---- 에디터 원클릭 실행(체크박스 방식) : 체크하면 실행되고 자동으로 꺼짐 ----
    /** 체크 → 현재 Seed로 맵 즉시 재생성. */
    UPROPERTY(EditAnywhere, Transient, Category = "SpartaArcade|Map")
    bool bRegenerateMap = false;

    /** 체크 → 랜덤 시드를 뽑아 Seed에 기록하고 재생성(마음에 든 맵의 시드가 남음). */
    UPROPERTY(EditAnywhere, Transient, Category = "SpartaArcade|Map")
    bool bNewRandomSeed = false;

    /** 체크 → 프리뷰 비주얼·그리드 제거(레벨 저장 용량 절약). */
    UPROPERTY(EditAnywhere, Transient, Category = "SpartaArcade|Map")
    bool bClearPreview = false;

    /** 켜두면 SpartaArcade 수치를 바꿀 때마다 맵이 즉시 재생성(라이브 튜닝). */
    UPROPERTY(EditAnywhere, Transient, Category = "SpartaArcade|Map")
    bool bLivePreview = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Map")
    int32 Seed = 1;                 // 같은 값=같은 맵. 0이면 랜덤(로그에 시드 출력)

    // 서버 측에서 결정되어 클라이언트로 복제 동기화되는 최종 랜덤 시드
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Map")
    int32 UsedSeed = 0;

    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Map")
    int32 GridWidth = 100;

    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Map")
    int32 GridHeight = 100;

    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Map")
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
    UPROPERTY(ReplicatedUsing = OnRep_SpawnWorldLocations, VisibleAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Spawns")
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

    // ---- [테스트] 박스 파괴 시뮬레이션(그리드 갱신·복제 확인용) ----
    /** 켜면 서버가 TestDestroyInterval마다 임의 박스 1개를 NotifyTileDestroyed로 파괴(기본 꺼짐).
     *  진짜 박스 액터 없이도 "파괴 → 칸 뚫림 → 장애물 통과 → 클라 동기화"를 눈으로 확인 가능. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Test")
    bool bDestroyBoxesForTest = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Test", meta = (ClampMin = "0.05"))
    float TestDestroyInterval = 1.f;

    FTimerHandle TestDestroyTimerHandle;
    void TestDestroyRandomBox();
	
    // 각 지형 타일별 비주얼을 블루프린트에서 관리하기 위한 TMap 추가
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Visual")
    TMap<ESpartaArcadeTileType, FSpartaArcadeTileVisualInfo> TileVisualMap;

    // 기존 참조 호환을 위한 BlockoutMaterial 정의 보존
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Visual")
    UMaterialInterface* BlockoutMaterial = nullptr;

    // 런타임에 스폰할 진짜 파괴 가능한 상자 블루프린트 클래스
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Spawns")
    TSubclassOf<AActor> BreakableBoxClass;

    // 스폰된 상자 액터 관리용 배열
    UPROPERTY()
    TArray<AActor*> SpawnedBoxes;

    // 런타임 상자 액터 스폰 헬퍼 함수
    void SpawnBreakableBoxes();
	
    // ---- 상태(복제) ----
    UPROPERTY(ReplicatedUsing = OnRep_MapGrid)
    FSpartaArcadeMapGrid MapGrid;

    UFUNCTION()
    void OnRep_MapGrid();

    UFUNCTION()
    void OnRep_SpawnWorldLocations();

    // ---- 컴포넌트 ----
    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    USceneComponent* SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UStaticMeshComponent* FloorPlane;

    /** 룸 바닥을 칸별 타일로(밝은 색) → 빈 공간(어두운 배경 플레인)과 구분. */
    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UHierarchicalInstancedStaticMeshComponent* FloorISM;

    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UHierarchicalInstancedStaticMeshComponent* WallISM;

    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UHierarchicalInstancedStaticMeshComponent* BoxISM;

    /** 실내 기둥(홀로 선 FixedWall) — 벽과 시각 구분용. 그리드 의미·충돌은 벽과 동일. */
    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UHierarchicalInstancedStaticMeshComponent* PillarISM;

    /** 변형 타일(지형 효과) — 바닥 타일처럼 평평하게, 색만 다르게. */
    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UHierarchicalInstancedStaticMeshComponent* IceISM;

    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UHierarchicalInstancedStaticMeshComponent* MudWaterISM;

    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Visual")
    UHierarchicalInstancedStaticMeshComponent* BushISM;

    /** 순수 데이터 생성(그리드·스폰 좌표·연결성 로그). 액터 스폰 없음 — 에디터 프리뷰/런타임 공용. */
    void GenerateGridData();

    /** 테스트용 장애물/자기장 액터 스폰(런타임 전용, BuildMap에서 호출). */
    void SpawnTestActors();
	
    /** 박스 셀 인덱스(Y*W+X) → BoxISM 인스턴스 인덱스.
     *  파괴 시 인스턴스를 '숨김'(제거 아님)으로 처리해 다른 인스턴스 인덱스가 안 흔들리게 유지. */
    TMap<int32, int32> BoxCellToInstance;

    /** BuildVisuals가 한 번이라도 돌았는지. 이후 OnRep은 전체 재구축 대신 증분 갱신. */
    bool bVisualsBuilt = false;

    /** 그리드와 대조해 더 이상 박스가 아닌 칸의 인스턴스를 숨김(서버·클라 공통 증분 갱신). */
    void RefreshDestroyedBoxVisuals();

    // 스폰 포인트에 겹치는 액터 및 ISM 인스턴스 강제 제거 함수
    void ClearStructuresAtSpawns();
};