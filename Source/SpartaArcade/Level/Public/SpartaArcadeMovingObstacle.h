#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpartaArcadeMovingObstacle.generated.h"

class UDataTable;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpartaArcade|Obstacle")
    float HoverHeight = 40.f;     // 지면에서 뜬 높이(시각)

    // 이동 장애물에 부여할 회전 효과 관련 제어 속성 정의
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpartaArcade|Obstacle")
    bool bOrientRotationToMovement = true; // 진행 방향으로 회전 여부

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpartaArcade|Obstacle")
    bool bUseSelfRotation = false; // 제자리 빙글빙글 자전 효과 여부

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpartaArcade|Obstacle")
    float SelfRotationSpeed = 180.f; // 초당 자전 회전 속도 (Yaw)

    // ---- 밸런싱 DataTable ----
    /** 장애물 수치 DT. 비우면 위 값 사용. Row 구조: FSpartaArcadeObstacleRow */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Obstacle")
    UDataTable* ObstacleTable = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Obstacle")
    FName ObstacleRowName = TEXT("Default");

    /** DT Row → 수치 복사(BeginPlay에서 자동 호출, PlaneZ 계산 전). */
    void ApplyObstacleRow();

protected:
    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Obstacle")
    USphereComponent* Collision;

    // 언리얼 엔진의 상속 컴포넌트 직렬화 버그로 인한 비주얼 미출력 문제를 방어하기 위해 변수명을 ObstacleMesh로 리네임합니다.
    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Obstacle")
    UStaticMeshComponent* ObstacleMesh;

    UPROPERTY()
    TWeakObjectPtr<ASpartaArcadeMapBuilder> Map;

    FVector PrevLocation = FVector::ZeroVector;
    
    FIntPoint CurCell = FIntPoint(0, 0);    // 현재 칸
    FIntPoint TargetCell = FIntPoint(0, 0); // 이동 목표 칸(칸 중심으로 부드럽게)
    FIntPoint Dir = FIntPoint(1, 0);        // 현재 진행 방향(4방)
    bool bHasTarget = false;
    float PlaneZ = 0.f;                     // 이동 평면 높이(스폰 Z + HoverHeight)

    bool CellOpen(const FIntPoint& C) const;
    void ChooseNextTarget();
};