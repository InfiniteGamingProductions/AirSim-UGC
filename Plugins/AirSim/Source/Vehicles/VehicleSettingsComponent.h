// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehicleSettingsComponent.generated.h"

UENUM(BlueprintType)
enum class ESimModeType : uint8 {
	Multirotor,
	Car,
	ComputerVision
};

UENUM(BlueprintType)
enum class EVehicleType : uint8 {
	PX4,
	ArduCopterSolo,
	ArduCopter,
	SimpleFlight,
	PhysXCar,
	ArduRover,
	ComputerVision
};

UCLASS()
class AIRSIM_API UVehicleSettingsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVehicleSettingsComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called to set AirSimVehicleSettings from component properties
	void SetAirSimVehicleSettings();

	UPROPERTY(EditDefaultsOnly)
	ESimModeType SimModeType;

	UPROPERTY(EditDefaultsOnly)
	EVehicleType VehicleType;

		
};
