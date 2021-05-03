#include "SimModeBase.h"
#include "Recording/RecordingThread.h"
#include "Misc/MessageDialog.h"
#include "Misc/EngineVersion.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/OutputDeviceNull.h"
#include "Engine/World.h"

#include <memory>
#include "AirBlueprintLib.h"
#include "common/AirSimSettings.hpp"
#include "common/ScalableClock.hpp"
#include "common/SteppableClock.hpp"
#include "SimJoyStick/SimJoyStick.h"
#include "common/EarthCelestial.hpp"
#include "sensors/lidar/LidarSimple.hpp"
#include "sensors/distance/DistanceSimple.hpp"

#include "Kismet/GameplayStatics.h"

#include "Weather/WeatherLib.h"

#include "DrawDebugHelpers.h"

//TODO: this is going to cause circular references which is fine here but
//in future we should consider moving SimMode not derived from AActor and move
//it to AirLib and directly implement WorldSimApiBase interface
#include "WorldSimApi.h"

#include "UGCBaseGameInstance.h"
#include "UGCRegistry.h"

ASimModeBase *ASimModeBase::SIMMODE = nullptr;

ASimModeBase* ASimModeBase::GetSimMode()
{
    return SIMMODE;
}

ASimModeBase::ASimModeBase()
{
    SIMMODE = this;

	PrimaryActorTick.bCanEverTick = true;

    static ConstructorHelpers::FClassFinder<ACameraDirector> camera_director_class(TEXT("Blueprint'/AirSim/Blueprints/BP_CameraDirector'"));
    CameraDirectorClass = camera_director_class.Succeeded() ? camera_director_class.Class : nullptr;

	static ConstructorHelpers::FClassFinder<APIPCamera> pip_camera_class_val(TEXT("Blueprint'/AirSim/Blueprints/BP_PIPCamera'"));
	PIPCameraClass = pip_camera_class_val.Succeeded() ? pip_camera_class_val.Class : nullptr;

    static ConstructorHelpers::FObjectFinder<UParticleSystem> collision_display(TEXT("ParticleSystem'/AirSim/StarterContent/Particles/P_Explosion.P_Explosion'"));
    if (!collision_display.Succeeded())
        CollisionDisplayParticleSystem = collision_display.Object;
    else
        CollisionDisplayParticleSystem = nullptr;

    static ConstructorHelpers::FClassFinder<AActor> sky_sphere_class(TEXT("Blueprint'/Engine/EngineSky/BP_Sky_Sphere'"));
    sky_sphere_class_ = sky_sphere_class.Succeeded() ? sky_sphere_class.Class : nullptr;
}

void ASimModeBase::BeginPlay()
{
    Super::BeginPlay();

	//initilize APIs
	WorldSimApiRef.reset(new WorldSimApi(this));
	ApiProviderRef.reset(new msr::airlib::ApiProvider(WorldSimApiRef.get()));
	
	//Setup Debug Reporter
    debugReporter.initialize(false);
    debugReporter.reset();

	UAirBlueprintLib::setLogMessagesVisibility(GetSettings().log_messages_visible);

    //get player start
	//get the vehicle pawn to get this
    TArray<APawn*> pawns;
    GetExistingVehiclePawns(pawns);
    bool have_existing_pawns = pawns.Num() > 0;
    APawn* fpv_pawn = nullptr;
    
    if (have_existing_pawns) {
        fpv_pawn = pawns[0];
    }
    else {
		fpv_pawn = GetWorld()->GetFirstPlayerController()->GetPawn();
    }

	// Grab player location
    FTransform player_start_transform = fpv_pawn->GetActorTransform();
    FVector player_loc = player_start_transform.GetLocation();
    // Move the world origin to the player's location (this moves the coordinate system and adds
    // a corresponding offset to all positions to compensate for the shift)
    GetWorld()->SetNewWorldOrigin(FIntVector(player_loc) + GetWorld()->OriginLocation);
    // Regrab the player's position after the offset has been added (which should be 0,0,0 now)
    player_start_transform = fpv_pawn->GetActorTransform();
    GlobalNedTransform.reset(new NedTransform(player_start_transform, UAirBlueprintLib::GetWorldToMetersScale(this)));

    SetupInputBindings();

	//Setup Time Of Day
    initializeTimeOfDay();
    AirSimSettings::TimeOfDaySetting tod_setting = GetSettings().tod_setting;
    SetTimeOfDay(tod_setting.enabled, tod_setting.start_datetime, tod_setting.is_start_datetime_dst,
        tod_setting.celestial_clock_speed, tod_setting.update_interval_secs, tod_setting.move_sun);

	SetupPhysicsLoopPeriod();

	SetupClockSpeed();

    SetupVehiclesAndCamera();

	//Setup Recording
	RecordTickCount = 0;
    FRecordingThread::init();

    if (GetSettings().recording_setting.enabled)
        StartRecording();

	//Setup Weather
    UWorld* World = GetWorld();
    if (World)
    {
        UWeatherLib::initWeather(World, SpawnedActors);
    }
	
	//Setup Abliity for AirLib to Spawn Actors
    UAirBlueprintLib::GenerateActorMap(this, SceneObjectMap);
	UAirBlueprintLib::GenerateAssetRegistryMap(this, AssetMap);

	SetStencilIDs();

	UAirBlueprintLib::LogMessage(TEXT("Press F1 to see help"), TEXT(""), LogDebugLevel::Informational);
}

