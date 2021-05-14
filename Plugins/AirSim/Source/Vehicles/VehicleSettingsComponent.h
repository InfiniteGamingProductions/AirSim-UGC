#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "common/AirSimSettings.hpp"
#include "UnrealSensors/SensorSetting.h"
#include "VehicleSettingsComponent.generated.h"

typedef struct msr::airlib::AirSimSettings AirSimSettings;

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

	UPROPERTY(EditDefaultsOnly, Category = "Sensors")
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