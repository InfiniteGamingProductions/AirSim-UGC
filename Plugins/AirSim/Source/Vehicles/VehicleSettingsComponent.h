#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "common/AirSimSettings.hpp"
#include "VehicleSettingsComponent.generated.h"

typedef struct msr::airlib::AirSimSettings AirSimSettings;
typedef msr::airlib::SensorBase::SensorType AirLibSensorType;

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

	FSensor();

	/**
	* Called to set AirSimSensorSettings from FSensor values
	* @param OutSensorSetting - The sensor setting that gets set by the function
	*/
	void SetAirSimSensorSetting(AirSimSettings::SensorSetting& OutSensorSetting);

	/**
	* Returns the AirLib Sensor Setting Refrence. Keep in mind it is nullptr by default
	*/
	AirSimSettings::SensorSetting* GetAirSimSensorSetting();

	UPROPERTY(EditDefaultsOnly)
	bool Enabled = true;

	UPROPERTY(EditDefaultsOnly)
	FString SensorName = TEXT("MySensor");

	UPROPERTY(EditDefaultsOnly)
	ESensorType SensorType;

private:
	AirLibSensorType ESensorTypeToAirLibSensorType(const ESensorType InputSensorType);

	AirSimSettings::SensorSetting* SensorSettingReference;
};

UCLASS()
class AIRSIM_API UVehicleSettingsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVehicleSettingsComponent();

	/**
	* Called to set AirSimVehicleSettings from component properties
	* @param OutVehicleSetting - The vehicle setting that gets set by the function
	* @return Was Sucessfull?
	*/
	void SetAirSimVehicleSettings(AirSimSettings::VehicleSetting* VehicleSetting);

	/**
	* Returns the AirLib Vehicle Setting Refrence. Keep in mind it is nullptr by default
	*/
	AirSimSettings::VehicleSetting* GetAirSimVehicleSetting();

	UPROPERTY(EditDefaultsOnly)
	bool OverwriteDefaultSettings = true;

	UPROPERTY(EditDefaultsOnly)
	EVehicleType VehicleType;

	UPROPERTY(EditDefaultsOnly)
	EDefaultVehicleState DefaultVehicleState;

	UPROPERTY(EditDefaultsOnly, Category = "Collisions")
	bool bEnableCollisions = true;

	UPROPERTY(EditDefaultsOnly, Category = "Collisions")
	bool bEnableCollisionPassthrough = false;

	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bEnableTrace = false;

	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bAllowAPIAlways = true;

	//Determines if the Remote Control settings should override what is set in the AirSim Settings JSON
	UPROPERTY(EditDefaultsOnly, Category = "Remote Control")
	bool bOverwriteRemoteControlSettings = false;

	UPROPERTY(EditDefaultsOnly, Category = "Remote Control")
	int RemoteControlID = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Remote Control")
	bool bAllowAPIWhenDisconnected = false;

	//Determines if existing sensors should be removed
	UPROPERTY(EditDefaultsOnly, Category = "Sensors")
	bool bOverwriteDefaultSensors = false;

	UPROPERTY(EditDefaultsOnly, Category = "Sensors")
	TArray<FSensor> Sensors;

private:
	AirSimSettings::VehicleSetting* vehicleSettingReference;
};