const NedTransform& ASimModeBase::GetGlobalNedTransform()
{
    return *GlobalNedTransform;
}

void ASimModeBase::CheckVehicleReady()
{
    for (auto& api : ApiProviderRef->getVehicleApis()) {
        if (api) { //sim-only vehicles may have api as null
            std::string message;
            if (!api->isReady(message)) {
                UAirBlueprintLib::LogMessage("Vehicle was not initialized", "", LogDebugLevel::Failure);
                if (message.size() > 0) {
                    UAirBlueprintLib::LogMessage(message.c_str(), "", LogDebugLevel::Failure);
                }
                UAirBlueprintLib::LogMessage("Tip: check connection info in settings.json", "", LogDebugLevel::Informational);
            }
        }
    }
}

void ASimModeBase::SetStencilIDs()
{
    UAirBlueprintLib::SetMeshNamingMethod(GetSettings().segmentation_setting.mesh_naming_method);

    if (GetSettings().segmentation_setting.init_method ==
            AirSimSettings::SegmentationSetting::InitMethodType::CommonObjectsRandomIDs) {     
        UAirBlueprintLib::InitializeMeshStencilIDs(!GetSettings().segmentation_setting.override_existing);
    }
    //else don't init
}

void ASimModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    FRecordingThread::stopRecording();
    FRecordingThread::killRecording();
    WorldSimApiRef.reset();
    ApiProviderRef.reset();
    ApiServer.reset();
    GlobalNedTransform.reset();

    CameraDirector = nullptr;
    sky_sphere_ = nullptr;
    sun_ = nullptr;

    SpawnedActors.Empty();
    VehicleSimApis.clear();

    Super::EndPlay(EndPlayReason);
}

void ASimModeBase::initializeTimeOfDay()
{
    sky_sphere_ = nullptr;
    sun_ = nullptr;

    TArray<AActor*> sky_spheres;
    UGameplayStatics::GetAllActorsOfClass(this->GetWorld(), sky_sphere_class_, sky_spheres);

    if (sky_spheres.Num() > 1)
        UAirBlueprintLib::LogMessage(TEXT("More than BP_Sky_Sphere were found. "),
            TEXT("TimeOfDay settings would be applied to first one."), LogDebugLevel::Failure);

    if (sky_spheres.Num() >= 1) {
        sky_sphere_ = sky_spheres[0];
        static const FName sun_prop_name(TEXT("Directional light actor"));
        auto* p = sky_sphere_class_->FindPropertyByName(sun_prop_name);

#if ENGINE_MINOR_VERSION > 24
        FObjectProperty* sun_prop = CastFieldChecked<FObjectProperty>(p);
#else
        UObjectProperty* sun_prop = Cast<UObjectProperty>(p);
#endif

        UObject* sun_obj = sun_prop->GetObjectPropertyValue_InContainer(sky_sphere_);
        sun_ = Cast<ADirectionalLight>(sun_obj);
        if (sun_)
            default_sun_rotation_ = sun_->GetActorRotation(); 
    }
}

