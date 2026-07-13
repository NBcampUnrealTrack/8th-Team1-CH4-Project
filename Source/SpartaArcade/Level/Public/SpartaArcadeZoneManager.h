#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpartaArcadeZoneManager.generated.h"

class ASpartaArcadeMapBuilder;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UDataTable;

// 압사당한 액터(플레이어 등) 알림 — 실제 사망 처리는 게임 시스템이 바인딩.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorCrushed, AActor*, CrushedActor);

/**
 * 서든데스 자기장(압사 블록) 매니저.
 *   · 외곽부터 시계방향 나선으로 칸을 압사 → 영구 벽(즉시 나타남). 최종 지대 = 중앙 아레나.
 *   · 붉은 경고 타일(1.5초) → 붉은 압사 블록 낙하. 낙하 칸 위 액터는 압사 트리거.
 *   · 진행도(float 하나)만 복제 → 서버·클라가 각자 나선을 로컬 렌더(맵 렌더링과 동일 패턴).
 *   · 압사 즉사 자체는 트리거만 제공(OnActorCrushed), 사망 판정은 게임 시스템. 이동 장애물은 소멸.
 */
UCLASS()
class SPARTAARCADE_API ASpartaArcadeZoneManager : public AActor
{
    GENERATED_BODY()

public:
    ASpartaArcadeZoneManager();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** 그리드 조회용 맵빌더 지정. 미지정 시 자동 탐색. */
    UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Zone")
    void SetMapBuilder(ASpartaArcadeMapBuilder* InMap);

    /** 자기장 카운트다운 시작(리셋). GameMode가 매치 시작 시 호출. 미호출 시 BeginPlay부터 자동. */
    UFUNCTION(BlueprintCallable, Category = "SpartaArcade|Zone")
    void StartCountdown();

    /** 압사당한 액터 알림(플레이어=사망 처리는 게임 시스템, 장애물=자체 소멸). */
    UPROPERTY(BlueprintAssignable, Category = "SpartaArcade|Zone")
    FOnActorCrushed OnActorCrushed;

    // ---- 타이밍(초) — 전부 조절 가능. 테스트 시 ActivationDelay를 작게. ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpartaArcade|Zone", meta = (ClampMin = "0.0"))
    float ActivationDelay = 90.f;   // 매치 시작 ~ 자기장 발동(1:30)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpartaArcade|Zone", meta = (ClampMin = "1.0"))
    float ShrinkDuration = 210.f;   // 발동 ~ 종료(외곽→중앙, 총 ~5분)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpartaArcade|Zone", meta = (ClampMin = "0.0"))
    float WarningLead = 1.5f;       // 붉은 경고 ~ 낙하까지 리드타임

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpartaArcade|Zone", meta = (ClampMin = "0.1"))
    float BlockHeightTiles = 2.f;   // 압사 블록 높이(타일 단위). 2 = 2칸 높이

    // ---- 밸런싱 DataTable ----
    /** 자기장 수치 DT. 비우면 위 값 사용. Row 구조: FSpartaArcadeZoneRow */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Zone")
    UDataTable* ZoneTable = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Zone")
    FName ZoneRowName = TEXT("Default");

    /** DT Row → 수치 복사(BeginPlay에서 자동 호출, 서버·클라 공통). */
    void ApplyZoneRow();

    // ---- 연출 색(붉은 톤) ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Zone")
    FLinearColor CrushColor = FLinearColor(0.45f, 0.05f, 0.05f);    // 낙하 완료 블록(어두운 적)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Zone")
    FLinearColor WarningColor = FLinearColor(1.0f, 0.22f, 0.16f);   // 경고 타일(밝은 적)

    /** 색칠용 언릿 머티리얼(맵빌더와 동일한 M_Blockout 지정 — "Used with ISM" 필수). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|Zone")
    UMaterialInterface* BlockoutMaterial = nullptr;

protected:
    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Zone")
    UHierarchicalInstancedStaticMeshComponent* CrushISM;    // 낙하 블록(입체 큐브)

    UPROPERTY(VisibleAnywhere, Category = "SpartaArcade|Zone")
    UHierarchicalInstancedStaticMeshComponent* WarningISM;  // 경고 타일(납작 플레인)

    UPROPERTY(ReplicatedUsing = OnRep_Progress)
    float ShrinkProgress = 0.f;     // 0..1, 서버가 갱신 → 클라 복제

    UFUNCTION()
    void OnRep_Progress();

    UPROPERTY()
    TWeakObjectPtr<ASpartaArcadeMapBuilder> Map;

    TArray<FIntPoint> SpiralCells;  // 외곽→중앙 나선(중앙 아레나 제외)
    bool bSpiralBuilt = false;
    bool bStarted = false;
    float Elapsed = 0.f;
    int32 RenderedCrush = 0;        // 지금까지 렌더한 낙하 블록 수
    int32 KillIndex = 0;            // 지금까지 압사 처리한 인덱스(서버)

    void TryBuildSpiral();
    void RefreshVisuals();
    void ProcessCrushKills();
    void ApplyColor(UHierarchicalInstancedStaticMeshComponent* Comp, const FLinearColor& Col);
};