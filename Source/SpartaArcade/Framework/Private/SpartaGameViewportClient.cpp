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
			float ViewportWidthRatio = 1.0f;  // 기본값: 전체 화면 가로 비율 (메뉴 등)
			float ViewportHeightRatio = 1.0f; // 기본값: 전체 화면 세로 비율
			float ViewportOriginX = 0.0f;     // 시작 X 위치 (0.0 = 맨 왼쪽)
			float ViewportOriginY = 0.0f;     // 시작 Y 위치 (0.0 = 맨 위쪽)

			// 현재 맵 이름에 TitleMap 또는 LobbyMap이 포함되어 있다면 메뉴/대기 화면이므로 전체 화면(1.0f)으로 지정하고, 그 외에는 0.8f 비율을 적용
			UWorld* WorldMap = GetWorld();
			if (WorldMap)
			{
				FString MapName = WorldMap->GetMapName();
				if (MapName.Contains(TEXT("TitleMap")) || MapName.Contains(TEXT("LobbyMap")))
				{
					ViewportWidthRatio = 1.0f;
				}
				else
				{
					ViewportWidthRatio = 0.8f;
				}
			}

			Player->Size.X = ViewportWidthRatio;
			Player->Size.Y = ViewportHeightRatio;
			Player->Origin.X = ViewportOriginX;
			Player->Origin.Y = ViewportOriginY;
		}
	}
}
