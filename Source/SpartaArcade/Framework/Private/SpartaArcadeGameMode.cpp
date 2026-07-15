#include "SpartaArcadeGameMode.h"
#include "SpartaArcadePlayerController.h"
#include "SpartaArcadeCharacter.h"
#include "Level/Public/SpartaArcadeMapGenerator.h"
#include "Level/Public/SpartaArcadeMapBuilder.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Character.h"
#include "Framework/Public/InGame/SpartaPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"

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

AActor* ASpartaArcadeGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!Player) return Super::ChoosePlayerStart_Implementation(Player);

	// 대기실/지정 대기 구역에 안전하게 먼저 스폰시키기 위해, 태그가 없는 기본 PlayerStart를 찾아서 반환합니다.
	// 이로 인해 맵 데이터 로딩 타이밍 어긋남에 의한 물리 끼임 현상을 원천 방지합니다.
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* Start = *It;
		if (Start && Start->PlayerStartTag.IsNone())
		{
			return Start;
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

//4인 플레이어 모서리 스폰을 위한 Logout 오버라이드 구현
void ASpartaArcadeGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// 접속 해제 시 고유 스폰 인덱스 반납 및 맵에서 제거
	if (AssignedSpawnIndices.Contains(Exiting))
	{
		int32 ReturnedIndex = AssignedSpawnIndices[Exiting];
		AssignedSpawnIndices.Remove(Exiting);
		UE_LOG(LogTemp, Warning, TEXT("[SpawnSystem] 플레이어 %s 가 퇴장하여 스폰 인덱스 %d 를 반납하였습니다."), *Exiting->GetName(), ReturnedIndex);
	}
}

