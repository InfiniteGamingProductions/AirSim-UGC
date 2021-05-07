#include "FlyingPawn.h"
#include "Components/StaticMeshComponent.h"
#include "AirBlueprintLib.h"
#include "common/Common.hpp"
#include "Containers/UnrealString.h"

AFlyingPawn::AFlyingPawn()
{
	PrimaryActorTick.bCanEverTick = true;

    bodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body Mesh"));
	RootComponent = bodyMesh;
    
    pawn_events_.getActuatorSignal().connect_member(this, &AFlyingPawn::SetRotorSpeed);

	vehicleSettings = CreateDefaultSubobject<UVehicleSettingsComponent>(TEXT("Vehicle Settings"));
	vehicleSettings->SimModeType = ESimModeType::Multirotor;
}

void AFlyingPawn::BeginPlay()
{
    Super::BeginPlay();

	initializeProps();
}

void AFlyingPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
	
    pawn_events_.getPawnTickSignal().emit(DeltaSeconds);
}

void AFlyingPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    pawn_events_.getActuatorSignal().disconnect_all();

    Super::EndPlay(EndPlayReason);
}

const common_utils::UniqueValueMap<std::string, APIPCamera*> AFlyingPawn::GetCameras() const
{
    common_utils::UniqueValueMap<std::string, APIPCamera*> uniqueValueMap;
	
	for (const TTuple<FString, APIPCamera*>& Element : cameras)
	{
		uniqueValueMap.insert_or_assign(TCHAR_TO_UTF8(*Element.Key), Element.Value);
	}
	
	return uniqueValueMap;

	// Old Implementation, Here for reference
	/*cameras.insert_or_assign("front_center", camera_front_center_);
    cameras.insert_or_assign("front_right", camera_front_right_);
    cameras.insert_or_assign("front_left", camera_front_left_);
    cameras.insert_or_assign("bottom_center", camera_bottom_center_);
    cameras.insert_or_assign("back_center", camera_back_center_);

    cameras.insert_or_assign("0", camera_front_center_);
    cameras.insert_or_assign("1", camera_front_right_);
    cameras.insert_or_assign("2", camera_front_left_);
    cameras.insert_or_assign("3", camera_bottom_center_);
    cameras.insert_or_assign("4", camera_back_center_);

    cameras.insert_or_assign("", camera_front_center_);
    cameras.insert_or_assign("fpv", camera_front_center_);*/
}

void AFlyingPawn::NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, 
    FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
    pawn_events_.getCollisionSignal().emit(MyComp, Other, OtherComp, bSelfMoved, HitLocation,
        HitNormal, NormalImpulse, Hit);
}

void AFlyingPawn::SetRotorSpeed(const std::vector<MultirotorPawnEvents::RotorActuatorInfo>& rotor_infos)
{
    for (int32 rotor_index = 0; rotor_index < rotor_infos.size(); rotor_index++) {
        URotatingMovementComponent* comp = rotatingMovements[rotor_index];
        if (comp != nullptr) {
            comp->RotationRate.Yaw = 
                rotor_infos.at(rotor_index).rotor_speed * rotor_infos.at(rotor_index).rotor_direction *
                180.0f / M_PIf * RotatorFactor;
        }
    }
}

void AFlyingPawn::initializeProps()
{
	if (rotatingMovements.Num() == 0 || propMeshes.Num() == 0)
	{
		return;
	}
	
	for (int32 x = 0; x < rotatingMovements.Num() && x < propMeshes.Num(); x++)
	{
		if (rotatingMovements[x] != nullptr && propMeshes[x] != nullptr)
		{
			rotatingMovements[x]->SetUpdatedComponent(propMeshes[x]);
		}
	}
}
