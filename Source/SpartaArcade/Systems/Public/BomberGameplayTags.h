#pragma once

#include "NativeGameplayTags.h"

namespace BomberGameplayTags
{
	// 상태 관련 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stunned);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Shielded);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Eliminated);

	// 쿨다운 관련 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_PlaceBomb);
}