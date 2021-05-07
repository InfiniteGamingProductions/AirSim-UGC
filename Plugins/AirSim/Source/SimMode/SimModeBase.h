#pragma once

#include "CoreMinimal.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "GameFramework/Actor.h"
#include "ParticleDefinitions.h"

#include <string>
#include "CameraDirector.h"
#include "common/AirSimSettings.hpp"
#include "common/ClockFactory.hpp"
#include "api/ApiServerBase.hpp"
#include "api/ApiProvider.hpp"
#include "PawnSimApi.h"
#include "common/StateReporterWrapper.hpp"
#include "SimModeBase.generated.h"

UCLASS()
class AIRSIM_API ASimModeBase : public AActor
{

    GENERATED_BODY()

public:

	ASimModeBase();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Resets the SimMode */
    UFUNCTION(BlueprintCallable, Category = "Airsim")
    virtual void Reset();

protected:
	typedef msr::airlib::AirSimSettings AirSimSettings;
	typedef common_utils::Utils Utils;
	typedef msr::airlib::SensorBase::SensorType SensorType;
	typedef msr::airlib::Vector3r Vector3r;
	typedef msr::airlib::Pose Pose;
	typedef msr::airlib::VectorMath VectorMath;

#pragma region Vehicles
public:
	/**
	* Spawn A new vehicle at runtime
	* @param vehicle_name - The Name of the vehicle actor
	* @param vehicle_type - 
	* @param pose - The spawn location and rotation
	* @param pawn_path - The path to the pawn blueprint that you want to spawn
	* @return Was Sucessfull?
	*/
	bool SpawnVehicleAtRuntime(const std::string& vehicle_name, const std::string& vehicle_type, const msr::airlib::Pose& pose, const std::string& pawn_path = "");

protected:
	
	/**
	* Get the vehicles that have already been spawned
	* @param OutPawns - the vehicle pawns that were found
	* @note This is a slow operation, use with caution
	* @note Override Required
	*/
	virtual void GetExistingVehiclePawns(TArray<APawn*>& OutPawns) const;

	/**
	* Check if the vehicle type is supported by the simmode
	* @return Returns true if the vehicle type is supported
	* @note Override Required
	*/
	virtual bool IsVehicleTypeSupported(const std::string& vehicle_type) const;

	/**
	* Returns the pawn path set in the AirSimSettings
	* @note Override Required
	*/
	virtual std::string GetVehiclePawnPath(const AirSimSettings::VehicleSetting& vehicle_setting) const;

	/**
	* Returns the PawnEvents object asociated with a vehicle pawn
	* @note Override Required
	*/
	virtual PawnEvents* GetVehiclePawnEvents(APawn* pawn) const;

	/**
	* Get the cameras from a vehicle pawn
	* @note Override Required
	*/
	virtual const common_utils::UniqueValueMap<std::string, APIPCamera*> GetVehiclePawnCameras(APawn* pawn) const;

	/**
	* Called To Register a new vechile with custom physics engine
	* @note Override Required, Unless not using custom physics engine
	*/
	virtual void RegisterPhysicsBody(msr::airlib::VehicleSimApiBase* physicsBody);

	/**
	* Setup a new vehicle pawn. Is called when the new vehicle is Spawned
	* @note Base implemintation doesn't do anything
	* @deprecated Should Use BeginPlay to do setup like this
	*/
	virtual void InitializeVehiclePawn(APawn* pawn);

	/**
	* Spawns a new vehicle pawn using vehicle settings to setup paramaters
	* @returns A pointer to the newly spawned vehicle
	* @note Override Optional
	*/
	virtual APawn* SpawnVehiclePawn(const AirSimSettings::VehicleSetting& vehicle_setting, const FVector StartLocation, const FRotator StartRotation);

	/**
	* Spawns Vehicles from AirSim Settings and Initilizes Camera Director. Is called durring BeginPlay()
	* @note Override Optional
	*/
	virtual void SetupVehiclesAndCamera();

