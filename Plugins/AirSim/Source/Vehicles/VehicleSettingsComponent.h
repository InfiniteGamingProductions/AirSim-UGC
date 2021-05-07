// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "common/AirSimSettings.hpp"
#include "VehicleSettingsComponent.generated.h"

typedef struct msr::airlib::AirSimSettings AirSimSettings;

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

UENUM(BlueprintType)
enum class EDefaultVehicleState : uint8 {
	Armed,
	Disarmed
};

UENUM(BlueprintType)
enum class ESensorType : uint8 {
	Camera,
	Barometer,
	Imu,
	Gps,
	Magnetometer,
	Distance_Sensor,
	Lidar
};

//Uses sensor spific settings from default sensors specified in AirSim Settings
USTRUCT(BlueprintType)
struct FSensor {
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	ESensorType SensorType;

	UPROPERTY(EditDefaultsOnly)
	bool Enabled = true;

	/**
	* Called to set AirSimSensorSettings from FSensor values
	* @param OutSensorSetting - The sensor setting that gets set by the function
	* @return Was Sucessfull?
	*/
	bool SetAirSimSensorSetting(AirSimSettings::SensorSetting& OutSensorSetting);
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
	/**
	* Called to set AirSimVehicleSettings from component properties
	* @param OutVehicleSetting - The vehicle setting that gets set by the function
	* @return Was Sucessfull?
	*/
	bool SetAirSimVehicleSettings(AirSimSettings::VehicleSetting& OutVehicleSetting);

	UPROPERTY(EditDefaultsOnly)
	ESimModeType SimModeType;

	UPROPERTY(EditDefaultsOnly)
	EVehicleType VehicleType;

	UPROPERTY(EditDefaultsOnly)
	EDefaultVehicleState DefaultVehicleState;

	UPROPERTY(EditDefaultsOnly)
	TArray<FSensor> Sensors;

		
};
