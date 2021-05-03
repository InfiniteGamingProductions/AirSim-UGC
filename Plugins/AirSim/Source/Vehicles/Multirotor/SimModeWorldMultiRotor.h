#pragma once

#include "CoreMinimal.h"

#include "FlyingPawn.h"
#include "common/Common.hpp"
#include "SimMode/SimModeWorldBase.h"
#include "api/VehicleSimApiBase.hpp"
#include "SimModeWorldMultiRotor.generated.h"


UCLASS()
class AIRSIM_API ASimModeWorldMultiRotor : public ASimModeWorldBase
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
	
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected: //overrides
    virtual void SetupClockSpeed() override;
	
    virtual std::unique_ptr<msr::airlib::ApiServerBase> CreateApiServer() const override;
	
    virtual void GetExistingVehiclePawns(TArray<AActor*>& pawns) const override;
	
    virtual bool IsVehicleTypeSupported(const std::string& vehicle_type) const override;
	
    virtual std::string GetVehiclePawnPath(const AirSimSettings::VehicleSetting& vehicle_setting) const override;
	
    virtual PawnEvents* GetVehiclePawnEvents(APawn* pawn) const override;
	
    virtual const common_utils::UniqueValueMap<std::string, APIPCamera*> GetVehiclePawnCameras(APawn* pawn) const override;
	
    virtual void InitializeVehiclePawn(APawn* pawn) override;
	
    virtual std::unique_ptr<PawnSimApi> CreateVehicleSimApi(const PawnSimApi::Params& pawn_sim_api_params) const override;
};