void ASimModeBase::SetTimeOfDay(bool is_enabled, const std::string& start_datetime, bool is_start_datetime_dst,
    float celestial_clock_speed, float update_interval_secs, bool move_sun)
{
    bool enabled_currently = tod_enabled_;
    
    if (is_enabled) {

        if (!sun_) {
            UAirBlueprintLib::LogMessage(TEXT("BP_Sky_Sphere was not found. "),
                TEXT("TimeOfDay settings would be ignored."), LogDebugLevel::Failure);
        }
        else {
            sun_->GetRootComponent()->Mobility = EComponentMobility::Movable;

            // this is a bit odd but given how advanceTimeOfDay() works currently, 
            // tod_sim_clock_start_ needs to be reset here.
            tod_sim_clock_start_ = ClockFactory::get()->nowNanos();

            tod_last_update_ = 0;
            if (start_datetime != "")
                tod_start_time_ = Utils::to_time_t(start_datetime, is_start_datetime_dst) * 1E9;
            else
                tod_start_time_ = std::time(nullptr) * 1E9;
        }
    }
    else if (enabled_currently) {
        // Going from enabled to disabled
        if (sun_) {
            setSunRotation(default_sun_rotation_);
            UAirBlueprintLib::LogMessageString("DateTime: ", Utils::to_string(ClockFactory::get()->nowNanos() / 1E9), LogDebugLevel::Informational);
        }
    }

    // do these in the end to ensure that advanceTimeOfDay() doesn't see
    // any inconsistent state.
    tod_enabled_ = is_enabled;
    tod_celestial_clock_speed_ = celestial_clock_speed;
    tod_update_interval_secs_ = update_interval_secs;
    tod_move_sun_ = move_sun;
}

bool ASimModeBase::IsSimulationPaused() const
{
    return UGameplayStatics::IsGamePaused(this);
}

void ASimModeBase::PauseSimulation(bool is_paused)
{
	UGameplayStatics::SetGamePaused(GetWorld(), is_paused);
}

void ASimModeBase::ContinueForTime(double seconds)
{
    //should be overridden by derived class
    unused(seconds);
    throw std::domain_error("ContinueForTime is not implemented by SimMode");
}

void ASimModeBase::ContinueForFrames(uint32_t frames)
{
    //should be overriden by derived class
    unused(frames);
    throw std::domain_error("ContinueForFrames is not implemented by SimMode");
}

void ASimModeBase::SetWind(const msr::airlib::Vector3r& wind) const
{
    // should be overridden by derived class
    unused(wind);
    throw std::domain_error("SetWind not implemented by SimMode");
}

std::unique_ptr<msr::airlib::ApiServerBase> ASimModeBase::CreateApiServer() const
{
    //this will be the case when compilation with RPCLIB is disabled or simmode doesn't support APIs
    return nullptr;
}

void ASimModeBase::SetupClockSpeed()
{
    //default setup - this should be overridden in derived modes as needed

    float clock_speed = GetSettings().clock_speed;

    //setup clock in ClockFactory
    std::string clock_type = GetSettings().clock_type;

    if (clock_type == "ScalableClock")
        ClockFactory::get(std::make_shared<msr::airlib::ScalableClock>(clock_speed == 1 ? 1 : 1 / clock_speed));
    else if (clock_type == "SteppableClock")
        ClockFactory::get(std::make_shared<msr::airlib::SteppableClock>(
            static_cast<msr::airlib::TTimeDelta>(msr::airlib::SteppableClock::DefaultStepSize * clock_speed)));
    else
        throw std::invalid_argument(common_utils::Utils::stringf(
            "clock_type %s is not recognized", clock_type.c_str()));
}

void ASimModeBase::SetupPhysicsLoopPeriod()
{
}

void ASimModeBase::Tick(float DeltaSeconds)
{
    if (IsRecording())
        ++RecordTickCount;

    advanceTimeOfDay();

    ShowClockStats();

    UpdateDebugReport(debugReporter);

    DrawLidarDebugPoints();

    DrawDistanceSensorDebugPoints();

    Super::Tick(DeltaSeconds);
}

