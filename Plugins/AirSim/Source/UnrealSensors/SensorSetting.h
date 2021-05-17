#pragma once

#include "CoreMinimal.h"
#include "common/AirSimSettings.hpp"
#include "SensorSetting.generated.h"

typedef struct msr::airlib::AirSimSettings AirSimSettings;

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
struct AIRSIM_API FSensorSetting
{
	GENERATED_BODY()

	/**
	* Called To Create an AirLib Sensor Setting with the FSensorSettings
	*/
	std::unique_ptr<AirSimSettings::SensorSetting> CreateAirLibSensorSetting();

	/**
	* Converts ESensorType to msr::airlib::SensorBase::SensorType
	*/
	msr::airlib::SensorBase::SensorType ESensorTypeToAirLibSensorType(const ESensorType InputSensorType);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Enabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SensorName = TEXT("MySensor");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESensorType SensorType;

	//The Positon and rotation of the sensor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition = "SensorType == ESensorType::Distance_Sensor || SensorType == ESensorType::Lidar"))
	FVector Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition = "SensorType == ESensorType::Distance_Sensor || SensorType == ESensorType::Lidar"))
	FRotator Rotation;

	/**
	* Should draw hit location points?
	* @warning - This can cause lag as it doesn't render through normal pipeline
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", Meta = (EditCondition = "SensorType == ESensorType::Distance_Sensor || SensorType == ESensorType::Lidar"))
	bool DrawDebugPoints = false;

	/* Distance Sensor Settings */

	//The Min Distance that the sensor can sense in meters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Sensor", Meta = (EditCondition = "SensorType == ESensorType::Distance_Sensor", DisplayName = "Min Distance"))
	float Distance_MinDistance = 0.2f;

	//The Max Distance that the sensor can sense in meters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Sensor", Meta = (EditCondition = "SensorType == ESensorType::Distance_Sensor", DisplayName = "Max Distance"))
	float Distance_MaxDistance = 40.0f;

	/* Lidar Sensor Settings */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar Sensor", Meta = (EditCondition = "SensorType == ESensorType::Lidar", DisplayName = "Number Of Channels"))
	int32 Lidar_NumberOfChannels = 16;
	//The Max Range of the Lidar in meters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar Sensor", Meta = (EditCondition = "SensorType == ESensorType::Lidar", DisplayName = "Range"))
	float Lidar_Range = 100.0f;
	//Note Must Be a positive number
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar Sensor", Meta = (EditCondition = "SensorType == ESensorType::Lidar", DisplayName = "Points Per Second"))
	int32 Lidar_PointsPerSecond = 100000;
	// rotations/sec
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar Sensor", Meta = (EditCondition = "SensorType == ESensorType::Lidar", DisplayName = "Horizontal Rotation Frequency"))
	int32 Lidar_HorizontalRotationFrequency = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar Sensor | FOV", Meta = (EditCondition = "SensorType == ESensorType::Lidar", DisplayName = "Horizontal FOV Start"))
	float Lidar_HorizontalFOVStart = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar Sensor | FOV", Meta = (EditCondition = "SensorType == ESensorType::Lidar", DisplayName = "Horizontal FOV End"))
	float Lidar_HorizontalFOVEnd = 359.0f;
	//Uppermost FOV in degrees (Default for Drones = -15, Default for Car = +10)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar Sensor | FOV", Meta = (EditCondition = "SensorType == ESensorType::Lidar", DisplayName = "Vertical FOV Upper"))
	float Lidar_VerticalFOVUpper = -15.0f;
	//Lower FOV in degrees (Default for Drones = -45, Default for Car = -10)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lidar Sensor | FOV", Meta = (EditCondition = "SensorType == ESensorType::Lidar", DisplayName = "Vertical FOV Lower"))
	float Lidar_VerticalFOVLower = -45.0f;
};