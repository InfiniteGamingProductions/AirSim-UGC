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
	/** Returns a reference to the initilized SimMode from a singleton format */
	UFUNCTION(BlueprintPure, Category = "Airsim")
	static ASimModeBase* GetSimMode();

	ASimModeBase();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Resets the SimMode */
    UFUNCTION(BlueprintCallable, Category = "Airsim")
    virtual void Reset();

	/**
	* Used to set the Wind Direction and Magnitude
	*/
	virtual void SetWind(const msr::airlib::Vector3r& wind) const;

	// Pause Functions
	/**
	* Called to determine if the simulation is paused or not 
	* @return If the simulation is paused or not
	*/
	virtual bool IsSimulationPaused() const;

	/**
	* Called to pause or unpause the simulation
	* @param Pause - true if you want to pause the sim, false if you want to resume
	*/
	virtual void PauseSimulation(bool Pause);

	/**
	* Called by WorldSimAPI to unpause for seconds and then pause again
	* @param Seconds - The time in seconds to continue
	* @warning Depending on implementation this halts the game thread!
	*/
	virtual void ContinueForTime(double Seconds);

	/**
	* Called by WorldSimAPI to unpause for a number of frames and then pause again
	* @param Frames - The number of Frames to continue
	* @warning Depending on implementation this halts the game thread!
	*/
	virtual void ContinueForFrames(uint32_t Frames);

	// Recording Functions
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

	// Debug Functions
    /**
	* Returns a Debug Report that is shown on the HUD
	* @return A Debug Report
	*/
    virtual std::string GetDebugReport();

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
    virtual void SetTimeOfDay(bool is_enabled, const std::string& start_datetime, bool is_start_datetime_dst,
        float celestial_clock_speed, float update_interval_secs, bool move_sun);

	// API Functions
	/** Starts API Server */
    void StartApiServer();

	/** Kills API Server */
    void StopApiServer();

	/** Returns true if Api Server is running */
    bool IsApiServerStarted();

protected:
	virtual std::unique_ptr<msr::airlib::ApiServerBase> createApiServer() const;

public:
    bool createVehicleAtRuntime(const std::string& vehicle_name, const std::string& vehicle_type,
        const msr::airlib::Pose& pose, const std::string& pawn_path = "");

    const NedTransform& getGlobalNedTransform();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Refs")
    ACameraDirector* CameraDirector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debugging")
    bool EnableReport = false;

    msr::airlib::ApiProvider* getApiProvider() const
    {
        return api_provider_.get();
    }
    const PawnSimApi* getVehicleSimApi(const std::string& vehicle_name = "") const
    {
        return static_cast<PawnSimApi*>(api_provider_->getVehicleSimApi(vehicle_name));
    }
    PawnSimApi* getVehicleSimApi(const std::string& vehicle_name = "")
    {
        return static_cast<PawnSimApi*>(api_provider_->getVehicleSimApi(vehicle_name));
    }

    TMap<FString, FAssetData> asset_map;
    TMap<FString, AActor*> scene_object_map;

protected: //must overrides
    typedef msr::airlib::AirSimSettings AirSimSettings;

    
    virtual void getExistingVehiclePawns(TArray<AActor*>& pawns) const;
    virtual bool isVehicleTypeSupported(const std::string& vehicle_type) const;
    virtual std::string getVehiclePawnPathName(const AirSimSettings::VehicleSetting& vehicle_setting) const;
    virtual PawnEvents* getVehiclePawnEvents(APawn* pawn) const;
    virtual const common_utils::UniqueValueMap<std::string, APIPCamera*> getVehiclePawnCameras(APawn* pawn) const;
    virtual void initializeVehiclePawn(APawn* pawn);
    virtual std::unique_ptr<PawnSimApi> createVehicleSimApi(
        const PawnSimApi::Params& pawn_sim_api_params) const;
    virtual msr::airlib::VehicleApiBase* getVehicleApi(const PawnSimApi::Params& pawn_sim_api_params,
        const PawnSimApi* sim_api) const;
    virtual void registerPhysicsBody(msr::airlib::VehicleSimApiBase *physicsBody);

protected: //optional overrides
    virtual APawn* createVehiclePawn(const AirSimSettings::VehicleSetting& vehicle_setting);
    virtual std::unique_ptr<PawnSimApi> createVehicleApi(APawn* vehicle_pawn);
    virtual void setupVehiclesAndCamera();
    virtual void setupInputBindings();
    //called when SimMode should handle clock speed setting
    virtual void setupClockSpeed();
    void initializeCameraDirector(const FTransform& camera_transform, float follow_distance);
    void checkVehicleReady(); //checks if vehicle is available to use
    virtual void updateDebugReport(msr::airlib::StateReporterWrapper& debug_reporter);

	/**
	* Used to get the first View Mode for the camera director
	* @return Returns the camera mode that should be used
	*/
	virtual ECameraDirectorMode GetInitialViewMode() const;

protected: //Utility methods for derived classes
    virtual const msr::airlib::AirSimSettings& getSettings() const;
    FRotator toFRotator(const AirSimSettings::Rotation& rotation, const FRotator& default_val);


protected:
    int record_tick_count;
    UPROPERTY() UClass* pip_camera_class;
    UPROPERTY() UParticleSystem* collision_display_template;

private:
    typedef common_utils::Utils Utils;
    typedef msr::airlib::ClockFactory ClockFactory;
    typedef msr::airlib::TTimePoint TTimePoint;
    typedef msr::airlib::TTimeDelta TTimeDelta;
    typedef msr::airlib::SensorBase::SensorType SensorType;
    typedef msr::airlib::Vector3r Vector3r;
    typedef msr::airlib::Pose Pose;
    typedef msr::airlib::VectorMath VectorMath;

private:
    //assets loaded in constructor
    UPROPERTY() UClass* external_camera_class_;
    UPROPERTY() UClass* camera_director_class_;
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

    std::unique_ptr<NedTransform> global_ned_transform_;
    std::unique_ptr<msr::airlib::WorldSimApiBase> world_sim_api_;
    std::unique_ptr<msr::airlib::ApiProvider> api_provider_;
    std::unique_ptr<msr::airlib::ApiServerBase> api_server_;
    msr::airlib::StateReporterWrapper debug_reporter_;

    std::vector<std::unique_ptr<msr::airlib::VehicleSimApiBase>> vehicle_sim_apis_;

    UPROPERTY()
        TArray<AActor*> spawned_actors_; //keep refs alive from Unreal GC

    bool lidar_checks_done_ = false; 
    bool lidar_draw_debug_points_ = false;
    static ASimModeBase* SIMMODE;
private:
    void setStencilIDs();
    void initializeTimeOfDay();
    void advanceTimeOfDay();
    void setSunRotation(FRotator rotation);
    void setupPhysicsLoopPeriod();
    void showClockStats();
    void drawLidarDebugPoints();
    void drawDistanceSensorDebugPoints();
};
