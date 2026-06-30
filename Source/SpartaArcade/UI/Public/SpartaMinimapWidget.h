#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpartaMinimapWidget.generated.h"

class UCanvasPanel;
class UImage;

UCLASS()
class SPARTAARCADE_API USpartaMinimapWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // --- UMG 위젯 바인딩 ---
    UPROPERTY(meta = (BindWidget))
    UCanvasPanel* MinimapCanvas;

    UPROPERTY(meta = (BindWidget))
    UImage* MinimapBackground;

    UPROPERTY(meta = (BindWidget))
    UImage* PlayerMarker;

    UPROPERTY(meta = (BindWidget))
    UImage* SafeZoneIndicator;

    // --- 미니맵 설정 값 (임시 수치) ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MinimapSettings")
    float MapWorldSize = 10000.0f; // 실제 월드의 가로/세로 크기 (cm)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MinimapSettings")
    FVector2D MinimapCanvasSize = FVector2D(250.0f, 250.0f); // UI 상의 캔버스 크기 (DP)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MinimapSettings")
    bool bShowTeammates = true;

public:
    // --- 외부 레벨/게임 시스템으로부터 데이터 업데이트 수신 ---
    UFUNCTION(BlueprintCallable, Category = "UI | Minimap")
    void UpdateSafeZone(const FVector& ZoneCenter, float ZoneRadius);

    UFUNCTION(BlueprintCallable, Category = "UI | Minimap")
    void SetupMapStructure(UTexture2D* MapTexture, float WorldSize);

private:
    // 월드 좌표를 미니맵 2D UI 캔버스 좌표로 변환
    FVector2D WorldToMinimapPosition(const FVector& WorldLocation) const;

    // 자기장 및 마커들의 UI 렌더 위치 업데이트
    void UpdateMarkerPositions();

    FVector CachedSafeZoneCenter = FVector::ZeroVector;
    float CachedSafeZoneRadius = 0.0f;
};
