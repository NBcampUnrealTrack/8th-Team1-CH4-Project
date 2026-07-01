#pragma once

#include "CoreMinimal.h"
#include "SpartaUIDefs.generated.h"

UENUM(BlueprintType)
enum class EDeathReason : uint8
{
    Explosion   UMETA(DisplayName = "폭발"),
    SafeZone    UMETA(DisplayName = "압사 블록"),
    Obstacle    UMETA(DisplayName = "장애물"),
    Unknown     UMETA(DisplayName = "Unknown")
};

UENUM(BlueprintType)
enum class EMatchResult : uint8
{
    Victory     UMETA(DisplayName = "승리!"),
    Defeat      UMETA(DisplayName = "패배.."),
    Draw        UMETA(DisplayName = "무승부")
};

UENUM(BlueprintType)
enum class ECharacterType : uint8
{
    CharacterA  UMETA(DisplayName = "폭발형"),
    CharacterB  UMETA(DisplayName = "속도형"),
    CharacterC  UMETA(DisplayName = "폭탄갯수형")
};

UENUM(BlueprintType)
enum class EItemType : uint8
{
    BombCount   UMETA(DisplayName = "폭탄 갯수 증가"),
    ExplosionRange UMETA(DisplayName = "폭발 범위 증가"),
    MoveSpeed   UMETA(DisplayName = "이동 속도 증가"),
    Shield      UMETA(DisplayName = "방어막")
};

USTRUCT(BlueprintType)
struct FMatchPlayerResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    FString PlayerName;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    int32 Rank;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    int32 SurvivalTime;
};

// Modified: UI 레이어 구조를 명확히 구분하기 위한 EUILayer 열거형 추가
UENUM(BlueprintType)
enum class EUILayer : uint8
{
    GameHUD        UMETA(DisplayName = "Game HUD"),
    MenuScreen     UMETA(DisplayName = "Menu Screen"),
    Popup          UMETA(DisplayName = "Popup"),
    SystemOverlay  UMETA(DisplayName = "System Overlay")
};