void ASimModeBase::ShowClockStats()
{
    float clock_speed = GetSettings().clock_speed;
    if (clock_speed != 1) {
        UAirBlueprintLib::LogMessageString("ClockSpeed config, actual: ", 
            Utils::stringf("%f, %f", clock_speed, ClockFactory::get()->getTrueScaleWrtWallClock()), 
            LogDebugLevel::Informational);
    }
}

void ASimModeBase::advanceTimeOfDay()
{
    const auto& settings = GetSettings();

    if (tod_enabled_ && sky_sphere_ && sun_ && tod_move_sun_) {
        auto secs = ClockFactory::get()->elapsedSince(tod_last_update_);
        if (secs > tod_update_interval_secs_) {
            tod_last_update_ = ClockFactory::get()->nowNanos();

            auto interval = ClockFactory::get()->elapsedSince(tod_sim_clock_start_) * tod_celestial_clock_speed_;
            uint64_t cur_time = ClockFactory::get()->addTo(tod_start_time_, interval)  / 1E9;

            UAirBlueprintLib::LogMessageString("DateTime: ", Utils::to_string(cur_time), LogDebugLevel::Informational);

            auto coord = msr::airlib::EarthCelestial::getSunCoordinates(cur_time, settings.origin_geopoint.home_geo_point.latitude,
                settings.origin_geopoint.home_geo_point.longitude);

            setSunRotation(FRotator(-coord.altitude, coord.azimuth, 0));
        }
    }
}

void ASimModeBase::setSunRotation(FRotator rotation)
{
    if (sun_ && sky_sphere_) {
        UAirBlueprintLib::RunCommandOnGameThread([this, rotation]() {
            sun_->SetActorRotation(rotation);

            FOutputDeviceNull ar;
            sky_sphere_->CallFunctionByNameWithArguments(TEXT("UpdateSunDirection"), ar, NULL, true);
        }, true /*wait*/);
    }
}

void ASimModeBase::Reset()
{
    //default implementation
    UAirBlueprintLib::RunCommandOnGameThread([this]() {
        for (auto& api : GetApiProvider()->getVehicleSimApis()) {
            api->reset();
        }
    }, true);
}

std::string ASimModeBase::GetDebugReport()
{
    return debugReporter.getOutput();
}

void ASimModeBase::SetupInputBindings()
{
    UAirBlueprintLib::EnableInput(this);

	APlayerController* controller = GetWorld()->GetFirstPlayerController();
	if (controller)
	{
		controller->InputComponent->BindAction("ResetAll", IE_Pressed, this, &ASimModeBase::Reset);
	}
}

ECameraDirectorMode ASimModeBase::GetInitialViewMode() const
{
    return Utils::toEnum<ECameraDirectorMode>(GetSettings().initial_view_mode);
}

const msr::airlib::AirSimSettings& ASimModeBase::GetSettings() const
{
    return AirSimSettings::singleton();
}

void ASimModeBase::InitializeCameraDirector(const FTransform& camera_transform, float follow_distance)
{
    TArray<AActor*> camera_dirs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACameraDirector::StaticClass(), camera_dirs);
    if (camera_dirs.Num() == 0) {
        //create director
        FActorSpawnParameters camera_spawn_params;
        camera_spawn_params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        camera_spawn_params.Name = "CameraDirector";
        CameraDirector = this->GetWorld()->SpawnActor<ACameraDirector>(CameraDirectorClass, 
            camera_transform, camera_spawn_params);
        CameraDirector->setFollowDistance(follow_distance);
        CameraDirector->setCameraRotationLagEnabled(false);
        //create external camera required for the director
        camera_spawn_params.Name = "ExternalCamera";
        CameraDirector->ExternalCamera = this->GetWorld()->SpawnActor<APIPCamera>(PIPCameraClass,
            camera_transform, camera_spawn_params);
    }
    else {
        CameraDirector = static_cast<ACameraDirector*>(camera_dirs[0]);
    }
}

bool ASimModeBase::ToggleRecording()
{
    if (IsRecording())
        StopRecording();
    else
        StartRecording();

    return IsRecording();
}

void ASimModeBase::StopRecording()
{
    FRecordingThread::stopRecording();
}

void ASimModeBase::StartRecording()
{
    FRecordingThread::startRecording(GetSettings().recording_setting, GetApiProvider()->getVehicleSimApis());
}

