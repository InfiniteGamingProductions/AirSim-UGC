// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "SimModeWorldMultiRotor.h"
#include "UObject/ConstructorHelpers.h"
#include "Logging/MessageLog.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#include "AirBlueprintLib.h"
#include "vehicles/multirotor/api/MultirotorApiBase.hpp"
#include "MultirotorPawnSimApi.h"
#include "physics/PhysicsBody.hpp"
#include "common/ClockFactory.hpp"
#include <memory>
#include "vehicles/multirotor/api/MultirotorRpcLibServer.hpp"


void ASimModeWorldMultiRotor::BeginPlay()
{
    Super::BeginPlay();
}

void ASimModeWorldMultiRotor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void ASimModeWorldMultiRotor::SetupClockSpeed()
{
    typedef msr::airlib::ClockFactory ClockFactory;

    float clock_speed = GetSettings().clock_speed;

    //setup clock in ClockFactory
    std::string clock_type = GetSettings().clock_type;

    if (clock_type == "ScalableClock") {
        //scalable clock returns interval same as wall clock but multiplied by a scale factor
        ClockFactory::get(std::make_shared<msr::airlib::ScalableClock>(clock_speed == 1 ? 1 : 1 / clock_speed));
    }
    else if (clock_type == "SteppableClock") {
        //steppable clock returns interval that is a constant number irrespective of wall clock
        //we can either multiply this fixed interval by scale factor to speed up/down the clock
        //but that would cause vehicles like quadrotors to become unstable
        //so alternative we use here is instead to scale control loop frequency. The downside is that
        //depending on compute power available, we will max out control loop frequency and therefore can no longer
        //get increase in clock speed

        //Approach 1: scale clock period, no longer used now due to quadrotor instability
        //ClockFactory::get(std::make_shared<msr::airlib::SteppableClock>(
        //static_cast<msr::airlib::TTimeDelta>(GetPhysicsLoopPeriod() * 1E-9 * clock_speed)));

        //Approach 2: scale control loop frequency if clock is speeded up
        if (clock_speed >= 1) {
            ClockFactory::get(std::make_shared<msr::airlib::SteppableClock>(
                static_cast<msr::airlib::TTimeDelta>(GetPhysicsLoopPeriod() * 1E-9))); //no clock_speed multiplier

            SetPhysicsLoopPeriod(GetPhysicsLoopPeriod() / static_cast<long long>(clock_speed));
        }
        else {
            //for slowing down, this don't generate instability
            ClockFactory::get(std::make_shared<msr::airlib::SteppableClock>(
                static_cast<msr::airlib::TTimeDelta>(GetPhysicsLoopPeriod() * 1E-9 * clock_speed)));
        }
    }
    else
        throw std::invalid_argument(common_utils::Utils::stringf(
            "clock_type %s is not recognized", clock_type.c_str()));
}

std::unique_ptr<msr::airlib::ApiServerBase> ASimModeWorldMultiRotor::CreateApiServer() const
{
#ifdef AIRLIB_NO_RPC
    return ASimModeBase::CreateApiServer();
#else
    return std::unique_ptr<msr::airlib::ApiServerBase>(new msr::airlib::MultirotorRpcLibServer(
        GetApiProvider(), GetSettings().api_server_address, GetSettings().api_port));
#endif
}

void ASimModeWorldMultiRotor::GetExistingVehiclePawns(TArray<AActor*>& pawns) const
{
    UAirBlueprintLib::FindAllActor<AFlyingPawn>(this, pawns);
}

bool ASimModeWorldMultiRotor::IsVehicleTypeSupported(const std::string& vehicle_type) const
{
    return ((vehicle_type == AirSimSettings::kVehicleTypeSimpleFlight) ||
            (vehicle_type == AirSimSettings::kVehicleTypePX4) ||
            (vehicle_type == AirSimSettings::kVehicleTypeArduCopterSolo) ||
            (vehicle_type == AirSimSettings::kVehicleTypeArduCopter));
}

std::string ASimModeWorldMultiRotor::GetVehiclePawnPath(const AirSimSettings::VehicleSetting& vehicle_setting) const
{
    //decide which derived BP to use
    std::string pawn_path = vehicle_setting.pawn_path;
    if (pawn_path == "")
        pawn_path = "DefaultQuadrotor";

    return pawn_path;
}

PawnEvents* ASimModeWorldMultiRotor::GetVehiclePawnEvents(APawn* pawn) const
{
    return static_cast<AFlyingPawn*>(pawn)->GetPawnEvents();
}

const common_utils::UniqueValueMap<std::string, APIPCamera*> ASimModeWorldMultiRotor::GetVehiclePawnCameras(
    APawn* pawn) const
{
    return (static_cast<const AFlyingPawn*>(pawn))->GetCameras();
}

std::unique_ptr<PawnSimApi> ASimModeWorldMultiRotor::CreateVehicleSimApi(const PawnSimApi::Params& pawn_sim_api_params) const
{
    std::unique_ptr<PawnSimApi> vehicle_sim_api = std::make_unique<MultirotorPawnSimApi>(pawn_sim_api_params);
    vehicle_sim_api->initialize();
	
    //For multirotors the vehicle_sim_api are in PhysicsWorld container and then get reset when world gets reset
    //vehicle_sim_api->reset();
	
    return vehicle_sim_api;
}