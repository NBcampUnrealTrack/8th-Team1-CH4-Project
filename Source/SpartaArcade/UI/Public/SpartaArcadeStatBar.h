#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "SpartaArcadeStatBar.generated.h"

class UHorizontalBox;
class USpartaArcadeStatSlot;


UCLASS()
class SPARTAARCADE_API USpartaArcadeStatBar : public UUserWidget
{
	GENERATED_BODY()

public:
	// 스탯바를 주어진 최대치 슬롯 템플릿 크기만큼 빌드(생성)합니다.
	UFUNCTION(BlueprintCallable, Category = "SpartaArcade|UI")
	void InitializeBar(int32 InMaxStat, TSubclassOf<USpartaArcadeStatSlot> SlotClass);

	// 현재 스탯 수치로 바의 칸 채우기 상태를 실시간 갱신합니다.
	UFUNCTION(BlueprintCallable, Category = "SpartaArcade|UI")
	void UpdateStatBar(int32 InCurrentStat);

protected:
	// 슬롯들을 일렬로 정렬해 담는 가로 정렬 박스 컴포넌트 (BindWidget 바인딩)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> HB_SlotContainer;

	// 에디터(블루프린트 디테일창)에서 일괄 관리할 기본(비어있음) 이미지 브러시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	FSlateBrush BaseEmptyBrush;

	// 에디터에서 일괄 관리할 획득 스탯(채워짐) 이미지 브러시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	FSlateBrush ObtainedFilledBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpartaArcade|UI")
	int32 MaxStat = 5;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SpartaArcade|UI")
	int32 CurrentStat = 0;

	// 런타임에 동적 스폰된 슬롯 인스턴스 보관용 배열
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SpartaArcade|UI")
	TArray<TObjectPtr<USpartaArcadeStatSlot>> CreatedSlots;
};