bool ASimModeBase::IsRecording() const
{
    return FRecordingThread::isRecording();
}

//API server start/stop
void ASimModeBase::StartApiServer()
{
    if (GetSettings().enable_rpc) {

#ifdef AIRLIB_NO_RPC
        ApiServer.reset();
#else
        ApiServer = CreateApiServer();
#endif

        try {
            ApiServer->start(false, SpawnedActors.Num() + 4);
        }
        catch (std::exception& ex) {
            UAirBlueprintLib::LogMessageString("Cannot start RpcLib Server", ex.what(), LogDebugLevel::Failure);
        }
    }
    else
        UAirBlueprintLib::LogMessageString("API server is disabled in settings", "", LogDebugLevel::Informational);

}
void ASimModeBase::StopApiServer()
{
    if (ApiServer != nullptr) {
        ApiServer->stop();
        ApiServer.reset(nullptr);
    }
}
bool ASimModeBase::IsApiServerStarted()
{
    return ApiServer != nullptr;
}

void ASimModeBase::UpdateDebugReport(msr::airlib::StateReporterWrapper& debug_reporter)
{
    debug_reporter.update();
    debug_reporter.setEnable(EnableReport);

    if (debug_reporter.canReport()) {
        debug_reporter.clearReport();

        for (auto& api : GetApiProvider()->getVehicleSimApis()) {
            PawnSimApi* vehicle_sim_api = static_cast<PawnSimApi*>(api);
            msr::airlib::StateReporter& reporter = *debug_reporter.getReporter();
            std::string vehicle_name = vehicle_sim_api->getVehicleName();

            reporter.writeHeading(std::string("Vehicle: ").append(
                vehicle_name == "" ? "(default)" : vehicle_name));

            const msr::airlib::Kinematics::State* kinematics = vehicle_sim_api->getGroundTruthKinematics();

            reporter.writeValue("Position", kinematics->pose.position);
            reporter.writeValue("Orientation", kinematics->pose.orientation);
            reporter.writeValue("Lin-Vel", kinematics->twist.linear);
            reporter.writeValue("Lin-Accl", kinematics->accelerations.linear);
            reporter.writeValue("Ang-Vel", kinematics->twist.angular);
            reporter.writeValue("Ang-Accl", kinematics->accelerations.angular);
        }
    }
}

FRotator ASimModeBase::ToFRotator(const msr::airlib::AirSimSettings::Rotation& rotation, const FRotator& default_val)
{
    FRotator frotator = default_val;
    if (!std::isnan(rotation.yaw))
        frotator.Yaw = rotation.yaw;
    if (!std::isnan(rotation.pitch))
        frotator.Pitch = rotation.pitch;
    if (!std::isnan(rotation.roll))
        frotator.Roll = rotation.roll;

    return frotator;
}

APawn* ASimModeBase::SpawnVehiclePawn(const AirSimSettings::VehicleSetting& vehicle_setting)
{
    //get UU origin of global NED frame
    const FTransform uu_origin = GetGlobalNedTransform().getGlobalTransform();

    // compute initial pose
    FVector spawn_position = uu_origin.GetLocation();
    Vector3r settings_position = vehicle_setting.position;
    if (!VectorMath::hasNan(settings_position))
        spawn_position = GetGlobalNedTransform().fromGlobalNed(settings_position);

    FRotator spawn_rotation = ToFRotator(vehicle_setting.rotation, uu_origin.Rotator());

    std::string vehicle_name = vehicle_setting.vehicle_name;

    //spawn vehicle pawn
    FActorSpawnParameters pawn_spawn_params;
    pawn_spawn_params.Name = FName(vehicle_name.c_str());
    pawn_spawn_params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    UClass* vehicle_bp_class = UAirBlueprintLib::LoadClass(
        GetSettings().pawn_paths.at(
			GetVehiclePawnPath(vehicle_setting)).pawn_bp
	);

	//Check if main vehicle is trying to be overridden.
	UUGCBaseGameInstance* GameInstance = static_cast<UUGCBaseGameInstance*>(GetGameInstance());
	if (GameInstance)
	{
		TSubclassOf<AActor> overrideVehicleClass = GameInstance->UGCRegistry->GetOverrideForActorClass(vehicle_bp_class);
		vehicle_bp_class = overrideVehicleClass;
	}

    APawn* spawned_pawn = static_cast<APawn*>(
		this->GetWorld()->SpawnActor(vehicle_bp_class, &spawn_position, &spawn_rotation, pawn_spawn_params)
	);

    SpawnedActors.Add(spawned_pawn);

    return spawned_pawn;
}

