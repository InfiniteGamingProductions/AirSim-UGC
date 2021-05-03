// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "SimModeComputerVision.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

#include "AirBlueprintLib.h"
#include "common/AirSimSettings.hpp"
#include "PawnSimApi.h"
#include "AirBlueprintLib.h"
#include "common/Common.hpp"
#include "common/EarthUtils.hpp"
#include "api/VehicleSimApiBase.hpp"
#include "common/AirSimSettings.hpp"
#include "physics/Kinematics.hpp"
#include "api/RpcLibServerBase.hpp"


std::unique_ptr<msr::airlib::ApiServerBase> ASimModeComputerVision::CreateApiServer() const
{
#ifdef AIRLIB_NO_RPC
    return ASimModeBase::CreateApiServer();
#else
    return std::unique_ptr<msr::airlib::ApiServerBase>(new msr::airlib::RpcLibServerBase(
        GetApiProvider(), GetSettings().api_server_address, GetSettings().api_port));
#endif
}

void ASimModeComputerVision::GetExistingVehiclePawns(TArray<AActor*>& pawns) const
{
    UAirBlueprintLib::FindAllActor<TVehiclePawn>(this, pawns);
}

bool ASimModeComputerVision::IsVehicleTypeSupported(const std::string& vehicle_type) const
{
    return vehicle_type == msr::airlib::AirSimSettings::kVehicleTypeComputerVision;
}

std::string ASimModeComputerVision::GetVehiclePawnPath(const AirSimSettings::VehicleSetting& vehicle_setting) const
{
    //decide which derived BP to use
    std::string pawn_path = vehicle_setting.pawn_path;
    if (pawn_path == "")
        pawn_path = "DefaultComputerVision";

    return pawn_path;
}

PawnEvents* ASimModeComputerVision::GetVehiclePawnEvents(APawn* pawn) const
{
    return static_cast<TVehiclePawn*>(pawn)->getPawnEvents();
}
const common_utils::UniqueValueMap<std::string, APIPCamera*> ASimModeComputerVision::GetVehiclePawnCameras(
    APawn* pawn) const
{
    return static_cast<const TVehiclePawn*>(pawn)->getCameras();
}
void ASimModeComputerVision::InitializeVehiclePawn(APawn* pawn)
{
    static_cast<TVehiclePawn*>(pawn)->initializeForBeginPlay();
}

std::unique_ptr<PawnSimApi> ASimModeComputerVision::CreateVehicleSimApi(
    const PawnSimApi::Params& pawn_sim_api_params) const
{
    auto vehicle_sim_api = std::unique_ptr<PawnSimApi>(new PawnSimApi(pawn_sim_api_params));
    vehicle_sim_api->initialize();
    vehicle_sim_api->reset();
    return vehicle_sim_api;
}

bool ASimModeComputerVision::IsSimulationPaused() const
{
    return UGameplayStatics::IsGamePaused(this->GetWorld());
}

void ASimModeComputerVision::PauseSimulation(bool is_paused)
{
    UGameplayStatics::SetGamePaused(this->GetWorld(), is_paused);
}