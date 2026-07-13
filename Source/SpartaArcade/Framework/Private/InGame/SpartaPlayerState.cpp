#include "InGame/SpartaPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Systems/Public/BomberTypes.h"

void ASpartaPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpartaPlayerState, CharacterType);
	DOREPLIFETIME(ASpartaPlayerState, FirstAidKits);
	DOREPLIFETIME(ASpartaPlayerState, TeamID);
	DOREPLIFETIME(ASpartaPlayerState, Hearts);
	DOREPLIFETIME(ASpartaPlayerState, CurrentState);
}

void ASpartaPlayerState::SetCharacterType(ESpartaArcadeCharacterType NewType)
{
	if (HasAuthority())
	{
		CharacterType = NewType;
	}
}

ESpartaArcadeCharacterType ASpartaPlayerState::GetCharacterType() const
{
	return CharacterType;
}

void ASpartaPlayerState::SetFirstAidKits(int32 NewCount)
{
	if (HasAuthority())
	{
		FirstAidKits = NewCount;
		// 서버 측에서 구급약 획득 시 이벤트를 즉각 브로드캐스트
		OnRep_FirstAidKits();
	}
}

int32 ASpartaPlayerState::GetFirstAidKits() const
{
	return FirstAidKits;
}

void ASpartaPlayerState::SetTeamID(int32 NewTeamID)
{
	if(HasAuthority())
	{
		TeamID = NewTeamID;
	}
}

int32 ASpartaPlayerState::GetTeamID() const
{
	return TeamID;
}

void ASpartaPlayerState::SetHearts(int32 NewHearts)
{
	if (HasAuthority())
	{
		Hearts = NewHearts;
		OnRep_Hearts();
	}
}

int32 ASpartaPlayerState::GetHearts() const
{
	return Hearts;
}

void ASpartaPlayerState::SetCurrentState(EBomberPlayerState NewState)
{
	if(HasAuthority())
	{
		CurrentState = NewState;
		OnRep_CurrentState();
	}
}

EBomberPlayerState ASpartaPlayerState::GetCurrentState() const
{
	return CurrentState;
}

void ASpartaPlayerState:: SetStartHearts(int32 NewStartHearts)
{
	StartHearts = NewStartHearts;
}

int32 ASpartaPlayerState::GetStartHearts() const
{
	return StartHearts;
}

void ASpartaPlayerState::SetSelfReviveHearts(int32 NewSelfReviveHearts)
{
	SelfReviveHearts = NewSelfReviveHearts;
}

int32 ASpartaPlayerState::GetSelfReviveHearts() const
{
	return SelfReviveHearts;
}

void ASpartaPlayerState::OnRep_Hearts()
{
	if (OnHeartsChanged.IsBound())
	{
		OnHeartsChanged.Broadcast(Hearts, StartHearts);
	}
}

void ASpartaPlayerState::OnRep_CurrentState()
{
	if (OnStunStateChanged.IsBound())
	{
		OnStunStateChanged.Broadcast(CurrentState == EBomberPlayerState::Stunned);
	}
}
	

//클라이언트에서 구급약 개수 복제 수신 시 동적 UI 이벤트 브로드캐스트
void ASpartaPlayerState::OnRep_FirstAidKits()
{
	if (OnFirstAidKitsChanged.IsBound())
	{
		OnFirstAidKitsChanged.Broadcast(FirstAidKits);
	}
}

void ASpartaPlayerState::BroadcastCurrentState()
{
	OnRep_Hearts();
	OnRep_CurrentState();
	// 구급약 개수 초기 상태 브로드캐스트 호출 추가
	OnRep_FirstAidKits();
}