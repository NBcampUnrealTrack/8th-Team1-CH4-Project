#include "SpartaArcadeGameMode.h"
#include "SpartaArcadePlayerController.h"
#include "SpartaArcadeCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"
#include "GameFramework/GameStateBase.h"

ASpartaArcadeGameMode::ASpartaArcadeGameMode()
{
	// 커스텀 플레이어 컨트롤러 클래스
	PlayerControllerClass = ASpartaArcadePlayerController::StaticClass();

	// Todo : 플레이어 폰 클래스 생성 후 지정
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	// Todo : 플레이어 컨트롤러 새로 생성 후 지정
	static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownPlayerController"));
	if(PlayerControllerBPClass.Class != NULL)
	{
		PlayerControllerClass = PlayerControllerBPClass.Class;
	}
}

void ASpartaArcadeGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 팀 모드가 켜져 있을 때만 입장한 플레이어들을 Team 1과 Team 2로 자동 분배
	if (ASpartaPlayerState* SPS = NewPlayer->GetPlayerState<ASpartaPlayerState>())
	{
		if (bIsTeamMode)
		{
			int32 NumPlayers = 0;
			if (AGameStateBase* GS = GetWorld()->GetGameState())
			{
				NumPlayers = GS->PlayerArray.Num();
			}

			int32 AssignedTeam = (NumPlayers % 2 == 1) ? 1 : 2;
			SPS->SetTeamID(AssignedTeam);
			UE_LOG(LogTemp, Warning, TEXT("[TeamBalance] %s 플레이어가 Team %d로 자동 배정되었습니다."), *NewPlayer->GetName(), AssignedTeam);
		}
		else
		{
			// 개인전일 경우 닉네임이 항상 흰색으로 뜨도록 팀 ID를 0으로 고정
			SPS->SetTeamID(0);
		}
	}
}

void ASpartaArcadeGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 게임 진입 즉시 양 팀의 인원 수 균형을 강제 체크하고 보정 처리 실행
	BalanceTeams();
}

void ASpartaArcadeGameMode::BalanceTeams()
{
	UWorld* World = GetWorld();
	if (!World) return;

	AGameStateBase* GS = World->GetGameState();
	if (!GS) return;

	// 팀전 모드가 아닐 경우 모든 플레이어의 팀 ID를 0으로 초기화하고 조율 생략
	if (!bIsTeamMode)
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			if (ASpartaPlayerState* SPS = Cast<ASpartaPlayerState>(PS))
			{
				SPS->SetTeamID(0);
			}
		}
		return;
	}

	TArray<ASpartaPlayerState*> Team1Players;
	TArray<ASpartaPlayerState*> Team2Players;
	TArray<ASpartaPlayerState*> UnassignedPlayers;

	// 1. 현재 맵에 복제 완료된 모든 플레이어들의 팀 소속 현황 분류
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (ASpartaPlayerState* SPS = Cast<ASpartaPlayerState>(PS))
		{
			int32 TeamID = SPS->GetTeamID();
			if (TeamID == 1)
			{
				Team1Players.Add(SPS);
			}
			else if (TeamID == 2)
			{
				Team2Players.Add(SPS);
			}
			else
			{
				UnassignedPlayers.Add(SPS);
			}
		}
	}

	// 2. 미배정된 플레이어가 있다면 인원수가 적은 팀 방향으로 우선 할당
	for (ASpartaPlayerState* SPS : UnassignedPlayers)
	{
		if (Team1Players.Num() <= Team2Players.Num())
		{
			SPS->SetTeamID(1);
			Team1Players.Add(SPS);
		}
		else
		{
			SPS->SetTeamID(2);
			Team2Players.Add(SPS);
		}
	}

	// 3. 양 팀의 인원수 격차가 2명 이상 벌어져 불균형인 경우 강제 팀 이전 조율
	while (FMath::Abs(Team1Players.Num() - Team2Players.Num()) > 1)
	{
		if (Team1Players.Num() > Team2Players.Num())
		{
			ASpartaPlayerState* MovePlayer = Team1Players.Pop();
			MovePlayer->SetTeamID(2);
			Team2Players.Add(MovePlayer);
			
			// 닉네임 비주얼 갱신 연계
			if (APawn* Pawn = MovePlayer->GetPawn())
			{
				if (ASpartaArcadeCharacter* Character = Cast<ASpartaArcadeCharacter>(Pawn))
				{
					Character->UpdateNickname();
				}
			}
			UE_LOG(LogTemp, Warning, TEXT("[TeamBalance] %s 플레이어가 팀 밸런스 균등 조율을 위해 Team 2로 임의 이동 배정되었습니다."), *MovePlayer->GetPlayerName());
		}
		else
		{
			ASpartaPlayerState* MovePlayer = Team2Players.Pop();
			MovePlayer->SetTeamID(1);
			Team1Players.Add(MovePlayer);

			if (APawn* Pawn = MovePlayer->GetPawn())
			{
				if (ASpartaArcadeCharacter* Character = Cast<ASpartaArcadeCharacter>(Pawn))
				{
					Character->UpdateNickname();
				}
			}
			UE_LOG(LogTemp, Warning, TEXT("[TeamBalance] %s 플레이어가 팀 밸런스 균등 조율을 위해 Team 1로 임의 이동 배정되었습니다."), *MovePlayer->GetPlayerName());
		}
	}
}