std::unique_ptr<PawnSimApi> ASimModeBase::CreateVehicleApi(APawn* vehicle_pawn)
{
    InitializeVehiclePawn(vehicle_pawn);

    //create vehicle sim api
    const auto& ned_transform = GetGlobalNedTransform();
    const auto& pawn_ned_pos = ned_transform.toLocalNed(vehicle_pawn->GetActorLocation());
    const auto& home_geopoint = msr::airlib::EarthUtils::nedToGeodetic(pawn_ned_pos, GetSettings().origin_geopoint);
    const std::string vehicle_name( TCHAR_TO_UTF8(*(vehicle_pawn->GetName())) );

    PawnSimApi::Params pawn_sim_api_params(vehicle_pawn, &GetGlobalNedTransform(),
        GetVehiclePawnEvents(vehicle_pawn), GetVehiclePawnCameras(vehicle_pawn), PIPCameraClass, 
        CollisionDisplayParticleSystem, home_geopoint, vehicle_name);

    std::unique_ptr<PawnSimApi> vehicle_sim_api = CreateVehicleSimApi(pawn_sim_api_params);
	msr::airlib::VehicleApiBase* vehicle_api = vehicle_sim_api->getVehicleApiBase();
    GetApiProvider()->insert_or_assign(vehicle_name, vehicle_api, vehicle_sim_api.get());

    return vehicle_sim_api;
}

bool ASimModeBase::SpawnVehicleAtRuntime(const std::string& vehicle_name, const std::string& vehicle_type,
    const msr::airlib::Pose& pose, const std::string& pawn_path)
{
    if (!IsVehicleTypeSupported(vehicle_type)) {
        Utils::log(Utils::stringf("Vehicle type %s is not supported in this game mode", vehicle_type.c_str()), Utils::kLogLevelWarn);
        return false;
    }

    // TODO: Figure out a better way to add more fields
    //       Maybe allow passing a JSON string for the vehicle settings?

    // Retroactively adjust AirSimSettings, so it's like we knew about this vehicle all along
    AirSimSettings::singleton().addVehicleSetting(vehicle_name, vehicle_type, pose, pawn_path);
    const auto* vehicle_setting = GetSettings().getVehicleSetting(vehicle_name);

    auto spawned_pawn = SpawnVehiclePawn(*vehicle_setting);

    auto vehicle_sim_api = CreateVehicleApi(spawned_pawn);

    // Usually physics registration happens at init, in ASimModeWorldBase::initializeForPlay(), but not in this case
    vehicle_sim_api->reset();
    RegisterPhysicsBody(vehicle_sim_api.get());

    // Can't be done before the vehicle apis have been created
    if ((vehicle_setting->is_fpv_vehicle || !GetApiProvider()->hasDefaultVehicle()) && vehicle_name != "")
        GetApiProvider()->makeDefaultVehicle(vehicle_name);

    VehicleSimApis.push_back(std::move(vehicle_sim_api));

	//TODO: return if spawning vehicle was sucessfull or not
    return true;
}

