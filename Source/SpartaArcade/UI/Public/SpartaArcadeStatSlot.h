#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "SpartaArcadeStatSlot.generated.h"

class UImage;

UCLASS()
class SPARTAARCADE_API USpartaArcadeStatSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	// 스탯 상태(획득/미획득)에 따라 이미지를 교체하는 제어 함수
	UFUNCTION(BlueprintCallable, Category = "SpartaArcade|UI")
	void SetFilled(bool bFill);

	// 바 컴포넌트에서 초기화 시점에 기본(비어있음) 이미지와 스탯(채워짐) 이미지 브러시를 주입하는 헬퍼 함수
	void SetupBrushes(const FSlateBrush& InEmptyBrush, const FSlateBrush& InFilledBrush);

protected:
	// 슬롯의 비주얼을 표현할 단일 이미지 컴포넌트 (BindWidget 바인딩)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> StatImage;

	// 캐싱된 브러시 슬레이트 정보
	FSlateBrush EmptyBrush;
	FSlateBrush FilledBrush;
};
