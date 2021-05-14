#include "Vehicles/VehicleSettingsComponent.h"

// Sets default values for this component's properties
UVehicleSettingsComponent::UVehicleSettingsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	vehicleSettingReference = nullptr;
}

void UVehicleSettingsComponent::SetAirSimVehicleSettings(AirSimSettings::VehicleSetting* VehicleSetting)
{
	//Check if we are allowed to change the settings first
	if (bOverwriteDefaultSettings)
	{
		VehicleSetting->vehicle_type = getVehicleTypeAsString(VehicleType);
		VehicleSetting->default_vehicle_state = getVehicleDefaultStateAsString(DefaultVehicleState);

		VehicleSetting->enable_collisions = bEnableCollisions;
		VehicleSetting->enable_collision_passthrough = bEnableCollisionPassthrough;
		VehicleSetting->enable_trace = bEnableTrace;
		VehicleSetting->allow_api_always = bAllowAPIAlways;
	}

	if (bOverwriteRemoteControlSettings)
	{
		VehicleSetting->rc.remote_control_id = RemoteControlID;
		VehicleSetting->rc.allow_api_when_disconnected = bAllowAPIWhenDisconnected;
	}

	//setup sensors
	if (bOverwriteDefaultSensors)
	{
		VehicleSetting->sensors.clear();
	}

	//Add Sensors Specified
	for (FSensorSetting sensorSetting : Sensors)
	{
		//Get a AirLibSensor from the sensorSetting element
		std::unique_ptr<AirSimSettings::SensorSetting> AirLibSensor = sensorSetting.CreateAirLibSensorSetting();

		//Add Sensor Setting to the vehicle Settings
		VehicleSetting->sensors[AirLibSensor->sensor_name] = std::move(AirLibSensor);
	}

	//We dont need to setup cameras here because they are already setup in the pawn
	//See PawnSimApi::setupCamerasFromSettings to see why

	//Save the reference to the vehicle setting
	vehicleSettingReference = VehicleSetting;
}

AirSimSettings::VehicleSetting* UVehicleSettingsComponent::GetAirSimVehicleSetting()
{
	return vehicleSettingReference;
}

std::string UVehicleSettingsComponent::getVehicleTypeAsString(EVehicleType vehicleType)
{
	switch (vehicleType)
	{
	case EVehicleType::PX4:
		return AirSimSettings::kVehicleTypePX4;
		break;
	case EVehicleType::ArduCopterSolo:
		return AirSimSettings::kVehicleTypeArduCopterSolo;
		break;
	case EVehicleType::ArduCopter:
		return AirSimSettings::kVehicleTypeArduCopter;
		break;
	case EVehicleType::SimpleFlight:
		return AirSimSettings::kVehicleTypeSimpleFlight;
		break;
	case EVehicleType::PhysXCar:
		return AirSimSettings::kVehicleTypePhysXCar;
		break;
	case EVehicleType::ArduRover:
		return AirSimSettings::kVehicleTypeArduRover;
		break;
	case EVehicleType::ComputerVision:
		return AirSimSettings::kVehicleTypeComputerVision;
		break;
	default:
		return AirSimSettings::kVehicleTypeComputerVision;
		break;
	}
}

std::string UVehicleSettingsComponent::getVehicleDefaultStateAsString(EDefaultVehicleState vehicleState)
{
	switch (vehicleState)
	{
	case EDefaultVehicleState::Armed:
		return "Armed";
		break;
	case EDefaultVehicleState::Disarmed:
		return "Disarmed";
		break;
	default:
		return "Disarmed";
		break;
	}
}

