// Fill out your copyright notice in the Description page of Project Settings.


#include "BomberGameplayTags.h"

namespace BomberGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stunned, "State.Stunned",
		"캐릭터가 기절 상태임을 나타내는 태그");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invincible, "State.Invincible",
		"캐릭터가 무적 상태임을 나타내는 태그");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Shielded, "State.Shielded",
		"캐릭터가 방어막을 보유 중임을 나타내는 태그");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Eliminated, "State.Eliminated",
		"캐릭터가 탈락했음을 나타내는 태그");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_PlaceBomb, "Cooldown.PlaceBomb",
		"폭탄 설치 쿨다운 중임을 나타내는 태그");
}
