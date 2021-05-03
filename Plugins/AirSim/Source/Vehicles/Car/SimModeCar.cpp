#include "SimModeCar.h"
#include "UObject/ConstructorHelpers.h"

#include "AirBlueprintLib.h"
#include "common/AirSimSettings.hpp"
#include "CarPawnSimApi.h"
#include "AirBlueprintLib.h"
#include "common/Common.hpp"
#include "common/EarthUtils.hpp"
#include "vehicles/car/api/CarRpcLibServer.hpp"

extern CORE_API uint32 GFrameNumber;

void ASimModeCar::BeginPlay()
{
    Super::BeginPlay();

    initializePauseState();
}

void ASimModeCar::initializePauseState()
{
    pause_period_ = 0;
    pause_period_start_ = 0;
    PauseSimulation(false);
}

bool ASimModeCar::IsSimulationPaused() const
{
    return current_clockspeed_ == 0;
}

void ASimModeCar::PauseSimulation(bool is_paused)
{
    if (is_paused)
        current_clockspeed_ = 0;
    else
        current_clockspeed_ = GetSettings().clock_speed;

    UAirBlueprintLib::setUnrealClockSpeed(this, current_clockspeed_);
}

void ASimModeCar::ContinueForTime(double seconds)
{
    pause_period_start_ = ClockFactory::get()->nowNanos();
    pause_period_ = seconds;
    PauseSimulation(false);
}

void ASimModeCar::ContinueForFrames(uint32_t frames)
{
    targetFrameNumber_ = GFrameNumber + frames;
    frame_countdown_enabled_ = true;
    PauseSimulation(false);
}

void ASimModeCar::SetupClockSpeed()
{
    current_clockspeed_ = GetSettings().clock_speed;

    //setup clock in PhysX
    UAirBlueprintLib::setUnrealClockSpeed(this, current_clockspeed_);
    UAirBlueprintLib::LogMessageString("Clock Speed: ", std::to_string(current_clockspeed_), LogDebugLevel::Informational);
}

void ASimModeCar::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    
    if (pause_period_start_ > 0) {
        if (ClockFactory::get()->elapsedSince(pause_period_start_) >= pause_period_) {
            if (!IsSimulationPaused())
                PauseSimulation(true);

            pause_period_start_ = 0;
        }
    }

    if(frame_countdown_enabled_){
        if (targetFrameNumber_ <= GFrameNumber){
            if (! IsSimulationPaused())
                PauseSimulation(true);

            frame_countdown_enabled_ = false;
        }
    }
}

//-------------------------------- overrides -----------------------------------------------//

std::unique_ptr<msr::airlib::ApiServerBase> ASimModeCar::CreateApiServer() const
{
#ifdef AIRLIB_NO_RPC
    return ASimModeBase::CreateApiServer();
#else
    return std::unique_ptr<msr::airlib::ApiServerBase>(new msr::airlib::CarRpcLibServer(
        GetApiProvider(), GetSettings().api_server_address, GetSettings().api_port));
#endif
}

void ASimModeCar::GetExistingVehiclePawns(TArray<APawn*>& pawns) const
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACarPawn::StaticClass(), FoundActors);

	for each (AActor* actor in FoundActors)
	{
		pawns.Add(static_cast<APawn*>(actor));
	}
}

bool ASimModeCar::IsVehicleTypeSupported(const std::string& vehicle_type) const
{
    return ((vehicle_type == AirSimSettings::kVehicleTypePhysXCar) ||
            (vehicle_type == AirSimSettings::kVehicleTypeArduRover));
}

std::string ASimModeCar::GetVehiclePawnPath(const AirSimSettings::VehicleSetting& vehicle_setting) const
{
    //decide which derived BP to use
    std::string pawn_path = vehicle_setting.pawn_path;
    if (pawn_path == "")
        pawn_path = "DefaultCar";

    return pawn_path;
}

PawnEvents* ASimModeCar::GetVehiclePawnEvents(APawn* pawn) const
{
    return static_cast<ACarPawn*>(pawn)->getPawnEvents();
}

const common_utils::UniqueValueMap<std::string, APIPCamera*> ASimModeCar::GetVehiclePawnCameras(APawn* pawn) const
{
    return (static_cast<const ACarPawn*>(pawn))->getCameras();
}

void ASimModeCar::InitializeVehiclePawn(APawn* pawn)
{
    auto vehicle_pawn = static_cast<ACarPawn*>(pawn);
    vehicle_pawn->initializeForBeginPlay(GetSettings().engine_sound);
}

std::unique_ptr<PawnSimApi> ASimModeCar::CreateVehicleSimApi(
    const PawnSimApi::Params& pawn_sim_api_params) const
{
    auto vehicle_pawn = static_cast<ACarPawn*>(pawn_sim_api_params.pawn);
    auto vehicle_sim_api = std::unique_ptr<PawnSimApi>(new CarPawnSimApi(pawn_sim_api_params, 
        vehicle_pawn->getKeyBoardControls(), vehicle_pawn->getVehicleMovementComponent()));
    vehicle_sim_api->initialize();
    vehicle_sim_api->reset();
    return vehicle_sim_api;
}