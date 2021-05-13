#include "Vehicles/VehicleSettingsComponent.h"

#pragma region FSensorSetting
std::unique_ptr<AirSimSettings::SensorSetting> FSensorSetting::GetSensorSetting() {
	//return a AirLibSensor ptr
	return AirSimSettings::createSensorSetting(
		ESensorTypeToAirLibSensorType(SensorType),
		std::string(TCHAR_TO_UTF8(*SensorName)),
		Enabled
	);
}

AirLibSensorType FSensorSetting::ESensorTypeToAirLibSensorType(const ESensorType InputSensorType)
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
#pragma endregion FSensorSetting

#pragma region FDistanceSensorSetting
FDistanceSensorSetting::FDistanceSensorSetting()
{
	SensorType = ESensorType::Distance_Sensor;
}

void FDistanceSensorSetting::SetSensorSettings(AirSimSettings::SensorSetting* Sensor)
{
	AirSimSettings::DistanceSetting* airLibSetting = static_cast<AirSimSettings::DistanceSetting*>(Sensor);
	if (airLibSetting)
	{
		airLibSetting->min_distance = MinDistance;
		airLibSetting->max_distance = MaxDistance;

		//set location and rotation values
		FVector pos = Transform.GetLocation();
		airLibSetting->position = msr::airlib::Vector3r(pos.X, pos.Y, pos.Z);
		FRotator rot = Transform.GetRotation().Rotator();
		airLibSetting->rotation = msr::airlib::AirSimSettings::Rotation(rot.Yaw, rot.Pitch, rot.Roll);

		airLibSetting->draw_debug_points = DrawDebugPoints;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot set sensor settings for sensor that is not of type DistanceSenor"));
	}
}
#pragma endregion FDistanceSensorSetting

#pragma region FLidarSensorSetting
FLidarSensorSetting::FLidarSensorSetting()
{
	SensorType = ESensorType::Lidar;
}

void FLidarSensorSetting::SetSensorSettings(AirSimSettings::SensorSetting* Sensor)
{
	AirSimSettings::LidarSetting* airLibSetting = static_cast<AirSimSettings::LidarSetting*>(Sensor);
	if (airLibSetting)
	{
		//set location and rotation values
		FVector pos = Transform.GetLocation();
		airLibSetting->position = msr::airlib::Vector3r(pos.X, pos.Y, pos.Z);
		FRotator rot = Transform.GetRotation().Rotator();
		airLibSetting->rotation = msr::airlib::AirSimSettings::Rotation(rot.Yaw, rot.Pitch, rot.Roll);

		//Set Sensor Settings
		airLibSetting->number_of_channels = NumberOfChannels;
		airLibSetting->range = Range;
		airLibSetting->points_per_second = PointsPerSecond;
		airLibSetting->horizontal_rotation_frequency = HorizontalRotationFrequency;
		airLibSetting->horizontal_FOV_start = HorizontalFOVStart;
		airLibSetting->horizontal_FOV_end = HorizontalFOVEnd;
		airLibSetting->vertical_FOV_upper = VerticalFOVUpper;
		airLibSetting->vertical_FOV_lower = VerticalFOVLower;

		airLibSetting->draw_debug_points = DrawDebugPoints;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot set sensor settings for sensor that is not of type LidarSenor"));
	}

}
#pragma endregion FLidarSensorSetting

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
		VehicleSetting->vehicle_type = getVehicleTypeString(VehicleType);
		VehicleSetting->default_vehicle_state = getVehicleDefaultStateString(DefaultVehicleState);

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

		//TODO:: Rework this to use base sensor array for all sensors instead of having multiple arrays for each sensor type
		//Add the base sensors
		for (FSensorSetting sensorSetting : BaseSensors)
		{
			//Get a AirLibSensor from the sensorSetting element
			std::unique_ptr<AirSimSettings::SensorSetting> AirLibSensor = sensorSetting.GetSensorSetting();

			//Add Sensor Setting to the vehicle Settings
			VehicleSetting->sensors[AirLibSensor->sensor_name] = std::move(AirLibSensor);
		}
		//Add DistanceSensors
		for (FDistanceSensorSetting sensorSetting : DistanceSensors)
		{
			std::unique_ptr<AirSimSettings::SensorSetting> AirLibSensor = sensorSetting.GetSensorSetting();
			sensorSetting.SetSensorSettings(AirLibSensor.get());

			//Add Sensor Setting to the vehicle Settings
			VehicleSetting->sensors[AirLibSensor->sensor_name] = std::move(AirLibSensor);
		}
		//Add LidarSensors
		for (FLidarSensorSetting sensorSetting : LidarSensors)
		{
			std::unique_ptr<AirSimSettings::SensorSetting> AirLibSensor = sensorSetting.GetSensorSetting();
			sensorSetting.SetSensorSettings(AirLibSensor.get());

			//Add Sensor Setting to the vehicle Settings
			VehicleSetting->sensors[AirLibSensor->sensor_name] = std::move(AirLibSensor);
		}

		//We dont need to setup cameras here because they are already setup in the pawn
		//See PawnSimApi::setupCamerasFromSettings to see why
	}

	//Save the reference to the vehicle setting
	vehicleSettingReference = VehicleSetting;
}

AirSimSettings::VehicleSetting* UVehicleSettingsComponent::GetAirSimVehicleSetting()
{
	return vehicleSettingReference;
}

std::string UVehicleSettingsComponent::getVehicleTypeString(EVehicleType vehicleType)
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

std::string UVehicleSettingsComponent::getVehicleDefaultStateString(EDefaultVehicleState vehicleState)
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

