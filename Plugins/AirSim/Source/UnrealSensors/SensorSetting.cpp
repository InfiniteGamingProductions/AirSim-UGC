#include "SensorSetting.h"

std::unique_ptr<AirSimSettings::SensorSetting> FSensorSetting::CreateAirLibSensorSetting() {

	//create a new sensor setting
	std::unique_ptr<AirSimSettings::SensorSetting> airLibSensorSetting = AirSimSettings::createSensorSetting(
		ESensorTypeToAirLibSensorType(SensorType), std::string(TCHAR_TO_UTF8(*SensorName)), Enabled);

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

msr::airlib::SensorBase::SensorType FSensorSetting::ESensorTypeToAirLibSensorType(const ESensorType InputSensorType)
{
	switch (InputSensorType) {
	case ESensorType::Barometer:
		return msr::airlib::SensorBase::SensorType::Barometer;
	case ESensorType::Imu:
		return msr::airlib::SensorBase::SensorType::Imu;
	case ESensorType::Gps:
		return msr::airlib::SensorBase::SensorType::Gps;
	case ESensorType::Magnetometer:
		return msr::airlib::SensorBase::SensorType::Magnetometer;
	case ESensorType::Distance_Sensor:
		return msr::airlib::SensorBase::SensorType::Distance;
	case ESensorType::Lidar:
		return msr::airlib::SensorBase::SensorType::Lidar;
	}

	return msr::airlib::SensorBase::SensorType::Barometer;
}