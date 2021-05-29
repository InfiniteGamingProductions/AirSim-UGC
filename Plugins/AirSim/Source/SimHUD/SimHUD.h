#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SimHUDWidget.h"
#include "SimMode/SimModeBase.h"
#include "PIPCamera.h"
#include "api/ApiServerBase.hpp"
#include <memory>
#include "SimHUD.generated.h"


UENUM(BlueprintType)
enum class ESimulatorMode : uint8
{
    SIM_MODE_HIL 	UMETA(DisplayName = "Hardware-in-loop")
};

UCLASS()
class AIRSIM_API ASimHUD : public AHUD
{
    GENERATED_BODY()

public:
	typedef msr::airlib::AirSimSettings AirSimSettings;

	ASimHUD();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	//Input Event Bindings
    void OnToggleRecording();
    void OnToggleReport();
    void OnToggleHelp();
    void OnToggleTrace();
    void OnToggleSubwindow0();
    void OnToggleSubwindow1();
    void OnToggleSubwindow2();
    void OnToggleAllSubwindows();
	void OnPause();

	UFUNCTION(BlueprintCallable, Category = "Pause")
	void PauseSimulation();

	UFUNCTION(BlueprintCallable, Category = "Pause")
	void UnpauseSimulation();

private:
	//Begin Play Setup Functions
	void SetUnrealEngineSettings();
	void CreateSimMode();
	void CreateHUDWidget();
	void InitializeSubWindows();
	void SetupInputBindings();

	void ToggleRecordHandler();

	void UpdateWidgetSubwindowVisibility();
    std::vector<AirSimSettings::SubwindowSetting>& GetSubWindowSettings();

    UPROPERTY() USimHUDWidget* HUDWidget;
    UPROPERTY() ASimModeBase* SimMode;
	UPROPERTY() UUserWidget* PauseMenu;

    APIPCamera* SubwindowCameras[AirSimSettings::kSubwindowCount];

	TSubclassOf<UUserWidget> HUDWidgetClass;
	TSubclassOf<UUserWidget> PauseMenuClass;
};
