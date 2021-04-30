#pragma once

#include "CoreMinimal.h"

#include "SimMode/SimModeBase.h"
#include "CarPawn.h"
#include "common/Common.hpp"
#include "api/VehicleSimApiBase.hpp"
#include "SimModeCar.generated.h"


UCLASS()
class AIRSIM_API ASimModeCar : public ASimModeBase
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    virtual bool IsSimulationPaused() const override;
    virtual void PauseSimulation(bool is_paused) override;
    virtual void ContinueForTime(double seconds) override;
    virtual void ContinueForFrames(uint32_t frames) override;

private:
    typedef msr::airlib::ClockFactory ClockFactory;
    typedef common_utils::Utils Utils;
    typedef msr::airlib::TTimePoint TTimePoint;
    typedef msr::airlib::TTimeDelta TTimeDelta;
    typedef ACarPawn TVehiclePawn;
    typedef msr::airlib::VehicleSimApiBase VehicleSimApiBase;
    typedef msr::airlib::VectorMath VectorMath;
    typedef msr::airlib::Vector3r Vector3r;

private:
    void initializePauseState();

protected:
    virtual void SetupClockSpeed() override;
    virtual std::unique_ptr<msr::airlib::ApiServerBase> CreateApiServer() const override;
    virtual void GetExistingVehiclePawns(TArray<AActor*>& pawns) const override;
    virtual bool IsVehicleTypeSupported(const std::string& vehicle_type) const override;
    virtual std::string GetVehiclePawnPath(const AirSimSettings::VehicleSetting& vehicle_setting) const override;
    virtual PawnEvents* GetVehiclePawnEvents(APawn* pawn) const override;
    virtual const common_utils::UniqueValueMap<std::string, APIPCamera*> GetVehiclePawnCameras(APawn* pawn) const override;
    virtual void InitializeVehiclePawn(APawn* pawn) override;
    virtual std::unique_ptr<PawnSimApi> CreateVehicleSimApi(
        const PawnSimApi::Params& pawn_sim_api_params) const override;
    virtual msr::airlib::VehicleApiBase* GetVehicleApi(const PawnSimApi::Params& pawn_sim_api_params,
        const PawnSimApi* sim_api) const override;

private:
    std::atomic<float> current_clockspeed_;
    std::atomic<TTimeDelta> pause_period_;
    std::atomic<TTimePoint> pause_period_start_;
    uint32_t targetFrameNumber_;
    std::atomic_bool frame_countdown_enabled_;;
};
