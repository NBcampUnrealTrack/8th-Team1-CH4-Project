#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "SpartaArcadeMapTypes.h"
#include "SpartaArcadeMapGrid.generated.h"

/**
 * 맵의 그리드.
 *
 * 맵을 타일 종류의 표 하나로 
 *   - 생성: 표를 채운다
 *   - 빌더: 표를 읽어서 바닥/벽/박스 액터를 깐다
 *   - 나중에: 미니맵·타일효과·자기장 수축이 전부 표를 읽거나 고친다
 *
 * 저장 방식: 2차원처럼 보이지만 실제론 1차원 배열(TArray).
 *   칸 (X, Y) → 배열 인덱스 = Y * Width + X  
 */
USTRUCT(BlueprintType)
struct FSpartaArcadeMapGrid
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "SpartaArcade|Map")
    int32 Width = 30;

    UPROPERTY(BlueprintReadOnly, Category = "SpartaArcade|Map")
    int32 Height = 30;

    UPROPERTY(BlueprintReadOnly, Category = "SpartaArcade|Map")
    TArray<ESpartaArcadeTileType> Tiles;

    /** 그리드를 (W x H) 크기로 잡고 전부 한 종류로 채움. */
    void Init(int32 InWidth, int32 InHeight, ESpartaArcadeTileType Fill = ESpartaArcadeTileType::Empty)
    {
        Width  = InWidth;
        Height = InHeight;
        Tiles.Init(Fill, Width * Height);
    }

    FORCEINLINE bool IsInside(int32 X, int32 Y) const
    {
        return X >= 0 && X < Width && Y >= 0 && Y < Height;
    }

    FORCEINLINE int32 IndexOf(int32 X, int32 Y) const
    {
        return Y * Width + X;
    }

    FORCEINLINE ESpartaArcadeTileType GetTile(int32 X, int32 Y) const
    {
        return IsInside(X, Y) ? Tiles[IndexOf(X, Y)] : ESpartaArcadeTileType::FixedWall;
    }

    FORCEINLINE void SetTile(int32 X, int32 Y, ESpartaArcadeTileType Type)
    {
        if (IsInside(X, Y))
        {
            Tiles[IndexOf(X, Y)] = Type;
        }
    }

    /**
     * 0단계(임시) 데이터 생성: 테두리 벽 + 내부 박스 랜덤 + 네 모서리 안전구역.
     *
     * 액터를 만들지 않는 "순수 데이터" 함수. 같은 Seed → 항상 같은 맵.
     */
    static void GenerateSimple(FSpartaArcadeMapGrid& OutGrid, int32 Seed,
                               int32 InWidth = 30, int32 InHeight = 30,
                               float BoxRate = 0.45f, int32 SafeMargin = 3)
    {
        OutGrid.Init(InWidth, InHeight, ESpartaArcadeTileType::Empty);
        FRandomStream Rng(Seed);

        for (int32 Y = 0; Y < InHeight; ++Y)
        {
            for (int32 X = 0; X < InWidth; ++X)
            {
                const bool bBorder =
                    (X == 0 || Y == 0 || X == InWidth - 1 || Y == InHeight - 1);

                if (bBorder)
                {
                    OutGrid.SetTile(X, Y, ESpartaArcadeTileType::FixedWall);
                }
                else if (Rng.FRand() < BoxRate)
                {
                    OutGrid.SetTile(X, Y, ESpartaArcadeTileType::DestructibleBox);
                }
            }
        }

        auto ClearPocket = [&OutGrid](int32 StartX, int32 StartY, int32 Size)
        {
            for (int32 Y = StartY; Y < StartY + Size; ++Y)
            {
                for (int32 X = StartX; X < StartX + Size; ++X)
                {
                    if (OutGrid.GetTile(X, Y) == ESpartaArcadeTileType::DestructibleBox)
                    {
                        OutGrid.SetTile(X, Y, ESpartaArcadeTileType::Empty);
                    }
                }
            }
        };

        ClearPocket(1, 1, SafeMargin);                                                  // 좌상단
        ClearPocket(InWidth - 1 - SafeMargin, 1, SafeMargin);                           // 우상단
        ClearPocket(1, InHeight - 1 - SafeMargin, SafeMargin);                          // 좌하단
        ClearPocket(InWidth - 1 - SafeMargin, InHeight - 1 - SafeMargin, SafeMargin);   // 우하단
    }
};
