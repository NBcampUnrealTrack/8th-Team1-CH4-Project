#include "SpartaGameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"

void USpartaGameViewportClient::LayoutPlayers()
{
	Super::LayoutPlayers();

	// 로컬 플레이어가 존재하고 첫 번째 플레이어가 유효한 경우 뷰포트 영역을 강제 제어
	if (GameInstance && GameInstance->GetNumLocalPlayers() > 0)
	{
		ULocalPlayer* Player = GameInstance->GetLocalPlayerByIndex(0);
		if (Player)
		{
			float ViewportWidthRatio = 0.8f;  // 게임 화면 가로 비율 (0.0 ~ 1.0)
			float ViewportHeightRatio = 1.0f; // 게임 화면 세로 비율 (0.0 ~ 1.0)
			float ViewportOriginX = 0.0f;     // 시작 X 위치 (0.0 = 맨 왼쪽)
			float ViewportOriginY = 0.0f;     // 시작 Y 위치 (0.0 = 맨 위쪽)

			Player->Size.X = ViewportWidthRatio;
			Player->Size.Y = ViewportHeightRatio;
			Player->Origin.X = ViewportOriginX;
			Player->Origin.Y = ViewportOriginY;
		}
	}
}
