#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpartaMapOverlayWidget.generated.h"

class UUniformGridPanel;

UCLASS()
class SPARTAARCADE_API USpartaMapOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 격자 구조로 방 위젯들을 동적 정렬할 그리드 패널
	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* MapGridPanel;

	// 격자에 들어갈 개별 방(Room) 단위 위젯
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI | Map")
	TSubclassOf<UUserWidget> RoomUnitWidgetClass;

public:
	// 탐색된 방 목록, 현재 플레이어 방, 자기장 방 목록을 받아 맵을 갱신
	UFUNCTION(BlueprintCallable, Category = "UI | Map")
	void RefreshMap(const TArray<FIntPoint>& DiscoveredRooms, FIntPoint CurrentRoom, const TArray<FIntPoint>& DangerRooms);
};
