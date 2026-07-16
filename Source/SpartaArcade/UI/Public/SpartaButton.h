#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "SpartaButton.generated.h"


UCLASS()
class SPARTAARCADE_API USpartaButton : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = true))
    FText ButtonText;

    UPROPERTY(BlueprintAssignable, Category = "UI | Events")
    FOnButtonClickedEvent OnClicked;

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetButtonText(const FText& InText);

    // 버튼의 배경 색상을 설정할 수 있는 편의 함수 추가
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetButtonColor(const FLinearColor& InColor);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;

    UFUNCTION()
    void HandleInternalButtonClicked();

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* Text; 

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* Button;
};
