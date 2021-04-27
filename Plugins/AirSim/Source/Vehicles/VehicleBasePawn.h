// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"

#include "VehicleBasePawn.generated.h"

UCLASS()
class AIRSIM_API AVehicleBasePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AVehicleBasePawn();

public:

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void OnPauseButtonClicked();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "User Interface")
	TSubclassOf<UUserWidget> PauseMenuClass;

private:
	UPROPERTY()
	UUserWidget* PauseMenuReference;

	UFUNCTION(BlueprintCallable, Category = "Pause", meta = (AllowPrivateAccess = "true"))
	void PauseGame();

	UFUNCTION(BlueprintCallable, Category = "Pause", meta = (AllowPrivateAccess = "true"))
	void UnpauseGame();
};
