#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "SpartaArcadeMapGrid.h"

/**
 * 정렬(grid-aligned) 방 레이아웃 생성 파라미터.
 * 거친 슬롯 격자 위에서 중앙부터 클러스터로 방을 키움 → 빈 슬롯은 void.
 */
struct FSpartaArcadeRoomGenParams
{
    int32 Width = 100;
    int32 Height = 100;
    int32 SectorCols = 8;            // 가로 슬롯 수
    int32 SectorRows = 8;            // 세로 슬롯 수
    int32 Gap = 1;                   // 슬롯(방) 사이 간격 = 벽/문 두께(칸)
    int32 VoidSlots = 10;            // 제거할 작은 방 최대 개수(클수록 덜 참, 연결 보장 한도까지)
    float MergeChance = 0.75f;       // 인접 슬롯을 직사각형/ㄴ자 방으로 합칠 확률
    int32 CenterSlots = 2;           // 중앙 정사각형 크기(슬롯 단위). 2 = 2x2
    float ExtraOpenChance = 0.18f;   // 스패닝 트리 외 추가 통로(순환로) 확률. 높이면 덜 미로 같아짐
    float BreakableWallChance = 0.3f;// 통로 없는 경계가 부술 수 있는 벽이 될 확률(폭탄 지름길)

    // ---- 방 내부 채우기(기둥/박스) ----
    int32 InteriorBlock = 5;         // 구역(스타일) 블록 한 변 크기(칸). 클수록 큼직하게 정돈
    float BoxDensity = 0.4f;         // 기둥 인접 칸이 부술 수 있는 박스가 될 확률
    int32 DoorClearRadius = 1;       // 열린 문 주변 비울 반경(입구 막힘 방지)
    float EmptyStyleWeight = 0.30f;  // 구역이 '빈 곳'일 비율
    float RegularStyleWeight = 0.40f;// 구역이 '규칙 격자'일 비율(나머지는 '어질러짐')
    float MessyPillarChance = 0.16f; // 어질러짐 구역의 랜덤 기둥 확률
    float EmptyBoxChance = 0.05f;    // 빈 곳 구역의 드문 박스 확률

    // ---- 스폰 + 안전구역 ----
    int32 SafeRadius = 3;            // 스폰 주변 비울 사각 반경(넉넉할수록 큰 시작 공간)

    // ---- 변형 타일(지형 효과) ----
    float VariantCoverage = 0.25f;   // 바닥(기둥·박스 뺀 빈 칸) 중 변형 타일 비율
    bool  bVariantsInCenter = false; // 중앙 아레나에도 변형 타일 배치할지(기본 제외)

    // ---- 이동 장애물 스폰(액터는 별도, 여기선 위치만 추출) ----
    int32 NumObstacles = 10;         // 맵당 이동 장애물 수(스폰 위치 개수)
};

/**
 * 정렬 방 레이아웃 생성기.
 *   1) 중앙 정사각형 고정 → 인접 슬롯으로 클러스터 성장(연결 보장), 빈 슬롯은 void
 *   2) 슬롯/병합 갭을 바닥으로, 인접 방 경계는 "한 칸 열고 나머지 박스벽"
 *   3) 바닥에 면한 void는 고정벽으로 폐합
 * 같은 Seed = 같은 맵. (방 내부 채우기[박스·타일·장애물]는 다음 단계)
 */
struct FSpartaArcadeRoomGenerator
{
    static void Generate(FSpartaArcadeMapGrid& OutGrid, int32 Seed, const FSpartaArcadeRoomGenParams& Params,
        TArray<FIntPoint>* OutSpawns = nullptr,
        TArray<FIntPoint>* OutObstacleSpawns = nullptr,
        FIntPoint* OutCenterMin = nullptr, FIntPoint* OutCenterMax = nullptr);

    /** (StartX,StartY)에서 벽이 아닌 칸 도달 수(flood-fill). 막힘=FixedWall/Void, 통과=Empty/Box. 고립 검증용. */
    static int32 CountReachableNonWall(const FSpartaArcadeMapGrid& Grid, int32 StartX, int32 StartY);
};