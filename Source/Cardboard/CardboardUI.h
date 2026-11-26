#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"

// Lets a C++ widget base accept either a plain Button or a button Blueprint (WBP_Button) under the
// same BindWidgetOptional name: for a user widget it returns the first Button inside it.
// Binding to that inner Button keeps the Blueprint's own click handling (e.g. its sound) working too.
inline UButton* FindClickableButton(UWidget* Widget)
{
	if (UButton* Button = Cast<UButton>(Widget))
	{
		return Button;
	}

	UUserWidget* UserWidget = Cast<UUserWidget>(Widget);

	if (!UserWidget || !UserWidget->WidgetTree)
	{
		return nullptr;
	}

	UButton* Found = nullptr;

	UserWidget->WidgetTree->ForEachWidget([&Found](UWidget* Child)
	{
		if (!Found)
		{
			Found = Cast<UButton>(Child);
		}
	});

	return Found;
}
