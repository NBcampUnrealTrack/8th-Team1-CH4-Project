#include "SpartaArcadeStatBar.h"
#include "SpartaArcadeStatSlot.h"
#include "Components/HorizontalBox.h"

void USpartaArcadeStatBar::InitializeBar(int32 InMaxStat, TSubclassOf<USpartaArcadeStatSlot> SlotClass)
{
	if (!HB_SlotContainer || !SlotClass) return;

	MaxStat = InMaxStat;
	HB_SlotContainer->ClearChildren();
	CreatedSlots.Empty();

	// 최대 스탯치 개수만큼 개별 슬롯 위젯을 동적 생성하여 수평 박스에 삽입
	for (int32 i = 0; i < MaxStat; ++i)
	{
		USpartaArcadeStatSlot* NewSlot = CreateWidget<USpartaArcadeStatSlot>(this, SlotClass);
		if (NewSlot)
		{
			// Modified: 바 위젯에 에디터로 입력된 두 개의 이미지 브러시 정보를 자식 슬롯에게 동적 전달
			NewSlot->SetupBrushes(BaseEmptyBrush, ObtainedFilledBrush);
			HB_SlotContainer->AddChildToHorizontalBox(NewSlot);
			CreatedSlots.Add(NewSlot);
		}
	}

	// 슬롯 생성 직후 현재 수치에 맞게 비주얼 강제 동기화
	UpdateStatBar(CurrentStat);
}

void USpartaArcadeStatBar::UpdateStatBar(int32 InCurrentStat)
{
	CurrentStat = InCurrentStat;

	// 생성된 슬롯 참조들을 돌며 NewCurrentStat 미만 인덱스는 Visible(채움), 이상은 Hidden(비움) 처리
	for (int32 i = 0; i < CreatedSlots.Num(); ++i)
	{
		if (CreatedSlots[i])
		{
			CreatedSlots[i]->SetFilled(i < CurrentStat);
		}
	}
}
