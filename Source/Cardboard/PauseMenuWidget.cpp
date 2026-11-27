#include "PauseMenuWidget.h"

#include "CardboardUI.h"
#include "MySessionSubsystem.h"
#include "SeatedPawn.h"
#include "SettingsWidget.h"

void UPauseMenuWidget::Init(ASeatedPawn* InOwnerPawn)
{
	OwnerPawn = InOwnerPawn;
}

void UPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UButton* Button = FindClickableButton(ResumeButton))
	{
		Button->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleResumeClicked);
	}

	if (UButton* Button = FindClickableButton(SettingsButton))
	{
		Button->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleSettingsClicked);
	}

	if (UButton* Button = FindClickableButton(LeaveButton))
	{
		Button->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleLeaveClicked);
	}

	if (SettingsPanel)
	{
		SettingsPanel->OnClosed.AddUniqueDynamic(this, &UPauseMenuWidget::HandleSettingsClosed);
		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPauseMenuWidget::HandleResumeClicked()
{
	if (ASeatedPawn* Pawn = OwnerPawn.Get())
	{
		Pawn->ClosePauseMenu();
	}
}

void UPauseMenuWidget::HandleSettingsClicked()
{
	if (!SettingsPanel)
	{
		return;
	}

	SettingsPanel->RefreshFromSettings();
	SettingsPanel->SetVisibility(ESlateVisibility::Visible);

	if (MenuPanel)
	{
		MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPauseMenuWidget::HandleSettingsClosed()
{
	CloseSettingsPanel();
}

bool UPauseMenuWidget::CloseSettingsPanel()
{
	if (!SettingsPanel || SettingsPanel->GetVisibility() == ESlateVisibility::Collapsed)
	{
		return false;
	}

	SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);

	if (MenuPanel)
	{
		MenuPanel->SetVisibility(ESlateVisibility::Visible);
	}

	return true;
}

void UPauseMenuWidget::HandleLeaveClicked()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMySessionSubsystem* Sessions = GameInstance->GetSubsystem<UMySessionSubsystem>())
		{
			Sessions->LeaveToMainMenu();
		}
	}
}
