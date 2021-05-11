#include "Vehicles/VehicleSettingsComponent.h"

#pragma region FSensor
FSensor::FSensor()
{
	SensorSettingReference = nullptr;
}

void FSensor::SetAirSimSensorSetting(AirSimSettings::SensorSetting& OutSensorSetting)
{
	SensorSettingReference = &OutSensorSetting;
	OutSensorSetting.sensor_type = ESensorTypeToAirLibSensorType(SensorType);
	OutSensorSetting.enabled = Enabled;
	OutSensorSetting.sensor_name = std::string(TCHAR_TO_UTF8(*SensorName));
}

AirSimSettings::SensorSetting* FSensor::GetAirSimSensorSetting() 
{
	return SensorSettingReference;
}

AirLibSensorType FSensor::ESensorTypeToAirLibSensorType(const ESensorType InputSensorType)
{
	switch (InputSensorType) {
	case ESensorType::Barometer:
		return AirLibSensorType::Barometer;
	case ESensorType::Imu:
		return AirLibSensorType::Imu;
	case ESensorType::Gps:
		return AirLibSensorType::Gps;
	case ESensorType::Magnetometer:
		return AirLibSensorType::Magnetometer;
	case ESensorType::Distance_Sensor:
		return AirLibSensorType::Distance;
	case ESensorType::Lidar:
		return AirLibSensorType::Lidar;
	}

	return AirLibSensorType::Barometer;
}
#pragma endregion FSensor

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
	if (OverwriteDefaultSettings)
	{
		VehicleSetting->vehicle_type = std::string(TCHAR_TO_UTF8(*UEnum::GetValueAsString<EVehicleType>(VehicleType)));
		VehicleSetting->default_vehicle_state = std::string(TCHAR_TO_UTF8(*UEnum::GetValueAsString<EDefaultVehicleState>(DefaultVehicleState)));

		VehicleSetting->enable_collisions = bEnableCollisions;
		VehicleSetting->enable_collision_passthrough = bEnableCollisionPassthrough;
		VehicleSetting->enable_trace = bEnableTrace;
		VehicleSetting->allow_api_always = bAllowAPIAlways;

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

		for (FSensor sensorSettings : Sensors)
		{
			std::unique_ptr<AirSimSettings::SensorSetting> newSensor = std::unique_ptr<AirSimSettings::SensorSetting>(new AirSimSettings::SensorSetting());

			sensorSettings.SetAirSimSensorSetting(*newSensor);

			VehicleSetting->sensors[newSensor->sensor_name] = std::move(newSensor);
		}
		

		//We dont need to setup cameras here because they are already setup in the pawn
		//See PawnSimApi::setupCamerasFromSettings to see more
	}

	//Save the reference to the vehicle setting
	vehicleSettingReference = VehicleSetting;
}

AirSimSettings::VehicleSetting* UVehicleSettingsComponent::GetAirSimVehicleSetting()
{
	return vehicleSettingReference;
}