	/**
	* Checks if a vehicle is available to use
	* @note Throws Error if vehicle not available
	*/
	void CheckVehicleReady();

#pragma endregion Vehicles

public:
	/**
	* Used to set the Wind Direction and Magnitude
	* @note Override Required
	*/
	virtual void SetWind(const msr::airlib::Vector3r& wind) const;

#pragma region Pause Functions
public:
	/**
	* Called to determine if the simulation is paused or not 
	* @return If the simulation is paused or not
	* @note Override Optional
	*/
	virtual bool IsSimulationPaused() const;

	/**
	* Called to pause or unpause the simulation. Will handle calling UGameplayStatics::SetGamePaused as needed.
	* @param Pause - true if you want to pause, false if you want to resume
	* @note Override Optional
	*/
	virtual void PauseSimulation(bool Pause);

	/**
	* Called by WorldSimAPI to unpause for seconds and then pause again
	* @param Seconds - The time in seconds to continue
	* @warning Depending on implementation this halts the game thread!
	* @note Override Required
	*/
	virtual void ContinueForTime(double Seconds);

	/**
	* Called by WorldSimAPI to unpause for a number of frames and then pause again
	* @param Frames - The number of Frames to continue
	* @warning Depending on implementation this halts the game thread!
	* @note Override Required
	*/
	virtual void ContinueForFrames(uint32_t Frames);
#pragma endregion Pause Functions

#pragma region Recording Functions
public:
	/**
	* Toggles frame recording
	* @return returns if recording is enabled or not
	*/
    UFUNCTION(BlueprintCallable, Category = "Airsim")
    bool ToggleRecording();

	/** Tells Recording Thread to Start Recording */
    virtual void StartRecording();

	/** Tells Recording Thread to Stop Recording */
    virtual void StopRecording();

	/** Checks if Recording Thread is recording */
    virtual bool IsRecording() const;

private:
	int RecordTickCount;
#pragma endregion Recording Functions

#pragma region API Functions
public:
	/** Starts API Server */
    void StartApiServer();

	/** Kills API Server */
    void StopApiServer();

	/** Returns true if Api Server is running */
    bool IsApiServerStarted();

	/** Returns the API Provider */
	msr::airlib::ApiProvider* GetApiProvider() const
	{
		return ApiProviderRef.get();
	}

	/** Returns the vehicles PawnSimApi from the vehicle name */
	PawnSimApi* GetVehicleSimApi(const std::string& vehicle_name = "")
	{
		return static_cast<PawnSimApi*>(ApiProviderRef->getVehicleSimApi(vehicle_name));
	}

protected:
	/**
	* Initilizes the Api Server
	* @return Returns a pointer to the API Server that was created
	* @note Base implementation returns nullptr as it is assumed simmode doesn't support APIs
	* @note Must Override
	*/
	virtual std::unique_ptr<msr::airlib::ApiServerBase> CreateApiServer() const;

	/**
	* Creates a PawnSimApi for a given vehicle_pawn. Calls CreateVehicleSimApi()
	* @return Returns a pointer to the PawnSimApi object created for the vehicle_pawn
	* @note Optional Override
	*/
	virtual std::unique_ptr<PawnSimApi> CreateVehicleApi(APawn* vehicle_pawn);

	/**
	* Creates the vehicle sim api object
	* @param pawn_sim_api_params - Params for the pawn sim api (Base implementation doesn't use this)
	* @return Returns a pointer to the PawnSimApi object
	* @note Base implementation creates a base PawnSimApi, it is recommended to override this with your vehicles PawnSimApi
	* @note Must Override
	*/
	virtual std::unique_ptr<PawnSimApi> CreateVehicleSimApi(const PawnSimApi::Params& pawn_sim_api_params) const;
private:

	std::unique_ptr<msr::airlib::WorldSimApiBase> WorldSimApiRef;
	std::unique_ptr<msr::airlib::ApiProvider> ApiProviderRef;
	std::unique_ptr<msr::airlib::ApiServerBase> ApiServer;

	std::vector<std::unique_ptr<msr::airlib::VehicleSimApiBase>> VehicleSimApis;
    
#pragma endregion API Functions

#pragma region Time Of Day
public:
	//TODO: Fillout Paramater Deffenitions
	/**
	* Sets the Time of Day in UE4
	* @param is_enabled
	* @param start_datetime
	* @param is_start_datetime_dst
	* @param celestial_clock_speed
	* @param update_intercal_secs
	* @param move_sun
	*/
	virtual void SetTimeOfDay(bool is_enabled, const std::string& start_datetime, bool is_start_datetime_dst, float celestial_clock_speed, float update_interval_secs, bool move_sun);

private:
	typedef msr::airlib::ClockFactory ClockFactory;
	typedef msr::airlib::TTimePoint TTimePoint;
	typedef msr::airlib::TTimeDelta TTimeDelta;

