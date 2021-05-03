#pragma once

#include "CoreMinimal.h"

#include "ComputerVisionPawn.h"
#include "common/Common.hpp"
#include "api/VehicleSimApiBase.hpp"
#include "SimMode/SimModeBase.h"

#include "SimModeComputerVision.generated.h"


UCLASS()
class AIRSIM_API ASimModeComputerVision : public ASimModeBase
{
    GENERATED_BODY()

protected:
    virtual std::unique_ptr<msr::airlib::ApiServerBase> CreateApiServer() const override;
    virtual void GetExistingVehiclePawns(TArray<AActor*>& pawns) const override;
    virtual bool IsVehicleTypeSupported(const std::string& vehicle_type) const override;
    virtual std::string GetVehiclePawnPath(const AirSimSettings::VehicleSetting& vehicle_setting) const override;
    virtual PawnEvents* GetVehiclePawnEvents(APawn* pawn) const override;
    virtual const common_utils::UniqueValueMap<std::string, APIPCamera*> GetVehiclePawnCameras(APawn* pawn) const override;
    virtual void InitializeVehiclePawn(APawn* pawn) override;
    virtual std::unique_ptr<PawnSimApi> CreateVehicleSimApi(const PawnSimApi::Params& pawn_sim_api_params) const override;
};
