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
struct FSensorSetting {
	GENERATED_BODY()

	/**
	* Called To Create an AirLib Sensor Setting with the FSensorSettings
	*/
	std::unique_ptr<AirSimSettings::SensorSetting> CreateAirLibSensorSetting();

	/**
	* Converts ESensorType to msr::airlib::SensorBase::SensorType
	*/
	AirLibSensorType ESensorTypeToAirLibSensorType(const ESensorType InputSensorType);

	UPROPERTY(EditDefaultsOnly)
	bool Enabled = true;

	UPROPERTY(EditDefaultsOnly)
	FString SensorName = TEXT("MySensor");

	UPROPERTY(EditDefaultsOnly)
	ESensorType SensorType;

	//The Positon and rotation of the sensor
	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition="SensorType == ESensorType::Distance_Sensor || SensorType == ESensorType::Lidar", EditConditionHides))
	FVector Position;

	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "SensorType == ESensorType::Distance_Sensor || SensorType == ESensorType::Lidar", EditConditionHides))
	FRotator Rotation;

	/**
	* Should draw hit location points?
	* @warning - This can cause lag as it doesn't render through normal pipeline
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Debug", Meta = (EditCondition = "SensorType == ESensorType::Distance_Sensor || SensorType == ESensorType::Lidar", EditConditionHides))
	bool DrawDebugPoints = false;

	/* Distance Sensor Settings */
	
	//The Min Distance that the sensor can sense in meters
	UPROPERTY(EditDefaultsOnly, Category = "Distance Sensor", Meta = (EditCondition = "SensorType == ESensorType::Distance_Sensor", EditConditionHides, DisplayName = "Min Distance"))
	float Distance_MinDistance = 0.2f;

	//The Max Distance that the sensor can sense in meters
	UPROPERTY(EditDefaultsOnly, Category = "Distance Sensor", Meta = (EditCondition = "SensorType == ESensorType::Distance_Sensor", EditConditionHides, DisplayName = "Max Distance"))
	float Distance_MaxDistance = 40.0f;

	/* Lidar Sensor Settings */

	UPROPERTY(EditDefaultsOnly, Category = "Lidar Sensor", Meta = (EditCondition = "SensorType == ESensorType::Lidar", EditConditionHides, DisplayName = "Number Of Channels"))
	uint32 Lidar_NumberOfChannels = 16;
	//The Max Range of the Lidar in meters
	UPROPERTY(EditDefaultsOnly, Category = "Lidar Sensor", Meta = (EditCondition = "SensorType == ESensorType::Lidar", EditConditionHides, DisplayName = "Range"))
	float Lidar_Range = 100.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Lidar Sensor", Meta = (EditCondition = "SensorType == ESensorType::Lidar", EditConditionHides, DisplayName = "Points Per Second"))
	uint32 Lidar_PointsPerSecond = 100000;
	// rotations/sec
	UPROPERTY(EditDefaultsOnly, Category = "Lidar Sensor", Meta = (EditCondition = "SensorType == ESensorType::Lidar", EditConditionHides, DisplayName = "Horizontal Rotation Frequency"))
	uint32 Lidar_HorizontalRotationFrequency = 10;

	UPROPERTY(EditDefaultsOnly, Category = "Lidar Sensor | FOV", Meta = (EditCondition = "SensorType == ESensorType::Lidar", EditConditionHides, DisplayName = "Horizontal FOV Start"))
	float Lidar_HorizontalFOVStart = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Lidar Sensor | FOV", Meta = (EditCondition = "SensorType == ESensorType::Lidar", EditConditionHides, DisplayName = "Horizontal FOV End"))
	float Lidar_HorizontalFOVEnd = 359.0f;
	//Uppermost FOV in degrees (Default for Drones = -15, Default for Car = +10)
	UPROPERTY(EditDefaultsOnly, Category = "Lidar Sensor | FOV", Meta = (EditCondition = "SensorType == ESensorType::Lidar", EditConditionHides, DisplayName = "Vertical FOV Upper"))
	float Lidar_VerticalFOVUpper = -15.0f;
	//Lower FOV in degrees (Default for Drones = -45, Default for Car = -10)
	UPROPERTY(EditDefaultsOnly, Category = "Lidar Sensor | FOV", Meta = (EditCondition = "SensorType == ESensorType::Lidar", EditConditionHides, DisplayName = "Vertical FOV Lower"))
	float Lidar_VerticalFOVLower = -45.0f;
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
	bool bOverwriteDefaultSettings = true;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bOverwriteDefaultSettings"))
	EVehicleType VehicleType;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bOverwriteDefaultSettings"))
	EDefaultVehicleState DefaultVehicleState;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bOverwriteDefaultSettings"))
	bool bEnableCollisions = true;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bOverwriteDefaultSettings"))
	bool bEnableCollisionPassthrough = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bOverwriteDefaultSettings"))
	bool bEnableTrace = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bOverwriteDefaultSettings"))
	bool bAllowAPIAlways = true;

	//Determines if the Remote Control settings should override what is set in the AirSim Settings JSON
	UPROPERTY(EditDefaultsOnly, Category = "Remote Control")
	bool bOverwriteRemoteControlSettings = false;

	UPROPERTY(EditDefaultsOnly, Category = "Remote Control", meta = (EditCondition = "bOverwriteRemoteControlSettings"))
	int RemoteControlID = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Remote Control", meta = (EditCondition = "bOverwriteRemoteControlSettings"))
	bool bAllowAPIWhenDisconnected = false;

	//Determines if existing sensors should be removed before adding sensors
	UPROPERTY(EditDefaultsOnly, Category = "Sensors")
	bool bOverwriteDefaultSensors = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensors")
	TArray<FSensorSetting> Sensors;
private:
	/**
	* Converts Vehicle Type Enum To an std::string
	*/
	std::string getVehicleTypeAsString(EVehicleType vehicleType);

	/**
	* Converts default vehicle state Type Enum To an std::string
	*/
	std::string getVehicleDefaultStateAsString(EDefaultVehicleState vehicleState);

	AirSimSettings::VehicleSetting* vehicleSettingReference;
};
