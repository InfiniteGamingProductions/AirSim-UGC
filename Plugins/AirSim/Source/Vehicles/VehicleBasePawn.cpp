

#include "VehicleBasePawn.h"


#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"


// Sets default values
AVehicleBasePawn::AVehicleBasePawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called to bind functionality to input
void AVehicleBasePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Pause", IE_Pressed, this, &AVehicleBasePawn::OnPauseButtonClicked);
}

void AVehicleBasePawn::OnPauseButtonClicked()
{
	if (UGameplayStatics::IsGamePaused(this))
	{
		UnpauseGame();
	}
	else {
		PauseGame();
	}
}

void AVehicleBasePawn::PauseGame()
{
	if (!IsValid(PauseMenuReference))
	{
		PauseMenuReference = CreateWidget<UUserWidget>(GetWorld(), PauseMenuClass);
	}

	APlayerController* ctrl = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ctrl->bShowMouseCursor = true;
	ctrl->SetInputMode(FInputModeUIOnly());

	PauseMenuReference->AddToViewport();

	UGameplayStatics::SetGamePaused(this, true);
}

void AVehicleBasePawn::UnpauseGame()
{
	if (IsValid(PauseMenuReference))
	{
		APlayerController* ctrl = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		ctrl->bShowMouseCursor = false;
		ctrl->SetInputMode(FInputModeGameOnly());

		PauseMenuReference->RemoveFromViewport();

		UGameplayStatics::SetGamePaused(this, false);
	}
}