	UPROPERTY() UClass* sky_sphere_class_;

	UPROPERTY() AActor* sky_sphere_;
	UPROPERTY() ADirectionalLight* sun_;
	FRotator default_sun_rotation_;
	TTimePoint tod_sim_clock_start_;             // sim start in local time
	TTimePoint tod_last_update_;
	TTimePoint tod_start_time_;                  // tod, configurable
	bool tod_enabled_;
	float tod_celestial_clock_speed_;
	float tod_update_interval_secs_;
	bool tod_move_sun_;

	void initializeTimeOfDay();
	void advanceTimeOfDay();
	void setSunRotation(FRotator rotation);

#pragma endregion Time Of Day

#pragma region CameraDirector
public:
	// Reference to the camera director actor
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ACameraDirector* CameraDirector;

protected:
	/**
	* Spawns the Camera Director and sets it up.
	*/
	void InitializeCameraDirector(const FTransform& camera_transform, float follow_distance);

	/**
	* Used to get the first View Mode for the camera director
	* @return Returns the camera mode that should be used
	* @note Optional override
	*/
	virtual ECameraDirectorMode GetInitialViewMode() const;

	UPROPERTY() TSubclassOf<APIPCamera> PIPCameraClass;
private:
	UPROPERTY() TSubclassOf<ACameraDirector> CameraDirectorClass;
#pragma endregion CameraDirector

#pragma region Debug
public:
	/**
	* Returns a Debug Report that is shown on the HUD
	* @return A Debug Report
	*/
	virtual std::string GetDebugReport();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debugging")
	bool EnableReport = false;

protected:
	//Optional Override
	virtual void UpdateDebugReport(msr::airlib::StateReporterWrapper& debug_reporter);

private:
	msr::airlib::StateReporterWrapper debugReporter;

	bool lidar_checks_done_ = false;
	bool lidar_draw_debug_points_ = false;

	void DrawLidarDebugPoints();
	void DrawDistanceSensorDebugPoints();

	void ShowClockStats();
#pragma endregion Debug

#pragma region Singleton
public:
	/** Returns a reference to the initilized SimMode from a singleton format */
	UFUNCTION(BlueprintPure, Category = "Airsim")
	static ASimModeBase* GetSimMode();

private:
	static ASimModeBase* SIMMODE;

#pragma endregion Singleton

#pragma region Utility
public:
	//Why do these exist? 🤷‍
	TMap<FString, FAssetData> AssetMap;
	TMap<FString, AActor*> SceneObjectMap;

	const NedTransform& GetGlobalNedTransform();

protected:
	/**
	* Register Input Bindings as needed
	* @note Optional Override be sure to call base implemetation
	*/
	virtual void SetupInputBindings();

	/**
	* Returns the AirSimSettings Object that contains settings for setup
	* @deprecated Should call AirSimSettings::singleton() instead
	*/
	const msr::airlib::AirSimSettings& GetSettings() const;

	/**
	* Called when SimMode should handle clock speed setting
	* @note Optional Override
	*/
	virtual void SetupClockSpeed();

	/**
	* Converts Airsim Rotation to FRotator
	* @param rotation - The Airsim Rotation
	* @param default_val - In case any numbers in Airsim Rotation are nan use this
	* @return Returns converted FRotator
	*/
	FRotator ToFRotator(const AirSimSettings::Rotation& rotation, const FRotator& default_val = FRotator::ZeroRotator);

private:
	std::unique_ptr<NedTransform> GlobalNedTransform;

	UPROPERTY() TArray<AActor*> SpawnedActors; //keep refs alive from Unreal GC

	UPROPERTY() UParticleSystem* CollisionDisplayParticleSystem;

	void SetupPhysicsLoopPeriod();

	void SetStencilIDs();

#pragma endregion Utility
};
