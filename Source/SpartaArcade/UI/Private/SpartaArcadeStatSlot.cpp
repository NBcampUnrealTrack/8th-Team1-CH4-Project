#include "SpartaArcadeStatSlot.h"
#include "Components/Image.h"

void USpartaArcadeStatSlot::SetupBrushes(const FSlateBrush& InEmptyBrush, const FSlateBrush& InFilledBrush)
{
	EmptyBrush = InEmptyBrush;
	FilledBrush = InFilledBrush;
}

void USpartaArcadeStatSlot::SetFilled(bool bFill)
{
	if (StatImage)
	{
		// 획득 여부에 따라 주입받은 기본 이미지(Empty)와 획득 스탯 이미지(Filled)를 실시간 교체
		StatImage->SetBrush(bFill ? FilledBrush : EmptyBrush);
	}
}
