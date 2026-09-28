#include "MyUIManagerSubsystem.h"
#include "MenuRootWidget.h"

void UMyUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("MyUIManagerSubsystem initialized"));
}

void UMyUIManagerSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("MyUIManagerSubsystem deinitialized"));

	Super::Deinitialize();
}

void UMyUIManagerSubsystem::InitializeUI(
	APlayerController* PlayerController,
	TSubclassOf<UUserWidget> InMenuRootClass
)
{
	if (!PlayerController)
	{
		return;
	}

	if (!InMenuRootClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MenuRootClass was not provided!"));
		return;
	}

	CachedPlayerController = PlayerController;

	if (MenuRoot)
	{
		return;
	}

	MenuRoot = CreateWidget<UUserWidget>(
		PlayerController,
		InMenuRootClass
	);

	if (MenuRoot)
	{
		MenuRoot->AddToViewport();

		UE_LOG(LogTemp, Log, TEXT("MenuRoot created and added to viewport"));
	}
}

void UMyUIManagerSubsystem::ShowMainMenu()
{
	if (UMenuRootWidget* RootWidget = Cast<UMenuRootWidget>(MenuRoot))
	{
		RootWidget->ShowMainMenu();
	}
}

void UMyUIManagerSubsystem::ShowSessionMenu()
{
	if (UMenuRootWidget* RootWidget = Cast<UMenuRootWidget>(MenuRoot))
	{
		RootWidget->ShowSessionMenu();
	}
}

void UMyUIManagerSubsystem::ShowLobby()
{
	if (UMenuRootWidget* RootWidget = Cast<UMenuRootWidget>(MenuRoot))
	{
		RootWidget->ShowLobby();
	}
}

void UMyUIManagerSubsystem::ShowSettings()
{
	if (UMenuRootWidget* RootWidget = Cast<UMenuRootWidget>(MenuRoot))
	{
		RootWidget->ShowSettings();
	}
}