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

    if (UCanvasPanelSlot* MySlot = Cast<UCanvasPanelSlot>(Slot))
    {
        MySlot->SetSize(MinimapCanvasSize);
    }

    UWidgetTree* Tree = WidgetTree;
    if (Tree)
    {
        // 1. 최상위 Canvas Panel 생성
        if (!MinimapCanvas)
        {
            MinimapCanvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MinimapCanvas"));
            Tree->RootWidget = MinimapCanvas;
        }

        // 미니맵 영역 바깥으로 배경 지도나 요소들이 삐져나가지 않도록 경계 클리핑(ClipToBounds) 강제 활성화
        if (MinimapCanvas)
        {
            MinimapCanvas->SetClipping(EWidgetClipping::ClipToBounds);
        }

        // 2. 미니맵 배경 이미지 생성 및 레이아웃
        if (MinimapCanvas && !MinimapBackground)
        {
            MinimapBackground = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MinimapBackground"));
            UCanvasPanelSlot* BGSlot = MinimapCanvas->AddChildToCanvas(MinimapBackground);
            if (BGSlot)
            {
                BGSlot->SetAnchors(FAnchors(0.5f, 0.5f));
                BGSlot->SetAlignment(FVector2D(0.5f, 0.5f));
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
    MapWorldSize = WorldSize;
}

FVector2D USpartaMinimapWidget::WorldToMinimapPosition(const FVector& WorldLocation, const FVector& PlayerLocation) const
{
    if (MinimapViewRadius <= 0.0f)
    {
        return FVector2D::ZeroVector;
    }

    // 플레이어 캐릭터 기준의 상대 위치 계산
    FVector RelativeWorld = WorldLocation - PlayerLocation;

    // Unreal X = UI Y (역방향), Unreal Y = UI X (정방향)
    // 뷰 반경 비율에 따른 UI 픽셀 스케일링
    float RelUI_X = (RelativeWorld.Y / MinimapViewRadius) * (MinimapCanvasSize.X * 0.5f);
    float RelUI_Y = (-RelativeWorld.X / MinimapViewRadius) * (MinimapCanvasSize.Y * 0.5f);

    // 렌더 타겟 방식에서는 중앙 앵커(0.5, 0.5) 기준 상대 오프셋만 반환하므로 중앙 오프셋 더하기 제거
    return FVector2D(RelUI_X, RelUI_Y);
}

void USpartaMinimapWidget::UpdateMarkerPositions()
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerPawn)
    {
        return;
    }

    FVector PlayerLocation = PlayerPawn->GetActorLocation();

    // 1. 맵 배경 이미지는 캔버스 정중앙에 크기를 딱 맞춰 고정 (카메라가 촬영한 렌더 타겟이므로 시프트 불필요)
    if (MinimapBackground)
    {
        UCanvasPanelSlot* BGSlot = Cast<UCanvasPanelSlot>(MinimapBackground->Slot);
        if (BGSlot)
        {
            BGSlot->SetAnchors(FAnchors(0.5f, 0.5f));
            BGSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            BGSlot->SetSize(MinimapCanvasSize); // 캔버스 크기(250x250)에 딱 맞게 고정
            BGSlot->SetPosition(FVector2D::ZeroVector); // 중앙 고정
        }
    }

    // 2. 플레이어 본인 마커는 미니맵 중앙에 고정 (카메라가 플레이어를 중앙에 두고 비춤)
    if (PlayerMarker)
    {
        UCanvasPanelSlot* PlayerSlot = Cast<UCanvasPanelSlot>(PlayerMarker->Slot);
        if (PlayerSlot)
        {
            PlayerSlot->SetAnchors(FAnchors(0.5f, 0.5f));
            PlayerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            PlayerSlot->SetPosition(FVector2D::ZeroVector); // 중앙 고정
        }

        // 캐릭터의 시선(회전각) 반영
        // 배경 지도가 고정된 상태이므로 플레이어 마커만 캐릭터 시선(Yaw)을 반영하여 회전
        float PlayerYaw = PlayerPawn->GetActorRotation().Yaw;
        PlayerMarker->SetRenderTransformAngle(PlayerYaw + 90.0f);
    }

    // 3. 자기장 안전 구역 상대 위치 갱신
    if (SafeZoneIndicator)
    {
        UCanvasPanelSlot* SafeZoneSlot = Cast<UCanvasPanelSlot>(SafeZoneIndicator->Slot);
        if (SafeZoneSlot)
        {
            SafeZoneSlot->SetAnchors(FAnchors(0.5f, 0.5f));
            SafeZoneSlot->SetAlignment(FVector2D(0.5f, 0.5f));

            // 플레이어 대비 상대 픽셀 오프셋을 계산하여 인디케이터 위치로 설정 (중앙 앵커 기준)
            FVector2D SafeZoneMinimapPos = WorldToMinimapPosition(CachedSafeZoneCenter, PlayerLocation);
            SafeZoneSlot->SetPosition(SafeZoneMinimapPos);

            // 미니맵 뷰 반경에 따른 자기장 크기 스케일링
            float MinimapRadiusSizeX = (CachedSafeZoneRadius * 2.0f / MinimapViewRadius) * MinimapCanvasSize.X;
            float MinimapRadiusSizeY = (CachedSafeZoneRadius * 2.0f / MinimapViewRadius) * MinimapCanvasSize.Y;
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

    // MinimapBackground 슬레이트 리소스 해제 복구
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
