#include "SpartaMinimapWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Level/Public/SpartaArcadeMapBuilder.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"

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
        
        // 3. 플레이어 위치 마커 생성 및 레이아웃
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
        }
    }
}

void USpartaMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 동적 미니맵 렌더타겟 및 머터리얼 인스턴스 초기화 수행
    InitializeDynamicRenderTarget();

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
    }

    // 매 프레임 플레이어 마커 위치 동기화
    UpdateMarkerPositions();
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
    // 로컬 플레이어별 개별 미니맵이 자신을 기준으로 표시되도록 GetOwningPlayerPawn() 기반으로 수정
    APawn* PlayerPawn = GetOwningPlayerPawn();
    if (!PlayerPawn)
    {
        if (APlayerController* PC = GetOwningPlayer())
        {
            PlayerPawn = PC->GetPawn();
        }
    }

    if (!PlayerPawn)
    {
        PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    }

    if (!PlayerPawn)
    {
        return;
    }

    FVector PlayerLocation = PlayerPawn->GetActorLocation();

    // 미니맵 카메라가 각 로컬 캐릭터의 머리 위에 오도록 씬 캡처 위치를 매 틱 강제 이동
    USceneCaptureComponent2D* SceneCapture = PlayerPawn->FindComponentByClass<USceneCaptureComponent2D>();
    if (!SceneCapture)
    {
        TArray<USceneCaptureComponent2D*> Captures;
        PlayerPawn->GetComponents<USceneCaptureComponent2D>(Captures);
        if (Captures.Num() > 0)
        {
            SceneCapture = Captures[0];
        }
    }

    if (SceneCapture)
    {
        FVector NewCaptureLoc = FVector(PlayerLocation.X, PlayerLocation.Y, PlayerLocation.Z + CachedCameraRelativeZ);
        FRotator NewCaptureRot = FRotator(-90.f, 0.f, 0.f); // 수직 하방 촬영
        SceneCapture->SetWorldLocationAndRotation(NewCaptureLoc, NewCaptureRot);
    }

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
        PlayerMarker->SetRenderTransformAngle(PlayerYaw);
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
}

// 각 플레이어별 독립된 미니맵 렌더타겟 및 직접 텍스처 브러시 바인딩을 위한 함수 구현
void USpartaMinimapWidget::InitializeDynamicRenderTarget()
{
    if (bInitializedDynamicRT) return;

    APawn* PlayerPawn = GetOwningPlayerPawn();
    if (!PlayerPawn)
    {
        if (APlayerController* PC = GetOwningPlayer())
        {
            PlayerPawn = PC->GetPawn();
        }
    }
    if (!PlayerPawn) return;

    // 캐릭터에서 SceneCaptureComponent2D 찾기
    USceneCaptureComponent2D* SceneCapture = PlayerPawn->FindComponentByClass<USceneCaptureComponent2D>();
    if (!SceneCapture)
    {
        TArray<USceneCaptureComponent2D*> Captures;
        PlayerPawn->GetComponents<USceneCaptureComponent2D>(Captures);
        if (Captures.Num() > 0)
        {
            SceneCapture = Captures[0];
        }
    }

    if (SceneCapture && MinimapBackground)
    {
        // 1. 동적 렌더 타겟 생성 (256x256, RGBA8 포맷)
        DynamicRenderTarget = NewObject<UTextureRenderTarget2D>(this);
        if (DynamicRenderTarget)
        {
            DynamicRenderTarget->InitAutoFormat(256, 256);
            DynamicRenderTarget->UpdateResourceImmediate(true);
            
            // SceneCapture의 타겟 렌더타겟을 동적으로 생성한 RT로 변경
            SceneCapture->TextureTarget = DynamicRenderTarget;
            
            //미니맵 카메라 갱신 누락 방지를 위해 매 프레임 캡처 및 움직임 기반 캡처를 강제 활성화하고 즉시 1회 캡처
            SceneCapture->bCaptureEveryFrame = true;
            SceneCapture->bCaptureOnMovement = true;
            SceneCapture->CaptureScene();
            
            //씬 캡쳐 카메라의 상대 높이를 가져와 캐싱
            CachedCameraRelativeZ = SceneCapture->GetRelativeLocation().Z;
            if (CachedCameraRelativeZ < 100.f)
            {
                CachedCameraRelativeZ = 1500.f; // 높이가 비정상적인 경우 디폴트 1500 설정
            }

            // 미니맵 UI 머티리얼(MinimapMaterialClass)이 지정된 경우 동적 인스턴스를 생성해 렌더타겟 텍스처를 바인딩
            if (MinimapMaterialClass)
            {
                DynamicMinimapMaterial = UMaterialInstanceDynamic::Create(MinimapMaterialClass, this);
                if (DynamicMinimapMaterial)
                {
                    DynamicMinimapMaterial->SetTextureParameterValue(TEXT("RT_Minimap"), DynamicRenderTarget);
                    MinimapBackground->SetBrushFromMaterial(DynamicMinimapMaterial);
                }
            }
            else
            {
                MinimapBackground->SetBrushResourceObject(DynamicRenderTarget);
            }
            
            UE_LOG(LogTemp, Warning, TEXT("[MinimapSystem] 플레이어 %s 를 위한 동적 미니맵 렌더타겟 및 머터리얼 인스턴스 연동이 완료되었습니다."), *PlayerPawn->GetName());
            bInitializedDynamicRT = true;
        }
    }
}
