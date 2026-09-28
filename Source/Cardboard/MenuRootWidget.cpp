// Fill out your copyright notice in the Description page of Project Settings.

#include "MenuRootWidget.h"

void UMenuRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

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
	SetOnlyVisible(MainMenu);
}

void UMenuRootWidget::ShowSessionMenu()
{
	UE_LOG(LogTemp, Warning, TEXT("ShowSessionMenu called"));
	SetOnlyVisible(SessionMenu);
}

void UMenuRootWidget::ShowLobby()
{
	SetOnlyVisible(Lobby);
}

void UMenuRootWidget::ShowSettings()
{
	SetOnlyVisible(Settings);
}