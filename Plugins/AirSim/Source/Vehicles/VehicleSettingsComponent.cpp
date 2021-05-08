#include "Vehicles/VehicleSettingsComponent.h"

#pragma region FSensor
FSensor::FSensor(AirSimSettings::SensorSetting* AirLibSensorSettingRef)
{
	SensorSettingReference = AirLibSensorSettingRef;
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

	// ...
}

// Called when the game starts
void UVehicleSettingsComponent::BeginPlay()
{
	Super::BeginPlay();

	// Can Probrobly Remove this Function
}

bool UVehicleSettingsComponent::SetAirSimVehicleSettings(AirSimSettings::VehicleSetting& OutVehicleSetting)
{
	//TODO: This
	return false;
}

