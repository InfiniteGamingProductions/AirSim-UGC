#pragma once

#include "GameFramework/RotatingMovementComponent.h"

#include "PIPCamera.h"
#include "common/common_utils/UniqueValueMap.hpp"
#include "MultirotorPawnEvents.h"
#include "Vehicles/VehicleBasePawn.h"

#include "FlyingPawn.generated.h"

UCLASS()
class AIRSIM_API AFlyingPawn : public AVehicleBasePawn
{
    GENERATED_BODY()

public:
    AFlyingPawn();
	
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation,
        FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

    //Returns the camera objects from the pawn
    const common_utils::UniqueValueMap<std::string, APIPCamera*> GetCameras() const;
	
	//Returns the pawn events object
    FORCEINLINE MultirotorPawnEvents* GetPawnEvents() { return &pawn_events_; }
	
    //Used to set the prop speed
    void SetRotorSpeed(const std::vector<MultirotorPawnEvents::RotorActuatorInfo>& rotor_infos);


private:
	//Functions

	//Pairs rotating movement components to the prop meshes
    UFUNCTION(BlueprintCallable, meta = (AllowPrivateAccess = "true"))
	void initializeProps();
    
    //Unreal components
	
	//The root mesh of the drone
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* bodyMesh;

	//Components used to rotate the props (Note: Must be added in blueprint and order matters start from back left prop and go clockwise)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TArray<URotatingMovementComponent*> rotatingMovements;

	//Static Mesh Components for the props (Note: Must be added in blueprint and order must match rotating movements)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TArray<UStaticMeshComponent*> propMeshes;

	//Holds Map of camera names to camera objects (Note: Must be added in blueprints)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TMap<FString, APIPCamera*> cameras;

	//Vars
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debugging", meta = (AllowPrivateAccess = "true"))
	float RotatorFactor = 1.0f;
	
    MultirotorPawnEvents pawn_events_;
};