void ASimModeBase::SetupVehiclesAndCamera()
{
    //get UU origin of global NED frame
    const FTransform uu_origin = GetGlobalNedTransform().getGlobalTransform();

    //determine camera director camera default pose and spawn it
    const auto& camera_director_setting = GetSettings().camera_director;
    FVector camera_director_position_uu = uu_origin.GetLocation() + 
        GetGlobalNedTransform().fromLocalNed(camera_director_setting.position);
    FTransform camera_transform(ToFRotator(camera_director_setting.rotation), 
        camera_director_position_uu);
    InitializeCameraDirector(camera_transform, camera_director_setting.follow_distance);

    //find all vehicle pawns
    TArray<APawn*> pawns;
    GetExistingVehiclePawns(pawns);
    bool haveUEPawns = pawns.Num() > 0;
    APawn* fpv_pawn = nullptr;
        
    if (haveUEPawns) {
        fpv_pawn = pawns[0];
    } else {
        //add vehicles from settings
        for (const auto& vehicle_setting_pair : GetSettings().vehicles)
        {
            //if vehicle is of type for derived SimMode and auto creatable
            const auto& vehicle_setting = *vehicle_setting_pair.second;
            if (vehicle_setting.auto_create && IsVehicleTypeSupported(vehicle_setting.vehicle_type)) {

                APawn* spawned_pawn = SpawnVehiclePawn(vehicle_setting);
                pawns.Add(spawned_pawn);

                if (vehicle_setting.is_fpv_vehicle)
                    fpv_pawn = spawned_pawn;
            }
        }
    }
    //create API objects for each pawn we have
    for (APawn* pawn : pawns)
    {
        std::unique_ptr<PawnSimApi> vehicle_sim_api = CreateVehicleApi(pawn);
        std::string vehicle_name = vehicle_sim_api->getVehicleName();

        if ((fpv_pawn == pawn || !GetApiProvider()->hasDefaultVehicle()) && vehicle_name != "")
            GetApiProvider()->makeDefaultVehicle(vehicle_name);

        VehicleSimApis.push_back(std::move(vehicle_sim_api));
    }

    if (GetApiProvider()->hasDefaultVehicle()) {
        //TODO: better handle no FPV vehicles scenario
        GetVehicleSimApi()->possess();
        CameraDirector->initializeForBeginPlay(GetInitialViewMode(), GetVehicleSimApi()->getPawn(),
            GetVehicleSimApi()->getCamera("fpv"), GetVehicleSimApi()->getCamera("back_center"), nullptr);
    }
    else
        CameraDirector->initializeForBeginPlay(GetInitialViewMode(), nullptr, nullptr, nullptr, nullptr);

    CheckVehicleReady();
}

void ASimModeBase::RegisterPhysicsBody(msr::airlib::VehicleSimApiBase *physicsBody)
{
    // derived class shoudl override this method to add new vehicle to the physics engine
}

void ASimModeBase::GetExistingVehiclePawns(TArray<APawn*>& OutPawns) const
{
	unused(OutPawns);
    //derived class should override this method to retrieve types of pawns they support
}

bool ASimModeBase::IsVehicleTypeSupported(const std::string& vehicle_type) const
{
    //derived class should override this method to retrieve types of pawns they support
    return false;
}

std::string ASimModeBase::GetVehiclePawnPath(const AirSimSettings::VehicleSetting& vehicle_setting) const
{
    //derived class should override this method to retrieve types of pawns they support
    return "";
}

PawnEvents* ASimModeBase::GetVehiclePawnEvents(APawn* pawn) const
{
    unused(pawn);

    //derived class should override this method to retrieve types of pawns they support
    return nullptr;
}

const common_utils::UniqueValueMap<std::string, APIPCamera*> ASimModeBase::GetVehiclePawnCameras(APawn* pawn) const
{
    unused(pawn);

    //derived class should override this method to retrieve types of pawns they support
    return common_utils::UniqueValueMap<std::string, APIPCamera*>();
}

void ASimModeBase::InitializeVehiclePawn(APawn* pawn)
{
    unused(pawn);
    //derived class should override this method to setup the vehicle pawn when it is spawed
}

std::unique_ptr<PawnSimApi> ASimModeBase::CreateVehicleSimApi(const PawnSimApi::Params& pawn_sim_api_params) const
{
    unused(pawn_sim_api_params);
	std::unique_ptr<PawnSimApi> sim_api = std::unique_ptr<PawnSimApi>();
    sim_api->initialize();

    return sim_api;
}

