// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_PlaceBomb.h"
#include "SpartaArcadeBomb.h"
#include "BomberGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "BomberAttributeSet.h"
#include "SpartaArcadeCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

UGA_PlaceBomb::UGA_PlaceBomb()
{
	// 기절 중엔 발동 X
	ActivationBlockedTags.AddTag(BomberGameplayTags::State_Stunned);

	// 서버에서만
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// 폭탄이 몇 초 뒤 터질 때 이 인스턴스의 HandleBombExploded를 호출해야 하므로,
	// 매 발동마다 인스턴스가 새로 생성/폐기되는 정책이면 콜백이 조용히 무시된다.
	// 캐릭터당 하나의 인스턴스가 계속 살아있도록 명시적으로 고정.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_PlaceBomb::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!IsValid(AvatarActor) || !IsValid(BombClass))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	ASpartaArcadeCharacter* Character = Cast<ASpartaArcadeCharacter>(AvatarActor);
	if (!IsValid(Character))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// AttributeSet에서 폭발 범위 / 최대 폭탄 개수 읽어오기
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	UBomberAttributeSet* AttrSet = ASC ? const_cast<UBomberAttributeSet*>(ASC->GetSet<UBomberAttributeSet>()) : nullptr;
	if (!IsValid(AttrSet))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	CachedASC = ASC;

	int32 MaxBombCount = FMath::RoundToInt(AttrSet->GetBombCount());
	if (!CanPlaceBomb(FMath::RoundToInt(AttrSet->GetCurrentPlacedBombs()), MaxBombCount))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector SpawnLocation = CalcGridSnappedSpawnLocation(AvatarActor);

	// 동일한 격자 자리에 폭탄이 중복 스폰되는 것을 방지
	if (IsSpawnLocationOccupied(SpawnLocation))
	{
		UE_LOG(LogTemp, Warning, TEXT("폭탄 중복 설치가 감지되어 스폰을 차단합니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	int32 BombRange = FMath::RoundToInt(AttrSet->GetBombRange());

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASpartaArcadeBomb* NewBomb = AvatarActor->GetWorld()->SpawnActor<ASpartaArcadeBomb>(
		BombClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (IsValid(NewBomb))
	{
		// 실제 폭탄 설치 및 스폰 성공 시에만 몽타주를 재생하도록 변경
		Character->PlayPlaceBombAnim();

		NewBomb->InitializeBomb(Character, BombRange);
		NewBomb->OnBombExploded.BindUObject(this, &UGA_PlaceBomb::HandleBombExploded);

		ASC->ApplyModToAttribute(UBomberAttributeSet::GetCurrentPlacedBombsAttribute(), EGameplayModOp::Additive, 1.f);
		UE_LOG(LogTemp, Warning, TEXT("[GA_PlaceBomb] 설치! CurrentPlacedBombs=%d (Ability=%p)"), FMath::RoundToInt(AttrSet->GetCurrentPlacedBombs()), this);
		if (OnCurrentPlacedBombsChanged.IsBound())
		{
			OnCurrentPlacedBombsChanged.Broadcast(FMath::RoundToInt(AttrSet->GetCurrentPlacedBombs()));
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UGA_PlaceBomb::CanPlaceBomb(int32 CurrentCount, int32 MaxBombCount) const
{
	return CurrentCount < MaxBombCount;
}

FVector UGA_PlaceBomb::CalcGridSnappedSpawnLocation(AActor* AvatarActor) const
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(AvatarActor);
	if (!IsValid(OwnerCharacter))
	{
		return AvatarActor->GetActorLocation();
	}

	// 클래식 봄버맨 타일 일치를 위해 100단위 그리드로 보정
	FVector MyLocation = OwnerCharacter->GetActorLocation();
	float RoundedX = FMath::RoundToFloat(MyLocation.X / 100.f) * 100.f;
	float RoundedY = FMath::RoundToFloat(MyLocation.Y / 100.f) * 100.f;
	float SpawnZ = MyLocation.Z - OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 35.f;

	return FVector(RoundedX, RoundedY, SpawnZ);
}

bool UGA_PlaceBomb::IsSpawnLocationOccupied(const FVector& SpawnLocation) const
{
	TArray<AActor*> FoundBombs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpartaArcadeBomb::StaticClass(), FoundBombs);
	for (AActor* Actor : FoundBombs)
	{
		if (Actor && FVector::Dist2D(SpawnLocation, Actor->GetActorLocation()) < 80.f)
		{
			return true;
		}
	}
	return false;
}

void UGA_PlaceBomb::HandleBombExploded()
{
	UE_LOG(LogTemp, Warning, TEXT("[GA_PlaceBomb] HandleBombExploded 호출됨 (Ability=%p, CachedASC valid=%s)"), this, CachedASC.IsValid() ? TEXT("true") : TEXT("false"));

	if (!CachedASC.IsValid()) return;

	CachedASC->ApplyModToAttribute(UBomberAttributeSet::GetCurrentPlacedBombsAttribute(), EGameplayModOp::Additive, -1.f);
	UE_LOG(LogTemp, Warning, TEXT("[GA_PlaceBomb] 폭발 처리 완료! CurrentPlacedBombs=%d"), GetCurrentPlacedBombs());

	if (OnCurrentPlacedBombsChanged.IsBound())
	{
		OnCurrentPlacedBombsChanged.Broadcast(GetCurrentPlacedBombs());
	}
}

int32 UGA_PlaceBomb::GetCurrentPlacedBombs() const
{
	if (!CachedASC.IsValid()) return 0;

	const UBomberAttributeSet* AttrSet = CachedASC->GetSet<UBomberAttributeSet>();
	return IsValid(AttrSet) ? FMath::RoundToInt(AttrSet->GetCurrentPlacedBombs()) : 0;
}

void UGA_PlaceBomb::BroadcastCurrentState()
{
	if (OnCurrentPlacedBombsChanged.IsBound())
	{
		OnCurrentPlacedBombsChanged.Broadcast(GetCurrentPlacedBombs());
	}
}
