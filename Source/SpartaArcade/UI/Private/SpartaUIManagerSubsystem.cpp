#include "SpartaUIManagerSubsystem.h"
#include "Blueprint/UserWidget.h"

void USpartaUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // 초기 레이어 스택 맵 구성
    LayerStacks.Add(EUILayer::GameHUD, FSpartaWidgetStack());
    LayerStacks.Add(EUILayer::MenuScreen, FSpartaWidgetStack());
    LayerStacks.Add(EUILayer::Popup, FSpartaWidgetStack());
    LayerStacks.Add(EUILayer::SystemOverlay, FSpartaWidgetStack());
}

void USpartaUIManagerSubsystem::Deinitialize()
{
    ClearAllLayers();
    Super::Deinitialize();
}

UUserWidget* USpartaUIManagerSubsystem::PushWidget(EUILayer Layer, TSubclassOf<UUserWidget> WidgetClass)
{
    if (!WidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("WidgetClass is null"));
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("World is null"));
        return nullptr;
    }

    UUserWidget* NewWidget = CreateWidget<UUserWidget>(World, WidgetClass);
    if (!NewWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to CreateWidget"));
        return nullptr;
    }

    // 레이어에 따른 Z-Order 지정 후 뷰포트에 추가
    int32 ZOrder = GetZOrderForLayer(Layer);
    NewWidget->AddToViewport(ZOrder);

    // 해당 레이어 스택에 등록
    FSpartaWidgetStack& Stack = LayerStacks.FindOrAdd(Layer);
    Stack.Widgets.Add(NewWidget);

    UE_LOG(LogTemp, Log, TEXT("Added Widget of class %s to Layer %d (Z-Order: %d)"), 
        *WidgetClass->GetName(), static_cast<int32>(Layer), ZOrder);

    return NewWidget;
}

void USpartaUIManagerSubsystem::PopWidget(EUILayer Layer)
{
    FSpartaWidgetStack* Stack = LayerStacks.Find(Layer);
    if (Stack && Stack->Widgets.Num() > 0)
    {
        UUserWidget* WidgetToRemove = Stack->Widgets.Last();
        if (IsValid(WidgetToRemove))
        {
            WidgetToRemove->RemoveFromParent();
            UE_LOG(LogTemp, Log, TEXT("Removed Widget from Layer %d"), static_cast<int32>(Layer));
        }
        Stack->Widgets.RemoveAt(Stack->Widgets.Num() - 1);
    }
}

void USpartaUIManagerSubsystem::ClearLayer(EUILayer Layer)
{
    FSpartaWidgetStack* Stack = LayerStacks.Find(Layer);
    if (Stack)
    {
        for (UUserWidget* Widget : Stack->Widgets)
        {
            if (IsValid(Widget))
            {
                Widget->RemoveFromParent();
            }
        }
        Stack->Widgets.Empty();
        UE_LOG(LogTemp, Log, TEXT("Cleared all widgets from Layer %d"), static_cast<int32>(Layer));
    }
}

void USpartaUIManagerSubsystem::ClearAllLayers()
{
    for (auto& Pair : LayerStacks)
    {
        ClearLayer(Pair.Key);
    }
    UE_LOG(LogTemp, Log, TEXT("Cleared all UI Layers"));
}

TArray<UUserWidget*> USpartaUIManagerSubsystem::GetWidgetsInLayer(EUILayer Layer) const
{
    const FSpartaWidgetStack* Stack = LayerStacks.Find(Layer);
    if (Stack)
    {
        return Stack->Widgets;
    }
    return TArray<UUserWidget*>();
}

int32 USpartaUIManagerSubsystem::GetZOrderForLayer(EUILayer Layer) const
{
    switch (Layer)
    {
    case EUILayer::GameHUD:
        return 0;
    case EUILayer::MenuScreen:
        return 10;
    case EUILayer::Popup:
        return 20;
    case EUILayer::SystemOverlay:
        return 30;
    default:
        return 0;
    }
}
