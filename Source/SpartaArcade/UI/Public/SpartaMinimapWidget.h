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

    // --- UMG 위젯 바인딩 (OptionalWidget = true 지정 및 동적 수립 탑재) ---
    UPROPERTY(meta = (BindWidget, OptionalWidget = true))
    UCanvasPanel* MinimapCanvas;

    UPROPERTY(meta = (BindWidget, OptionalWidget = true))
    UImage* MinimapBackground;

    UPROPERTY(meta = (BindWidget, OptionalWidget = true))
    UImage* PlayerMarker;
	
    // --- 미니맵 설정 값 (임시 수치) ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MinimapSettings")
    float MapWorldSize = 10000.0f; // 실제 월드의 가로/세로 크기 (cm)

    // 캐릭터 주변 반경 제어를 위한 뷰 반경 설정 변수 추가
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MinimapSettings")
    float MinimapViewRadius = 3000.0f; // 캐릭터 주변 표시할 월드 반경 (cm)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MinimapSettings")
    FVector2D MinimapCanvasSize = FVector2D(250.0f, 250.0f); // UI 상의 캔버스 크기 (DP)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MinimapSettings")
    bool bShowTeammates = true;

    // 미니맵 마스킹 및 꾸미기용 머티리얼을 지정하기 위한 변수 추가
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MinimapSettings")
    TObjectPtr<UMaterialInterface> MinimapMaterialClass;

public:
    // --- 외부 레벨/게임 시스템으로부터 데이터 업데이트 수신 ---

    UFUNCTION(BlueprintCallable, Category = "UI | Minimap")
    void SetupMapStructure(UTexture2D* MapTexture, float WorldSize);

    // Slate resource leak 방지를 위해 ReleaseSlateResources 오버라이드 선언 추가
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:

    // 캐릭터 주변 반경을 기준으로 하는 상대 좌표 변환 함수 선언 추가
    FVector2D WorldToMinimapPosition(const FVector& WorldLocation, const FVector& PlayerLocation) const;

    // 마커의 UI 렌더 위치 업데이트
    void UpdateMarkerPositions();
	
	// 각 플레이어별 독립된 미니맵 렌더타겟 및 동적 머터리얼 인스턴스 생성을 위한 변수/함수 추가
	UPROPERTY()
	class UTextureRenderTarget2D* DynamicRenderTarget = nullptr;

	UPROPERTY()
	class UMaterialInstanceDynamic* DynamicMinimapMaterial = nullptr;

	bool bInitializedDynamicRT = false;

	float CachedCameraRelativeZ = 2500.f;

	void InitializeDynamicRenderTarget();
};
