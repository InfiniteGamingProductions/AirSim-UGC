#include "Vehicles/VehicleSettingsComponent.h"

#pragma region FSensorSetting
std::unique_ptr<AirSimSettings::SensorSetting> FSensorSetting::CreateAirLibSensorSetting() {

	//create a new sensor setting
	std::unique_ptr<AirSimSettings::SensorSetting> airLibSensorSetting = AirSimSettings::createSensorSetting(ESensorTypeToAirLibSensorType(SensorType), std::string(TCHAR_TO_UTF8(*SensorName)), Enabled);

	//Set settings spific to sensor types
	switch (SensorType) {
	case ESensorType::Barometer:
		break;
	case ESensorType::Imu:
		break;
	case ESensorType::Gps:
		break;
	case ESensorType::Magnetometer:
		break;
	case ESensorType::Distance_Sensor:
	{
		AirSimSettings::DistanceSetting* distanceSetting = static_cast<AirSimSettings::DistanceSetting*>(airLibSensorSetting.get());
		if (distanceSetting)
		{
			//set location and rotation values
			distanceSetting->position = msr::airlib::Vector3r(Position.X, Position.Y, Position.Z);
			distanceSetting->rotation = msr::airlib::AirSimSettings::Rotation(Rotation.Yaw, Rotation.Pitch, Rotation.Roll);

			distanceSetting->draw_debug_points = DrawDebugPoints;

			distanceSetting->min_distance = Distance_MinDistance;
			distanceSetting->max_distance = Distance_MaxDistance;
		}

		break;
	}
	case ESensorType::Lidar:
	{
		AirSimSettings::LidarSetting* lidarSetting = static_cast<AirSimSettings::LidarSetting*>(airLibSensorSetting.get());
		if (lidarSetting)
		{
			//set location and rotation values
			lidarSetting->position = msr::airlib::Vector3r(Position.X, Position.Y, Position.Z);
			lidarSetting->rotation = msr::airlib::AirSimSettings::Rotation(Rotation.Yaw, Rotation.Pitch, Rotation.Roll);

			lidarSetting->draw_debug_points = DrawDebugPoints;

			lidarSetting->number_of_channels = Lidar_NumberOfChannels;
			lidarSetting->range = Lidar_Range;
			lidarSetting->points_per_second = Lidar_PointsPerSecond;
			lidarSetting->horizontal_rotation_frequency = Lidar_HorizontalRotationFrequency;
			lidarSetting->horizontal_FOV_start = Lidar_HorizontalFOVStart;
			lidarSetting->horizontal_FOV_end = Lidar_HorizontalFOVEnd;
			lidarSetting->vertical_FOV_upper = Lidar_VerticalFOVUpper;
			lidarSetting->vertical_FOV_lower = Lidar_VerticalFOVLower;
		}

		break;
	}
	default:
		break;
	}

	return airLibSensorSetting;
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
		VehicleSetting->vehicle_type = getVehicleTypeString(VehicleType);
		VehicleSetting->default_vehicle_state = getVehicleDefaultStateString(DefaultVehicleState);

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

