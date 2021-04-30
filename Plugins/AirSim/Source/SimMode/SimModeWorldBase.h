#pragma once

#include "CoreMinimal.h"
#include <memory>
#include <vector>
#include "api/VehicleSimApiBase.hpp"
#include "physics/FastPhysicsEngine.hpp"
#include "physics/World.hpp"
#include "physics/PhysicsWorld.hpp"
#include "common/StateReporterWrapper.hpp"
#include "api/ApiServerBase.hpp"
#include "SimModeBase.h"
#include "SimModeWorldBase.generated.h"

extern CORE_API uint32 GFrameNumber;

UCLASS()
class AIRSIM_API ASimModeWorldBase : public ASimModeBase
{
    GENERATED_BODY()
    
public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick( float DeltaSeconds ) override;

    virtual void Reset() override;

#pragma region Physics
public:
	virtual void SetWind(const msr::airlib::Vector3r& wind) const override;

protected:
	virtual void RegisterPhysicsBody(msr::airlib::VehicleSimApiBase* physicsBody) override;

	void StartAsyncUpdator();
	void StopAsyncUpdator();

	long long GetPhysicsLoopPeriod() const;
	void SetPhysicsLoopPeriod(long long  period);
private:
	typedef msr::airlib::UpdatableObject UpdatableObject;
	typedef msr::airlib::PhysicsEngineBase PhysicsEngineBase;
	typedef msr::airlib::ClockFactory ClockFactory;

	//create the physics engine as needed from settings
	std::unique_ptr<PhysicsEngineBase> CreatePhysicsEngine();

private:
	std::unique_ptr<msr::airlib::PhysicsWorld> physicsWorld;
	PhysicsEngineBase* physicsEngine;

	/*
	300Hz seems to be minimum for non-aggressive flights
	400Hz is needed for moderately aggressive flights (such as
	high yaw rate with simultaneous back move)
	500Hz is recommended for more aggressive flights
	Lenovo P50 high-end config laptop seems to be topping out at 400Hz.
	HP Z840 desktop high-end config seems to be able to go up to 500Hz.
	To increase freq with limited CPU power, switch Barometer to constant ref mode.
	*/
	long long physicsLoopPeriod = 3000000LL; //3ms
#pragma endregion Physics

#pragma region Pause Functions
public:
    virtual bool IsSimulationPaused() const override;
    virtual void PauseSimulation(bool is_paused) override;
    virtual void ContinueForTime(double seconds) override;
    virtual void ContinueForFrames(uint32_t frames) override;
#pragma endregion Pause Functions

#pragma region Debug
public:
	virtual std::string GetDebugReport() override;

protected:
	virtual void UpdateDebugReport(msr::airlib::StateReporterWrapper& debug_reporter) override;
#pragma endregion Debug
};
