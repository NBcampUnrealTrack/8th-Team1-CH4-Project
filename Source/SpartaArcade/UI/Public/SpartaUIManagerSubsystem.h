#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SpartaUIDefs.h"
#include "Blueprint/UserWidget.h"
#include "SpartaUIManagerSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FSpartaWidgetStack
{
    GENERATED_BODY()

    // 해당 레이어에 생성된 위젯 목록
    UPROPERTY(BlueprintReadOnly, Category = "UI Manager")
    TArray<UUserWidget*> Widgets;
};


// 전역 GameInstance 수명 주기에 맞춰 각 UI 레이어(GameHUD, MenuScreen, Popup, SystemOverlay)의 위젯 스택을 관리합니다.

UCLASS(BlueprintType, Blueprintable)
class SPARTAARCADE_API USpartaUIManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // 특정 레이어에 위젯을 생성하고 뷰포트에 추가 (Z-Order 자동 설정)
    UFUNCTION(BlueprintCallable, Category = "UI Manager", meta = (WorldContext = "WorldContextObject"))
    UUserWidget* PushWidget(EUILayer Layer, TSubclassOf<UUserWidget> WidgetClass);

    // 특정 레이어의 최상위 위젯 제거
    UFUNCTION(BlueprintCallable, Category = "UI Manager")
    void PopWidget(EUILayer Layer);

    // 특정 레이어의 모든 위젯 제거
    UFUNCTION(BlueprintCallable, Category = "UI Manager")
    void ClearLayer(EUILayer Layer);

    // 모든 레이어의 모든 위젯 제거
    UFUNCTION(BlueprintCallable, Category = "UI Manager")
    void ClearAllLayers();

    // 특정 레이어의 활성화된 위젯 리스트 반환
    UFUNCTION(BlueprintCallable, Category = "UI Manager")
    TArray<UUserWidget*> GetWidgetsInLayer(EUILayer Layer) const;

private:
    // 각 레이어별 위젯 스택 저장 맵
    UPROPERTY()
    TMap<EUILayer, FSpartaWidgetStack> LayerStacks;

    // 레이어 정의에 따라 지정된 Z-Order를 반환하는 헬퍼 함수
    int32 GetZOrderForLayer(EUILayer Layer) const;
};