void ASpartaArcadeGameMode::TeleportPlayersToSpawns(const TArray<FVector>& SpawnLocations)
{
	if (SpawnLocations.Num() == 0) return;

	UE_LOG(LogTemp, Warning, TEXT("[SpawnSystem] 맵 빌드 완료 감지: 플레이어 텔레포트 및 3x3 안전지대 확보 연산을 실행합니다."));

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC)
		{
			// 1. 이미 배정받은 고유 스폰 인덱스가 없다면 새로 순차적 할당
			int32 AssignedIndex = -1;
			if (AssignedSpawnIndices.Contains(PC))
			{
				AssignedIndex = AssignedSpawnIndices[PC];
			}
			else
			{
				TSet<int32> UsedIndices;
				for (const auto& Pair : AssignedSpawnIndices)
				{
					UsedIndices.Add(Pair.Value);
				}
				for (int32 i = 0; i < 4; ++i)
				{
					if (!UsedIndices.Contains(i))
					{
						AssignedIndex = i;
						break;
					}
				}
				if (AssignedIndex == -1)
				{
					AssignedIndex = AssignedSpawnIndices.Num() % 4;
				}
				AssignedSpawnIndices.Add(PC, AssignedIndex);
			}

			// 2. 해당 인덱스에 매치되는 텔레포트 목적지 월드 좌표 확보
			int32 TargetLocIndex = AssignedIndex % SpawnLocations.Num();
			FVector TeleportLoc = SpawnLocations[TargetLocIndex];
			APawn* PlayerPawn = PC->GetPawn();
			
			// 감지될 경우 벽 위가 아닌 벽 바로 옆(맵 중앙 방향으로 1칸 회피)에 텔레포트하도록 안전 연산 제공
			bool bIsFixedWallBlocked = false;
			TArray<FOverlapResult> WallOverlaps;
			FCollisionQueryParams TraceParams;
			if (PlayerPawn) TraceParams.AddIgnoredActor(PlayerPawn);
			TraceParams.AddIgnoredActor(PC);

			FCollisionShape CheckSphere = FCollisionShape::MakeSphere(40.f); // 캐릭터 캡슐 반경 정도 크기
			if (GetWorld()->OverlapMultiByObjectType(
				WallOverlaps,
				TeleportLoc,
				FQuat::Identity,
				FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects),
				CheckSphere,
				TraceParams))
			{
				for (const FOverlapResult& Overlap : WallOverlaps)
				{
					AActor* OverlapActor = Overlap.GetActor();
					if (OverlapActor)
					{
						FString ActorName = OverlapActor->GetName();
						if (ActorName.Contains(TEXT("Fixed")) || ActorName.Contains(TEXT("Wall")))
						{
							if (!ActorName.Contains(TEXT("Destructible")))
							{
								bIsFixedWallBlocked = true;
								break;
							}
						}
					}
				}
			}

			if (bIsFixedWallBlocked)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SpawnSystem] 목적지 (%s) 에 고정벽이 감지되었습니다. 맵 중앙 방향 옆으로 우회 회피합니다."), *TeleportLoc.ToString());
				
				float ShiftX = 0.f;
				float ShiftY = 0.f;
				const float ShiftDistance = 100.f; // 타일 1칸 만큼 옆으로 이동

				// 모서리 인덱스 분면에 따라 대각선 안쪽 방향 결정
				switch (AssignedIndex % 4)
				{
				case 0: // Corner0 (좌상단) -> 우하단 회피
					ShiftX = ShiftDistance;
					ShiftY = ShiftDistance;
					break;
				case 1: // Corner1 (우상단) -> 좌하단 회피
					ShiftX = -ShiftDistance;
					ShiftY = ShiftDistance;
					break;
				case 2: // Corner2 (좌하단) -> 우상단 회피
					ShiftX = ShiftDistance;
					ShiftY = -ShiftDistance;
					break;
				case 3: // Corner3 (우하단) -> 좌상단 회피
					ShiftX = -ShiftDistance;
					ShiftY = -ShiftDistance;
					break;
				default:
					break;
				}

				TeleportLoc.X += ShiftX;
				TeleportLoc.Y += ShiftY;
			}

			// 캐릭터의 캡슐이 땅에 얹어지거나 묻히지 않도록 Z축 미세 오프셋 보강
			TeleportLoc.Z += 10.f;

			if (PlayerPawn)
			{
				FRotator TeleportRot = FRotator::ZeroRotator;
				
				// 3. 안전한 물리 텔레포트 수행 및 클라이언트 무브먼트 리셋 동기화 (클라이언트가 굳어지지 않고 바로 조종 가능하게 처리)
				PlayerPawn->TeleportTo(TeleportLoc, TeleportRot, false, true);
				PC->ClientSetLocation(TeleportLoc, TeleportRot);
				UE_LOG(LogTemp, Warning, TEXT("[SpawnSystem] 플레이어 %s 를 스폰 인덱스 %d 좌표 (%s) 로 텔레포트 시켰습니다."), *PC->GetName(), AssignedIndex, *TeleportLoc.ToString());

				// 4. 텔레포트 주변 3x3 범위 내의 파괴 가능한 상자(Destructible Box/Actor)들만 감지해 완전 파괴 (외곽 고정벽 제외)
				// 타일 사이즈가 대략 100cm 이므로 3x3 범위는 반경 약 140cm 로 긁으면 주변 8칸이 정확하게 감지됩니다.
				TArray<FOverlapResult> Overlaps;
				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(PlayerPawn);
				QueryParams.AddIgnoredActor(PC);

				FCollisionShape Sphere = FCollisionShape::MakeSphere(140.f);
				if (GetWorld()->OverlapMultiByObjectType(
					Overlaps,
					TeleportLoc,
					FQuat::Identity,
					FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects),
					Sphere,
					QueryParams))
				{
					for (const FOverlapResult& Overlap : Overlaps)
					{
						AActor* OverlapActor = Overlap.GetActor();
						if (OverlapActor)
						{
							FString ActorName = OverlapActor->GetName();
							// 외곽 고정벽(FixedWall)은 제외하고, 파괴 가능한 상자(Destructible)만 골라서 파괴
							if (ActorName.Contains(TEXT("Destructible")) || ActorName.Contains(TEXT("Box")))
							{
								if (!ActorName.Contains(TEXT("Fixed")) && !ActorName.Contains(TEXT("Wall")))
								{
									OverlapActor->Destroy();
									UE_LOG(LogTemp, Warning, TEXT("[SpawnSystem] 스폰 안전구역 방해 상자 액터 %s 를 파괴했습니다."), *ActorName);
								}
							}
						}
					}
				}
			}
		}
	}
}

void ASpartaArcadeGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (!NewPlayer) return;

	APlayerController* PC = Cast<APlayerController>(NewPlayer);
	if (PC)
	{
		// ASpartaArcadeMapBuilder가 이미 맵 생성을 완료했는지 감지
		for (TActorIterator<ASpartaArcadeMapBuilder> It(GetWorld()); It; ++It)
		{
			ASpartaArcadeMapBuilder* MapBuilder = *It;
			if (MapBuilder && MapBuilder->bMapBuilt)
			{
				// 이미 맵이 빌드 완료되어 비주얼까지 수립된 상태이므로, 대기실에 태어난 폰을 즉각 인게임 모서리로 텔레포트
				APawn* PlayerPawn = PC->GetPawn();
				if (PlayerPawn)
				{
					const TArray<FVector>& SpawnLocations = MapBuilder->GetSpawnWorldLocations();
					if (SpawnLocations.Num() > 0)
					{
						int32 AssignedIndex = -1;
						if (AssignedSpawnIndices.Contains(PC))
						{
							AssignedIndex = AssignedSpawnIndices[PC];
						}
						else
						{
							TSet<int32> UsedIndices;
							for (const auto& Pair : AssignedSpawnIndices)
							{
								UsedIndices.Add(Pair.Value);
							}
							for (int32 i = 0; i < 4; ++i)
							{
								if (!UsedIndices.Contains(i))
								{
									AssignedIndex = i;
									break;
								}
							}
							if (AssignedIndex == -1)
							{
								AssignedIndex = AssignedSpawnIndices.Num() % 4;
							}
							AssignedSpawnIndices.Add(PC, AssignedIndex);
						}

						int32 TargetLocIndex = AssignedIndex % SpawnLocations.Num();
						FVector TeleportLoc = SpawnLocations[TargetLocIndex];
						
						// 고정벽 회피
						bool bIsFixedWallBlocked = false;
						TArray<FOverlapResult> WallOverlaps;
						FCollisionQueryParams TraceParams;
						TraceParams.AddIgnoredActor(PlayerPawn);
						TraceParams.AddIgnoredActor(PC);

						FCollisionShape CheckSphere = FCollisionShape::MakeSphere(40.f);
						if (GetWorld()->OverlapMultiByObjectType(
							WallOverlaps,
							TeleportLoc,
							FQuat::Identity,
							FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects),
							CheckSphere,
							TraceParams))
						{
							for (const FOverlapResult& Overlap : WallOverlaps)
							{
								AActor* OverlapActor = Overlap.GetActor();
								if (OverlapActor)
								{
									FString ActorName = OverlapActor->GetName();
									if (ActorName.Contains(TEXT("Fixed")) || ActorName.Contains(TEXT("Wall")))
									{
										if (!ActorName.Contains(TEXT("Destructible")))
										{
											bIsFixedWallBlocked = true;
											break;
										}
									}
								}
							}
						}

						if (bIsFixedWallBlocked)
						{
							float ShiftX = 0.f;
							float ShiftY = 0.f;
							const float ShiftDistance = 100.f;

							switch (AssignedIndex % 4)
							{
							case 0: ShiftX = ShiftDistance; ShiftY = ShiftDistance; break;
							case 1: ShiftX = -ShiftDistance; ShiftY = ShiftDistance; break;
							case 2: ShiftX = ShiftDistance; ShiftY = -ShiftDistance; break;
							case 3: ShiftX = -ShiftDistance; ShiftY = -ShiftDistance; break;
							}
							TeleportLoc.X += ShiftX;
							TeleportLoc.Y += ShiftY;
						}

						TeleportLoc.Z += 10.f;
						FRotator TeleportRot = FRotator::ZeroRotator;
						
						PlayerPawn->TeleportTo(TeleportLoc, TeleportRot, false, true);
						PC->ClientSetLocation(TeleportLoc, TeleportRot);

						// 3x3 박스 소거
						TArray<FOverlapResult> Overlaps;
						FCollisionQueryParams QueryParams;
						QueryParams.AddIgnoredActor(PlayerPawn);
						QueryParams.AddIgnoredActor(PC);

						FCollisionShape Sphere = FCollisionShape::MakeSphere(140.f);
						if (GetWorld()->OverlapMultiByObjectType(
							Overlaps,
							TeleportLoc,
							FQuat::Identity,
							FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects),
							Sphere,
							QueryParams))
						{
							for (const FOverlapResult& Overlap : Overlaps)
							{
								AActor* OverlapActor = Overlap.GetActor();
								if (OverlapActor)
								{
									FString ActorName = OverlapActor->GetName();
									if (ActorName.Contains(TEXT("Destructible")) || ActorName.Contains(TEXT("Box")))
									{
										if (!ActorName.Contains(TEXT("Fixed")) && !ActorName.Contains(TEXT("Wall")))
										{
											OverlapActor->Destroy();
										}
									}
								}
							}
						}
					}
				}
				break;
			}
		}
	}
}
