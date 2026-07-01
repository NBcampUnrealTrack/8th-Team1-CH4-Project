#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "SpartaGameViewportClient.generated.h"

UCLASS()
class SPARTAARCADE_API USpartaGameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()

public:
	// 로컬 플레이어의 뷰포트 크기와 위치를 비대칭으로 재조정하기 위해 LayoutPlayers를 오버라이드
	virtual void LayoutPlayers() override;
};
