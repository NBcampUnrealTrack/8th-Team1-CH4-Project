#include "SpartaMinimapWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

void USpartaMinimapWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void USpartaMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 매 프레임 플레이어 마커 및 자기장 표시 위치 동기화
    UpdateMarkerPositions();
}

void USpartaMinimapWidget::UpdateSafeZone(const FVector& ZoneCenter, float ZoneRadius)
{
    CachedSafeZoneCenter = ZoneCenter;
    CachedSafeZoneRadius = ZoneRadius;
}

void USpartaMinimapWidget::SetupMapStructure(UTexture2D* MapTexture, float WorldSize)
{
    if (MinimapBackground && MapTexture)
    {
        MinimapBackground->SetBrushFromTexture(MapTexture);
    }
    MapWorldSize = WorldSize;
}

FVector2D USpartaMinimapWidget::WorldToMinimapPosition(const FVector& WorldLocation) const
{
    if (MapWorldSize <= 0.0f)
    {
        return FVector2D::ZeroVector;
    }

    // 맵 월드 중심이 (0, 0)이라 가정하고 좌표 스케일링 수행
    // Unreal X = UI Y (역방향), Unreal Y = UI X (정방향)
    float NormalizedX = (WorldLocation.Y / MapWorldSize) + 0.5f;
    float NormalizedY = (-WorldLocation.X / MapWorldSize) + 0.5f;

    // 클램핑하여 미니맵 영역 밖으로 너무 벗어나지 않도록 처리
    NormalizedX = FMath::Clamp(NormalizedX, 0.0f, 1.0f);
    NormalizedY = FMath::Clamp(NormalizedY, 0.0f, 1.0f);

    return FVector2D(NormalizedX * MinimapCanvasSize.X, NormalizedY * MinimapCanvasSize.Y);
}

void USpartaMinimapWidget::UpdateMarkerPositions()
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerPawn)
    {
        return;
    }

    // 1. 본인 플레이어 마커 업데이트
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    FVector2D MinimapPos = WorldToMinimapPosition(PlayerLocation);

    if (PlayerMarker)
    {
        UCanvasPanelSlot* PlayerSlot = Cast<UCanvasPanelSlot>(PlayerMarker->Slot);
        if (PlayerSlot)
        {
            PlayerSlot->SetPosition(MinimapPos);
        }

        // 캐릭터가 바라보는 회전각 투영 (Yaw)
        float PlayerYaw = PlayerPawn->GetActorRotation().Yaw;
        // Unreal Yaw -> UI 2D 회전 보정 (보통 Yaw - 90 또는 90 - Yaw 방향)
        PlayerMarker->SetRenderTransformAngle(PlayerYaw + 90.0f);
    }

    // 2. 자기장 인디케이터 갱신
    if (SafeZoneIndicator)
    {
        UCanvasPanelSlot* SafeZoneSlot = Cast<UCanvasPanelSlot>(SafeZoneIndicator->Slot);
        if (SafeZoneSlot)
        {
            FVector2D SafeZoneMinimapPos = WorldToMinimapPosition(CachedSafeZoneCenter);
            SafeZoneSlot->SetPosition(SafeZoneMinimapPos);

            // 반지름 크기를 미니맵 스케일에 대응하여 조절
            // UI DesiredSize = (SafeZoneRadius * 2 / MapWorldSize) * MinimapCanvasSize
            float MinimapRadiusSizeX = (CachedSafeZoneRadius * 2.0f / MapWorldSize) * MinimapCanvasSize.X;
            float MinimapRadiusSizeY = (CachedSafeZoneRadius * 2.0f / MapWorldSize) * MinimapCanvasSize.Y;
            SafeZoneSlot->SetSize(FVector2D(MinimapRadiusSizeX, MinimapRadiusSizeY));
        }
    }
}
