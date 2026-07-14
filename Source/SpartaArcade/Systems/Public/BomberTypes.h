#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BomberTypes.generated.h"

// 캐릭터 타입 
UENUM(BlueprintType)
enum class ECharacterType : uint8
{
	Default,    // 기준형
	Explosion,  // 화력형
	Speed,      // 속도형
	BombCount   // 폭탄형
};

// 스탯 종류 
UENUM(BlueprintType)
enum class EBomberStatType : uint8
{
	BombRange,
	MoveSpeed,
	BombCount
};

// 캐릭터 스탯 DataTable Row 
USTRUCT(BlueprintType)
struct FCharacterStatRow : public FTableRowBase
{
	GENERATED_BODY()

	// 캐릭터 타입
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECharacterType CharacterType = ECharacterType::Default;

	// 폭발 범위 시작값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 StartBombRange = 1;

	// 보유 폭탄 수 시작값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 StartBombCount = 1;

	// 이동 속도 시작값 (단계)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 StartMoveSpeed = 1;

	// 스탯 상한값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxBombRange = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxBombCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxMoveSpeed = 5;
};

// 아이템 종류 
UENUM(BlueprintType)
enum class EBomberItemType : uint8
{
    None,       // 아이템 없음 (빈 상자)
    BombRange,  // 폭발 범위 +1
    MoveSpeed,  // 이동 속도 +1
    BombCount,  // 폭탄 수 +1
    MedKit,     // 구급상자
    Shield,      // 방어막
	KickUnlock   // 발차기 기능
};

// 아이템 DataTable
USTRUCT(BlueprintType)
struct FItemDataRow : public FTableRowBase
{
	GENERATED_BODY()

	// 아이템 종류
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EBomberItemType ItemType = EBomberItemType::None;

	// 스탯 적용량
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 StatValue = 1;

	// 드롭 가중치
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 DropWeight = 25;
};

// 폭탄 DataTable 
USTRUCT(BlueprintType)
struct FBombStatRow : public FTableRowBase
{
	GENERATED_BODY()

	// 폭발까지 걸리는 시간 
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float FuseTime = 3.f;

	// 폭발 범위 상한 
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxExplosionRange = 5;

	// 연쇄 폭발 가능 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bChainEnabled = true;

	// 타일 한 칸의 크기 
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TileSize = 100.f;
};

// 플레이어 상태
UENUM(BlueprintType)
enum class EBomberPlayerState : uint8
{
	Alive,
	Stunned,
	Eliminated
};

//전투 DataTable 
USTRUCT(BlueprintType)
struct FCombatStatRow : public FTableRowBase
{
	GENERATED_BODY()

	// 시작 하트 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 StartHearts = 3;

	// 기절 지속 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float StunDuration = 3.f;

	// 구조 후 무적 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float InvincibleDuration = 1.f;

	// 자력회복 시 부활 하트 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SelfReviveHearts = 1;
};