// Draws debug-points on main viewport for Lidar laser hits.
// Used for debugging only.
void ASimModeBase::DrawLidarDebugPoints()
{
    // Currently we are checking the sensor-collection instead of sensor-settings.
    // Also using variables to optimize not checking the collection if not needed.
    if (lidar_checks_done_ && !lidar_draw_debug_points_)
        return;

    if (GetApiProvider() == nullptr)
        return;

    for (auto& sim_api : GetApiProvider()->getVehicleSimApis()) {
        PawnSimApi* pawn_sim_api = static_cast<PawnSimApi*>(sim_api);
        std::string vehicle_name = pawn_sim_api->getVehicleName();

        msr::airlib::VehicleApiBase* api = GetApiProvider()->getVehicleApi(vehicle_name);
        if (api != nullptr) {
            msr::airlib::uint count_lidars = api->getSensors().size(SensorType::Lidar);

            for (msr::airlib::uint i = 0; i < count_lidars; i++) {
                // TODO: Is it incorrect to assume LidarSimple here?
                const msr::airlib::LidarSimple* lidar =
                    static_cast<const msr::airlib::LidarSimple*>(api->getSensors().getByType(SensorType::Lidar, i));
                if (lidar != nullptr && lidar->getParams().draw_debug_points) {
                    lidar_draw_debug_points_ = true;

                    msr::airlib::LidarData lidar_data = lidar->getOutput();

                    if (lidar_data.point_cloud.size() < 3)
                        return;

                    for (int j = 0; j < lidar_data.point_cloud.size(); j = j + 3) {
                        Vector3r point(lidar_data.point_cloud[j], lidar_data.point_cloud[j + 1], lidar_data.point_cloud[j + 2]);

                        FVector uu_point;

                        if (lidar->getParams().data_frame == AirSimSettings::kVehicleInertialFrame) {
                            uu_point = pawn_sim_api->getNedTransform().fromLocalNed(point);
                        }
                        else if (lidar->getParams().data_frame == AirSimSettings::kSensorLocalFrame) {

                            Vector3r point_w = VectorMath::transformToWorldFrame(point, lidar_data.pose, true);
                            uu_point = pawn_sim_api->getNedTransform().fromLocalNed(point_w);
                        }
                        else
                            throw std::runtime_error("Unknown requested data frame");

                        DrawDebugPoint(
                            this->GetWorld(),
                            uu_point,
                            5,              // size
                            FColor::Green,
                            false,          // persistent (never goes away)
                            0.03            // LifeTime: point leaves a trail on moving object
                        );
                    }
                }
            }
        }
    }

    lidar_checks_done_ = true;
}

// Draw debug-point on main viewport for Distance sensor hit
void ASimModeBase::DrawDistanceSensorDebugPoints()
{
    if (GetApiProvider() == nullptr)
        return;

    for (auto& sim_api : GetApiProvider()->getVehicleSimApis()) {
        PawnSimApi* pawn_sim_api = static_cast<PawnSimApi*>(sim_api);
        std::string vehicle_name = pawn_sim_api->getVehicleName();

        msr::airlib::VehicleApiBase* api = GetApiProvider()->getVehicleApi(vehicle_name);

        if (api != nullptr) {
            msr::airlib::uint count_distance_sensors = api->getSensors().size(SensorType::Distance);
            Pose vehicle_pose = pawn_sim_api->getGroundTruthKinematics()->pose;

            for (msr::airlib::uint i=0; i<count_distance_sensors; i++) {
                const msr::airlib::DistanceSimple* distance_sensor = 
                    static_cast<const msr::airlib::DistanceSimple*>(api->getSensors().getByType(SensorType::Distance, i));

                if (distance_sensor != nullptr && distance_sensor->getParams().draw_debug_points) {
                    msr::airlib::DistanceSensorData distance_sensor_data = distance_sensor->getOutput();

                    // Find position of point hit
                    // Similar to UnrealDistanceSensor.cpp#L19
                    // order of Pose addition is important here because it also adds quaternions which is not commutative!
                    Pose distance_sensor_pose = distance_sensor_data.relative_pose + vehicle_pose;
                    Vector3r start = distance_sensor_pose.position;
                    Vector3r point = start + VectorMath::rotateVector(VectorMath::front(), 
                                                distance_sensor_pose.orientation, true) * distance_sensor_data.distance;

                    FVector uu_point = pawn_sim_api->getNedTransform().fromLocalNed(point);

                    DrawDebugPoint(
                        this->GetWorld(),
                        uu_point,
                        10,              // size
                        FColor::Green,
                        false,          // persistent (never goes away)
                        0.03            // LifeTime: point leaves a trail on moving object
                    );
                }
            }
        }
    }
}
