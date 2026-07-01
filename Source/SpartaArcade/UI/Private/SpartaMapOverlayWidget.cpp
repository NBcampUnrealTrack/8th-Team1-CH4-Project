#include "SpartaMapOverlayWidget.h"
#include "Components/UniformGridPanel.h"
#include "Blueprint/UserWidget.h"

void USpartaMapOverlayWidget::RefreshMap(const TArray<FIntPoint>& DiscoveredRooms, FIntPoint CurrentRoom, const TArray<FIntPoint>& DangerRooms)
{
	// 그리드 패널 및 방 클래스가 바인딩되어 있는지 사전 체크
	if (!MapGridPanel || !RoomUnitWidgetClass) return;

	// 맵을 갱신하기 전에 기존에 배치되어 있던 모든 방 위젯 청소
	MapGridPanel->ClearChildren();

	// 탐색 완료된 모든 방 좌표들을 순회하며 Uniform Grid에 정렬 배치
	for (const FIntPoint& RoomCoord : DiscoveredRooms)
	{
		UUserWidget* RoomWidget = CreateWidget<UUserWidget>(this, RoomUnitWidgetClass);
		if (RoomWidget)
		{
			// X 좌표는 Column, Y 좌표는 Row에 매핑하여 격자 배치
			MapGridPanel->AddChildToUniformGrid(RoomWidget, RoomCoord.Y, RoomCoord.X);

			// 블루프린트 하위 클래스에서 정의할 "SetRoomState(bool bIsCurrent, bool bIsDanger)" 함수 동적 리플렉션 호출
			struct FSetRoomStateParams
			{
				bool bIsCurrent;
				bool bIsDanger;
			};

			UFunction* SetStateFunc = RoomWidget->FindFunction(FName("SetRoomState"));
			if (SetStateFunc)
			{
				FSetRoomStateParams Params;
				Params.bIsCurrent = (RoomCoord == CurrentRoom);
				Params.bIsDanger = DangerRooms.Contains(RoomCoord);
				
				// UObject 리플렉션 시스템을 통해 블루프린트 이벤트를 강제 트리거
				RoomWidget->ProcessEvent(SetStateFunc, &Params);
			}
		}
	}
}
