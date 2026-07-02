#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpartaArcadeMovingObstacle.generated.h"

class ASpartaArcadeMapBuilder;
class USphereComponent;

/**
 * 이동 장애물 — 맵을 배회하는 비추적 위협(피해야 하는 존재).
 *   · 자유 이동(칸 중심 사이를 부드럽게), 정면이 막히면 주위 뚫린 방향으로 랜덤 전환
 *   · 파괴 불가 벽·기둥 + 부술 수 있는 박스도 못 지나감(맵 그리드 조회로 판정)
 *   · 지면에서 살짝 떠 있음 → 타일 효과 무시(얼음/물/덤불 영향 안 받음)
 *   · 이동은 서버 권위, 위치는 복제. 접촉 피해·폭탄 파괴 보상은 게임 시스템(이 액터는 이동/충돌 골격만).
 */
UCLASS()
class SPARTAARCADE_API ASpartaArcadeMovingObstacle : public AActor
{
    GENERATED_BODY()

public:
    ASpartaArcadeMovingObstacle();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    /** 그리드 조회용 맵빌더 지정. 미지정 시 BeginPlay에서 자동 탐색. */
    UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Obstacle")
    void SetMapBuilder(ASpartaArcadeMapBuilder* InMap);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpartaArcade|Obstacle", meta = (ClampMin = "0.0"))
    float MoveSpeed = 450.f;      // cm/s, 플레이어보다 빠르게(수치 임시)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpartaArcade|Obstacle", meta = (ClampMin = "0.0"))
    float HoverHeight = 40.f;     // 지면에서 뜬 높이(시각)

protected:
    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Obstacle")
    USphereComponent* Collision;

    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Obstacle")
    UStaticMeshComponent* Mesh;

    UPROPERTY()
    TWeakObjectPtr<ASpartaArcadeMapBuilder> Map;

    FIntPoint CurCell = FIntPoint(0, 0);    // 현재 칸
    FIntPoint TargetCell = FIntPoint(0, 0); // 이동 목표 칸(칸 중심으로 부드럽게)
    FIntPoint Dir = FIntPoint(1, 0);        // 현재 진행 방향(4방)
    bool bHasTarget = false;
    float PlaneZ = 0.f;                     // 이동 평면 높이(스폰 Z + HoverHeight)

    bool CellOpen(const FIntPoint& C) const;
    void ChooseNextTarget();
};
