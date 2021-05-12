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
	* Called To Create or get the AirLib Sensor Setting associated with the FSensorSettings
	*/
	std::unique_ptr<AirSimSettings::SensorSetting> GetSensorSetting();

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

USTRUCT(BlueprintType)
struct FDistanceSensorSetting : public FSensorSetting {
	GENERATED_BODY()

	//The Positon and rotation of the sensor
	UPROPERTY(EditDefaultsOnly, Meta = (MakeEditWidget = true))
	FTransform Transform;

	//The Min Distance that the sensor can sense in meters
	UPROPERTY(EditDefaultsOnly)
	float MinDistance = 0.2f;
	//The Max Distance that the sensor can sense in meters
	UPROPERTY(EditDefaultsOnly)
	float MaxDistance = 40.0f;

	//Should DrawDebug Hit Locations
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool DrawDebugPoints = false;

};

USTRUCT(BlueprintType)
struct FLidarSensorSetting : public FSensorSetting {
	GENERATED_BODY()

	//The Positon and rotation of the sensor
	UPROPERTY(EditDefaultsOnly, Meta = (MakeEditWidget = true))
	FTransform Transform;

	UPROPERTY(EditDefaultsOnly)
	uint32 number_of_channels = 16;

	//The Max Range of the Lidar in meters
	UPROPERTY(EditDefaultsOnly)
	float range = 100.0f;

	UPROPERTY(EditDefaultsOnly)
	uint32 points_per_second = 100000;

	// rotations/sec
	UPROPERTY(EditDefaultsOnly)
	uint32 horizontal_rotation_frequency = 10;

	UPROPERTY(EditDefaultsOnly, Category = "FOV")
	float horizontal_FOV_start = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "FOV")
	float horizontal_FOV_end = 359.0f;
	//Uppermost FOV in degrees (Default for Drones = -15, Default for Car = +10)
	UPROPERTY(EditDefaultsOnly, Category = "FOV")
	float vertical_FOV_upper = -15.0f;
	//Lower FOV in degrees (Default for Drones = -45, Default for Car = -10)
	UPROPERTY(EditDefaultsOnly, Category = "FOV")
	float vertical_FOV_lower = -45.0f;

	//Should DrawDebug Hit Locations
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool DrawDebugPoints = false;

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

	//Determines if existing sensors should be removed before adding sensors below
	UPROPERTY(EditDefaultsOnly, Category = "Sensors")
	bool bOverwriteDefaultSensors = false;

	UPROPERTY(EditDefaultsOnly, Category = "Sensors")
	TArray<FSensorSetting> Sensors;

private:
	/**
	* Converts Vehicle Type Enum To an std::string
	*/
	std::string getVehicleTypeString(EVehicleType vehicleType);

	/**
	* Converts default vehicle state Type Enum To an std::string
	*/
	std::string getVehicleDefaultStateString(EDefaultVehicleState vehicleState);

	AirSimSettings::VehicleSetting* vehicleSettingReference;
};
