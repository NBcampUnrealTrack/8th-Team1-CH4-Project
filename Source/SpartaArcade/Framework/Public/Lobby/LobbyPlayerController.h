// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

enum class ESpartaArcadeCharacterType : uint8;
class USpartaMenuFlowWidget;
class USoundBase;
UCLASS()
class SPARTAARCADE_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
    virtual void BeginPlay() override;

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSelectCharacter(ESpartaArcadeCharacterType NewType);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerToggleReady();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStartMatch();

    // 팀 수동 선택 요청을 서버로 전달하는 RPC 추가
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSelectTeam(int32 NewTeamID);

    // 방장의 팀 자동 배분 설정 변경 요청을 서버로 전달하는 RPC 추가
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetAutoBalanceTeam(bool bEnabled);

	void LeaveLobby();

    void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);

private:
    // 로비 화면 진입 시 재생할 배경음악
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sound", Meta = (AllowPrivateAccess))
    TObjectPtr<USoundBase> LevelBGM;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = PlayerController, Meta = (AllowPrivateAccess))
    TSubclassOf<USpartaMenuFlowWidget> MainMenuWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = PlayerController, Meta = (AllowPrivateAccess))
    TObjectPtr<USpartaMenuFlowWidget> MainMenuWidgetInstance;
};
