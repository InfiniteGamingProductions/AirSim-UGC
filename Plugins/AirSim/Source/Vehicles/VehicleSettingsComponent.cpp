// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicles/VehicleSettingsComponent.h"

// Sets default values for this component's properties
UVehicleSettingsComponent::UVehicleSettingsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UVehicleSettingsComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UVehicleSettingsComponent::SetAirSimVehicleSettings()
{
	//TODO: This
}

