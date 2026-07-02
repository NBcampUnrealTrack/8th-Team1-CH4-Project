#pragma once

#include "CoreMinimal.h"
#include "SpartaArcadeMapTypes.generated.h"

/** 맵 그리드 한 칸이 가질 수 있는 모든 타일 종류들. */
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



/** 방 모양. 1단계 방 레이아웃에서 쓸 예정 */
UENUM(BlueprintType)
enum class ESpartaArcadeRoomShape : uint8
{
    Square  UMETA(DisplayName = "Square"),
    LShape  UMETA(DisplayName = "L-Shape"),
    Rect    UMETA(DisplayName = "Rectangle")
};
