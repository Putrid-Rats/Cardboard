// Fill out your copyright notice in the Description page of Project Settings.

#include "MenuRootWidget.h"

#include "SettingsWidget.h"

void UMenuRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Back in the settings panel returns to the main menu (AddUnique: construct can run more than once).
	if (USettingsWidget* SettingsWidget = Cast<USettingsWidget>(Settings))
	{
		SettingsWidget->OnClosed.AddUniqueDynamic(this, &UMenuRootWidget::HandleSettingsClosed);
	}

	ShowMainMenu();
}

void UMenuRootWidget::SetOnlyVisible(UUserWidget* WidgetToShow)
{
	UE_LOG(LogTemp, Warning, TEXT("SetOnlyVisible called"));
	
	if (MainMenu)
	{
		MainMenu->SetVisibility(
			MainMenu == WidgetToShow
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	}

	if (SessionMenu)
	{
		SessionMenu->SetVisibility(
			SessionMenu == WidgetToShow
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	}

	if (HostMenu)
	{
		HostMenu->SetVisibility(
			HostMenu == WidgetToShow
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	}

	if (Lobby)
	{
		Lobby->SetVisibility(
			Lobby == WidgetToShow
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	}

	if (Settings)
	{
		Settings->SetVisibility(
			Settings == WidgetToShow
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	}
}

void UMenuRootWidget::ShowMainMenu()
{
	UE_LOG(LogTemp, Warning, TEXT("ShowMainMenu called. MainMenu widget = %s"),
		MainMenu ? TEXT("VALID") : TEXT("NULL"));

	SetOnlyVisible(MainMenu);
}

void UMenuRootWidget::ShowSessionMenu()
{
	UE_LOG(LogTemp, Warning, TEXT("ShowSessionMenu called"));
	SetOnlyVisible(SessionMenu);
}

void UMenuRootWidget::ShowHostMenu()
{
	SetOnlyVisible(HostMenu);
}

void UMenuRootWidget::ShowLobby()
{
	UE_LOG(LogTemp, Warning, TEXT("ShowLobby called. Lobby widget = %s"),
		Lobby ? TEXT("VALID") : TEXT("NULL"));

	SetOnlyVisible(Lobby);
}

void UMenuRootWidget::ShowSettings()
{
	// Show the saved values, not whatever was left on the sliders after a Back without Apply.
	if (USettingsWidget* SettingsWidget = Cast<USettingsWidget>(Settings))
	{
		SettingsWidget->RefreshFromSettings();
	}

	SetOnlyVisible(Settings);
}

void UMenuRootWidget::HandleSettingsClosed()
{
	ShowMainMenu();
}