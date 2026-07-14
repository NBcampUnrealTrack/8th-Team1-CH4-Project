#include "SpartaMinimapWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Level/Public/SpartaArcadeMapBuilder.h"
#include "Level/Public/SpartaArcadeZoneManager.h"
#include "Blueprint/WidgetTree.h"

void USpartaMinimapWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // UMG 에디터 디자인에 설정이 누락되어도 C++ 코드에서 앵커, 피벗, 레이아웃을 설정 규칙에 맞춰 자동 동적 조립
    UWidgetTree* Tree = WidgetTree;
    if (Tree)
    {
        // 1. 최상위 Canvas Panel 생성
        if (!MinimapCanvas)
        {
            MinimapCanvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MinimapCanvas"));
            Tree->RootWidget = MinimapCanvas;
        }

        // 2. 미니맵 배경 이미지 생성 및 레이아웃
        if (MinimapCanvas && !MinimapBackground)
        {
            MinimapBackground = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MinimapBackground"));
            UCanvasPanelSlot* BGSlot = MinimapCanvas->AddChildToCanvas(MinimapBackground);
            if (BGSlot)
            {
                BGSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f)); // 꽉 차게 조립
                BGSlot->SetOffsets(FMargin(0.f));
                BGSlot->SetZOrder(0);
            }
            MinimapBackground->SetColorAndOpacity(FLinearColor(0.05f, 0.05f, 0.05f, 0.7f)); // 반투명 검은색 폴백 배경
        }

        // 3. 세이프존 안내선(원) 생성 및 레이아웃
        if (MinimapCanvas && !SafeZoneIndicator)
        {
            SafeZoneIndicator = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SafeZoneIndicator"));
            UCanvasPanelSlot* ZoneSlot = MinimapCanvas->AddChildToCanvas(SafeZoneIndicator);
            if (ZoneSlot)
            {
                ZoneSlot->SetAnchors(FAnchors(0.5f, 0.5f)); // 중앙 앵커
                ZoneSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // 중앙 피벗
                ZoneSlot->SetPosition(FVector2D::ZeroVector);
                ZoneSlot->SetZOrder(1);
            }
            
            // 엔진 기본 원 텍스처 또는 흰색 평면에 적색 곱하기
            SafeZoneIndicator->SetColorAndOpacity(FLinearColor(1.f, 0.1f, 0.1f, 0.8f));
        }

        // 4. 플레이어 위치 마커 생성 및 레이아웃
        if (MinimapCanvas && !PlayerMarker)
        {
            PlayerMarker = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PlayerMarker"));
            UCanvasPanelSlot* MarkerSlot = MinimapCanvas->AddChildToCanvas(PlayerMarker);
            if (MarkerSlot)
            {
                MarkerSlot->SetAnchors(FAnchors(0.5f, 0.5f)); // 중앙 앵커
                MarkerSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // 중앙 피벗
                MarkerSlot->SetSize(FVector2D(14.f, 14.f));
                MarkerSlot->SetZOrder(2);
            }
            PlayerMarker->SetColorAndOpacity(FLinearColor(1.f, 0.9f, 0.1f, 1.f)); // 노란색 마커 지정
        }
    }
}

void USpartaMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    UWorld* World = GetWorld();
    if (World)
    {
        // 1. 맵 빌더를 통한 동적 맵 스케일 수립
        static TWeakObjectPtr<ASpartaArcadeMapBuilder> CachedMapBuilder = nullptr;
        if (!CachedMapBuilder.IsValid())
        {
            CachedMapBuilder = Cast<ASpartaArcadeMapBuilder>(UGameplayStatics::GetActorOfClass(World, ASpartaArcadeMapBuilder::StaticClass()));
        }

        FVector MapCenter = FVector::ZeroVector;
        if (CachedMapBuilder.IsValid())
        {
            float TileSize = CachedMapBuilder->GetTileSize();
            float GridWidth = CachedMapBuilder->GetGridWidth();
            float GridHeight = CachedMapBuilder->GetGridHeight();
            MapWorldSize = FMath::Max(GridWidth, GridHeight) * TileSize;
            MapCenter = CachedMapBuilder->GetActorLocation();
        }

        // 2. 존 매니저를 통한 실시간 안전 구역 반경 동기화
        static TWeakObjectPtr<ASpartaArcadeZoneManager> CachedZoneManager = nullptr;
        if (!CachedZoneManager.IsValid())
        {
            CachedZoneManager = Cast<ASpartaArcadeZoneManager>(UGameplayStatics::GetActorOfClass(World, ASpartaArcadeZoneManager::StaticClass()));
        }

        if (CachedZoneManager.IsValid())
        {
            float ShrinkProgress = CachedZoneManager->GetShrinkProgress();
            
            float MaxRadius = MapWorldSize * 0.5f;
            float MinRadius = (CachedMapBuilder.IsValid()) ? (CachedMapBuilder->GetTileSize() * 5.0f) : 500.f;
            float CurrentSafeRadius = FMath::Lerp(MaxRadius, MinRadius, ShrinkProgress);

            UpdateSafeZone(MapCenter, CurrentSafeRadius);
        }
    }

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

// UWidget 및 USlot의 ReleaseSlateResources()를 명시적으로 호출하여 Slate 리소스 메모리 누수 해결
void USpartaMinimapWidget::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);

    if (MinimapCanvas)
    {
        // 캔버스 하위 자식 위젯들과 그 슬롯들의 Slate 리소스 명시적 해제
        const int32 ChildCount = MinimapCanvas->GetChildrenCount();
        for (int32 i = 0; i < ChildCount; ++i)
        {
            if (UWidget* Child = MinimapCanvas->GetChildAt(i))
            {
                if (Child->Slot)
                {
                    Child->Slot->ReleaseSlateResources(bReleaseChildren);
                }
                Child->ReleaseSlateResources(bReleaseChildren);
            }
        }

        if (MinimapCanvas->Slot)
        {
            MinimapCanvas->Slot->ReleaseSlateResources(bReleaseChildren);
        }
        MinimapCanvas->ReleaseSlateResources(bReleaseChildren);
    }

    if (MinimapBackground)
    {
        if (MinimapBackground->Slot)
        {
            MinimapBackground->Slot->ReleaseSlateResources(bReleaseChildren);
        }
        MinimapBackground->ReleaseSlateResources(bReleaseChildren);
    }

    if (PlayerMarker)
    {
        if (PlayerMarker->Slot)
        {
            PlayerMarker->Slot->ReleaseSlateResources(bReleaseChildren);
        }
        PlayerMarker->ReleaseSlateResources(bReleaseChildren);
    }

    if (SafeZoneIndicator)
    {
        if (SafeZoneIndicator->Slot)
        {
            SafeZoneIndicator->Slot->ReleaseSlateResources(bReleaseChildren);
        }
        SafeZoneIndicator->ReleaseSlateResources(bReleaseChildren);
    }
}
