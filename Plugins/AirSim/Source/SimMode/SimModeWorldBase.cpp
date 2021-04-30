#include "SimModeWorldBase.h"
#include <exception>
#include "AirBlueprintLib.h"


void ASimModeWorldBase::BeginPlay()
{
    Super::BeginPlay();

	std::vector<msr::airlib::UpdatableObject*> vehicles;
	for (auto& api : GetApiProvider()->getVehicleSimApis())
		vehicles.push_back(api);
	//TODO: directly accept getVehicleSimApis() using generic container

	std::unique_ptr<PhysicsEngineBase> physics_engine = CreatePhysicsEngine();
	physicsEngine = physics_engine.get();
	physicsWorld.reset(new msr::airlib::PhysicsWorld(std::move(physics_engine),
		vehicles, GetPhysicsLoopPeriod()));
}

void ASimModeWorldBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	//keep this lock as short as possible
	physicsWorld->lock();

	physicsWorld->enableStateReport(EnableReport);
	physicsWorld->updateStateReport();

	for (auto& api : GetApiProvider()->getVehicleSimApis())
		api->updateRenderedState(DeltaSeconds);

	physicsWorld->unlock();

	//perform any expensive rendering update outside of lock region
	for (auto& api : GetApiProvider()->getVehicleSimApis())
		api->updateRendering(DeltaSeconds);
}

void ASimModeWorldBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//stop physics thread before we dismantle
	StopAsyncUpdator();

    //remove everything that we created in BeginPlay
    physicsWorld.reset();

    Super::EndPlay(EndPlayReason);
}

void ASimModeWorldBase::Reset()
{
	UAirBlueprintLib::RunCommandOnGameThread([this]() {
		physicsWorld->reset();
		}, true);

	//no need to call base reset because of our custom implementation
}

#pragma region Physics
void ASimModeWorldBase::SetWind(const msr::airlib::Vector3r& wind) const
{
	physicsEngine->setWind(wind);
}

void ASimModeWorldBase::StartAsyncUpdator()
{
    physicsWorld->startAsyncUpdator();
}

void ASimModeWorldBase::StopAsyncUpdator()
{
    physicsWorld->stopAsyncUpdator();
}

void ASimModeWorldBase::RegisterPhysicsBody(msr::airlib::VehicleSimApiBase *physicsBody)
{
	physicsWorld->addBody(physicsBody);
}

long long ASimModeWorldBase::GetPhysicsLoopPeriod() const //nanoseconds
{
    return physicsLoopPeriod;
}
void ASimModeWorldBase::SetPhysicsLoopPeriod(long long  period)
{
    physicsLoopPeriod = period;
}

std::unique_ptr<ASimModeWorldBase::PhysicsEngineBase> ASimModeWorldBase::CreatePhysicsEngine()
{
    std::unique_ptr<PhysicsEngineBase> physics_engine;
    std::string physics_engine_name = GetSettings().physics_engine_name;
    if (physics_engine_name == "")
        physics_engine.reset(); //no physics engine
    else if (physics_engine_name == "FastPhysicsEngine") {
        msr::airlib::Settings fast_phys_settings;
        if (msr::airlib::Settings::singleton().getChild("FastPhysicsEngine", fast_phys_settings)) {
            physics_engine.reset(new msr::airlib::FastPhysicsEngine(fast_phys_settings.getBool("EnableGroundLock", true)));
        }
        else {
            physics_engine.reset(new msr::airlib::FastPhysicsEngine());
        }

        physics_engine->setWind(GetSettings().wind);
    }
    else {
        physics_engine.reset();
        UAirBlueprintLib::LogMessageString("Unrecognized physics engine name: ",  physics_engine_name, LogDebugLevel::Failure);
    }

    return physics_engine;
}
#pragma endregion Physics

#pragma region Pause Functions
void ASimModeWorldBase::PauseSimulation(bool is_paused)
{
	Super::PauseSimulation(is_paused);

    physicsWorld->pause(is_paused);
}

void ASimModeWorldBase::ContinueForTime(double seconds)
{
    if(physicsWorld->isPaused())
    {
        physicsWorld->pause(false);
        UGameplayStatics::SetGamePaused(this->GetWorld(), false);        
    }

	//Wouldn't this halt game thread for seconds!!! thats not good
    physicsWorld->continueForTime(seconds);
    while(!physicsWorld->isPaused())
    {
        continue; 
    }

    UGameplayStatics::SetGamePaused(this->GetWorld(), true);
}

void ASimModeWorldBase::ContinueForFrames(uint32_t frames)
{
    if(physicsWorld->isPaused())
    {
        physicsWorld->pause(false);
        UGameplayStatics::SetGamePaused(this->GetWorld(), false);        
    }
    
    physicsWorld->setFrameNumber((uint32_t)GFrameNumber);
    physicsWorld->continueForFrames(frames);
    while(!physicsWorld->isPaused())
    {
        physicsWorld->setFrameNumber((uint32_t)GFrameNumber);
    }

    UGameplayStatics::SetGamePaused(this->GetWorld(), true);
}
#pragma endregion Pause Functions

#pragma region Debug
void ASimModeWorldBase::UpdateDebugReport(msr::airlib::StateReporterWrapper& debug_reporter)
{
    unused(debug_reporter);
    //we use custom debug reporting for this class
}

std::string ASimModeWorldBase::GetDebugReport()
{
    return physicsWorld->getDebugReport();
}
#pragma endregion Debug