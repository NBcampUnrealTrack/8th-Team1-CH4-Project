#include "SpartaButton.h"

void USpartaButton::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (Text)
    {
        Text->SetText(ButtonText);
    }
}

void USpartaButton::NativeConstruct()
{
    Super::NativeConstruct();

    if (Button)
    {
        Button->OnClicked.AddDynamic(this, &USpartaButton::HandleInternalButtonClicked);
    }
}

void USpartaButton::HandleInternalButtonClicked()
{
    OnClicked.Broadcast();
}

void USpartaButton::SetButtonText(const FText& InText)
{
    ButtonText = InText;
    if (Text)
    {
        Text->SetText(InText);
    }
}
