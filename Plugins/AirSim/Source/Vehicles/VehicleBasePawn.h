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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called when the game ends or is killed
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

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
