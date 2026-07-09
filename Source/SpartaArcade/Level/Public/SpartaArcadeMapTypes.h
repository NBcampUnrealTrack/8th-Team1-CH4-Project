#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"   // 밸런싱 Row(FTableRowBase) 상속용
#include "SpartaArcadeMapTypes.generated.h"
// ↑ UENUM/USTRUCT/UCLASS을 쓰는 헤더는 항상 이 "*.generated.h"를 "마지막" include로.

/**
 * 맵 그리드 한 칸이 가질 수 있는 타일 종류.
 * 'enum class ... : uint8' = 1바이트(복제 가벼움) + 블루프린트 드롭다운 노출.
 */
UENUM(BlueprintType)
enum class ESpartaArcadeTileType : uint8
{
    Empty           UMETA(DisplayName = "Empty"),           // 빈 바닥 (이동 가능). 열린 문도 이 값.
    FixedWall       UMETA(DisplayName = "Fixed Wall"),       // 파괴 불가 벽 (방 외곽 폐합)
    DestructibleBox UMETA(DisplayName = "Destructible Box"), // 부서지는 박스(박스벽/지름길). 내용물은 게임 시스템 소관
    Ice             UMETA(DisplayName = "Ice"),              // 얼음: 미끄러짐
    MudWater        UMETA(DisplayName = "Mud / Water"),      // 물·진흙: 감속
    Bush            UMETA(DisplayName = "Bush"),             // 수풀: 은폐
    Conveyor        UMETA(DisplayName = "Conveyor"),         // 컨베이어: 강제 이동
    ZoneBlock       UMETA(DisplayName = "Zone Block"),       // 자기장 압사 블록 (낙하 후 영구 벽)
    Void            UMETA(DisplayName = "Void")              // 방 바깥 빈 공간. 렌더 안 함, 이동 불가.
};

/**
 * 방 모양 — 정렬 생성기에선 슬롯 병합으로 표현되며, 카탈로그 메타용으로 유지.
 */
UENUM(BlueprintType)
enum class ESpartaArcadeRoomShape : uint8
{
    Square  UMETA(DisplayName = "Square"),
    LShape  UMETA(DisplayName = "L-Shape"),
    Rect    UMETA(DisplayName = "Rectangle")
};

// ---- 밸런싱 DataTable Row (팀 컨벤션: BomberTypes.h의 FCharacterStatRow 등과 동일 패턴) ----
// 사용법: 콘텐츠 브라우저 우클릭 → Miscellaneous → Data Table → Row 구조로 아래 타입 선택 → "Default" Row 추가.

/** 맵 생성 수치 세트(맵빌더가 로드). ※ TileSize는 팀 코드(폭탄 스냅 등)가 100을 가정하므로 DT에서 제외. */
USTRUCT(BlueprintType)
struct FSpartaArcadeMapGenRow : public FTableRowBase
{
    GENERATED_BODY()

    // --- 그리드 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (ClampMin = "16"))
    int32 GridWidth = 100;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (ClampMin = "16"))
    int32 GridHeight = 100;

    // --- 방 배치 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms", meta = (ClampMin = "2"))
    int32 SectorCols = 8;               // 가로 슬롯 수

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms", meta = (ClampMin = "2"))
    int32 SectorRows = 8;               // 세로 슬롯 수

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms", meta = (ClampMin = "1"))
    int32 Gap = 1;                      // 슬롯 사이 벽/문 두께(칸)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms", meta = (ClampMin = "0"))
    int32 VoidSlots = 10;               // 제거할 작은 방 최대 개수

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MergeChance = 0.75f;          // 직사각형/ㄴ자 방으로 합칠 확률

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms", meta = (ClampMin = "1"))
    int32 CenterSlots = 2;              // 중앙 아레나 크기(슬롯)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ExtraOpenChance = 0.18f;      // 순환로(추가 문) 확률

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BreakableWallChance = 0.3f;   // 부술 수 있는 벽(지름길) 빈도

    // --- 방 내부 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interior", meta = (ClampMin = "2"))
    int32 InteriorBlock = 5;            // 구역 블록 크기(칸)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BoxDensity = 0.4f;            // 기둥 주변 박스 양

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interior", meta = (ClampMin = "0"))
    int32 DoorClearRadius = 1;          // 열린 문 주변 비울 반경

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EmptyStyleWeight = 0.30f;     // '빈 곳' 구역 비율

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RegularStyleWeight = 0.40f;   // '규칙 격자' 구역 비율(나머지=어질러짐)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MessyPillarChance = 0.16f;    // 어질러짐 구역 랜덤 기둥 확률

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EmptyBoxChance = 0.05f;       // 빈 곳 구역 드문 박스 확률

    // --- 스폰 / 변형 타일 / 장애물 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawns", meta = (ClampMin = "0"))
    int32 SafeRadius = 3;               // 스폰 안전구역 반경(칸)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variants", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float VariantCoverage = 0.25f;      // 변형 타일(얼음/물/덤불) 비율

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variants")
    bool bVariantsInCenter = false;     // 중앙 아레나에도 변형 타일

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacles", meta = (ClampMin = "0"))
    int32 NumObstacles = 10;            // 이동 장애물 수
};

/** 서든데스 자기장 타이밍/높이(존매니저가 로드). */
USTRUCT(BlueprintType)
struct FSpartaArcadeZoneRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone", meta = (ClampMin = "0.0"))
    float ActivationDelay = 90.f;       // 매치 시작 ~ 발동(초)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone", meta = (ClampMin = "1.0"))
    float ShrinkDuration = 210.f;       // 발동 ~ 종료(초)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone", meta = (ClampMin = "0.0"))
    float WarningLead = 1.5f;           // 경고 ~ 낙하 리드타임(초)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone", meta = (ClampMin = "0.1"))
    float BlockHeightTiles = 2.f;       // 압사 블록 높이(칸)
};

/** 이동 장애물 수치(장애물 액터가 로드). */
USTRUCT(BlueprintType)
struct FSpartaArcadeObstacleRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacle", meta = (ClampMin = "0.0"))
    float MoveSpeed = 450.f;            // cm/s — 플레이어보다 빠르게

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacle", meta = (ClampMin = "0.0"))
    float HoverHeight = 40.f;           // 지면에서 뜬 높이
};