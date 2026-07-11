#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "AuthService.generated.h"


class IOnlineSubsystem;

UCLASS()
class SPARTAARCADE_API UAuthService : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(IOnlineSubsystem* InOnlineSubsystem);

	void Login(const FString& AuthToken);

	void Logout();

	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	void OnLogoutComplete(int32 LocalUserNum, bool bWasSuccessful);

private:
	IOnlineIdentityPtr Identity;
};
