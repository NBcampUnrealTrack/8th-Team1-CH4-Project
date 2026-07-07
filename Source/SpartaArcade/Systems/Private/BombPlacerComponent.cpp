#include "BombPlacerComponent.h"
#include "StatComponent.h"
#include "SpartaArcadeBomb.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "SpartaArcadeCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UBombPlacerComponent::UBombPlacerComponent()
{
	SetIsReplicatedByDefault(true);

	PrimaryComponentTick.bCanEverTick = false;
}

void UBombPlacerComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
     Super::GetLifetimeReplicatedProps(OutLifetimeProps);
     DOREPLIFETIME(UBombPlacerComponent, CurrentPlacedBombs);
}

void UBombPlacerComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedStatComponent = GetOwner()->FindComponentByClass<UStatComponent>();
}

void UBombPlacerComponent::ServerPlaceBomb_Implementation()
{
	if (!CanPlaceBomb()) return;
	if (!IsValid(BombClass)) return;

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	FVector SpawnLocation = GetOwner()->GetActorLocation();

	// SpartaArcadeCharacter와 호환되도록 타일 격자 100단위로 정렬하여 스폰 위치 설정
	if (OwnerCharacter)
	{
		FVector MyLocation = OwnerCharacter->GetActorLocation();
		float RoundedX = FMath::RoundToFloat(MyLocation.X / 100.f) * 100.f;
		float RoundedY = FMath::RoundToFloat(MyLocation.Y / 100.f) * 100.f;
		float SpawnZ = MyLocation.Z - OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 50.f;
		SpawnLocation = FVector(RoundedX, RoundedY, SpawnZ);
	}

	// 동일한 격자 자리에 폭탄이 중복 스폰되는 것을 방지하기 위해 2D 거리 검사 수행
	TArray<AActor*> FoundBombs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpartaArcadeBomb::StaticClass(), FoundBombs);
	for (AActor* Actor : FoundBombs)
	{
		if (Actor)
		{
			// 2D 상의 평면 거리가 80유닛 미만이면 같은 칸으로 판별하여 스폰 차단
			float Dist2D = FVector::Dist2D(SpawnLocation, Actor->GetActorLocation());
			if (Dist2D < 80.f)
			{
				UE_LOG(LogTemp, Warning, TEXT("폭탄 중복 설치가 감지되어 스폰을 차단합니다."));
				return;
			}
		}
	}

	FActorSpawnParameters SpawnParams;
	// Z축 물리 간섭 방지 및 스폰 당시의 겹침 허용을 위해 AlwaysSpawn으로 변경
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASpartaArcadeBomb* NewBomb = GetWorld()->SpawnActor<ASpartaArcadeBomb>(
		BombClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (!IsValid(NewBomb)) return;


	NewBomb->OnBombExploded.BindUObject(this, &UBombPlacerComponent::OnBombExploded);

	int32 ExplosionRange = IsValid(CachedStatComponent)
		? CachedStatComponent->GetBombRange()
		: 2;

	ASpartaArcadeCharacter* OwnerArcadeCharacter = Cast<ASpartaArcadeCharacter>(GetOwner());
	NewBomb->InitializeBomb(OwnerArcadeCharacter, ExplosionRange);
	CurrentPlacedBombs++;
}

bool UBombPlacerComponent::CanPlaceBomb() const
{
	if (!IsValid(CachedStatComponent)) return false;

	return CurrentPlacedBombs < CachedStatComponent->GetBombCount();
}

void UBombPlacerComponent::OnBombExploded()
{
	CurrentPlacedBombs--;
}

void UBombPlacerComponent::OnRep_CurrentPlacedBombs()
{
	if(OnCurrentPlacedBombsChanged.IsBound())
	{
		OnCurrentPlacedBombsChanged.Broadcast(CurrentPlacedBombs);
	}
}

void UBombPlacerComponent::BroadcastCurrentState()
{
	OnRep_CurrentPlacedBombs();
}