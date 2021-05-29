#include "SimHUD.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"

#include "Vehicles/Multirotor/SimModeWorldMultiRotor.h"
#include "Vehicles/Car/SimModeCar.h"
#include "Vehicles/ComputerVision/SimModeComputerVision.h"

#include "common/AirSimSettings.hpp"
#include <stdexcept>


ASimHUD::ASimHUD()
{
    static ConstructorHelpers::FClassFinder<UUserWidget> hudWidgetClassFinder(TEXT("WidgetBlueprint'/AirSim/Blueprints/BP_SimHUDWidget'"));
    HUDWidgetClass = hudWidgetClassFinder.Succeeded() ? hudWidgetClassFinder.Class : nullptr;

	static ConstructorHelpers::FClassFinder<UUserWidget> pauseMenuClassFinder(TEXT("WidgetBlueprint'/AirSim/Blueprints/Widgets/WBP_PauseMenu'"));
	PauseMenuClass = pauseMenuClassFinder.Succeeded() ? pauseMenuClassFinder.Class : nullptr;
}

void ASimHUD::BeginPlay()
{
    Super::BeginPlay();

    try {
		UAirBlueprintLib::InitilizeAirSimSettings();

        SetUnrealEngineSettings();

        CreateSimMode();

        CreateHUDWidget();

        SetupInputBindings();

        if (SimMode)
            SimMode->StartApiServer();
    }
    catch (std::exception& ex) {
        UAirBlueprintLib::LogMessageString("Error at startup: ", ex.what(), LogDebugLevel::Failure);
        //FGenericPlatformMisc::PlatformInit();
        //FGenericPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Error at Startup"), ANSI_TO_TCHAR(ex.what()));
        UAirBlueprintLib::ShowMessage(EAppMsgType::Ok, std::string("Error at startup: ") + ex.what(), "Error");
    }
}

void ASimHUD::Tick(float DeltaSeconds)
{
    if (SimMode && SimMode->EnableReport)
        HUDWidget->updateDebugReport(SimMode->GetDebugReport());
}

void ASimHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (SimMode)
        SimMode->StopApiServer();

    if (HUDWidget) {
        HUDWidget->Destruct();
        HUDWidget = nullptr;
    }

    if (SimMode) {
        SimMode->Destroy();
        SimMode = nullptr;
    }

    UAirBlueprintLib::OnEndPlay();

    Super::EndPlay(EndPlayReason);
}

// Begin Play Setup Functions

void ASimHUD::SetUnrealEngineSettings()
{
	//TODO: should we only do below on SceneCapture2D components and cameras?
	//avoid motion blur so capture images don't get
	GetWorld()->GetGameViewport()->GetEngineShowFlags()->SetMotionBlur(false);

	//use two different methods to set console var because sometime it doesn't seem to work
	static const auto custom_depth_var = IConsoleManager::Get().FindConsoleVariable(TEXT("r.CustomDepth"));
	custom_depth_var->Set(3);

	//Equivalent to enabling Custom Stencil in Project > Settings > Rendering > Postprocessing
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), FString("r.CustomDepth 3"));

	//during startup we init stencil IDs to random hash and it takes long time for large environments
	//we get error that GameThread has timed out after 30 sec waiting on render thread
	static const auto render_timeout_var = IConsoleManager::Get().FindConsoleVariable(TEXT("g.TimeoutForBlockOnRenderFence"));
	render_timeout_var->Set(300000);
}

void ASimHUD::CreateSimMode()
{
	std::string simmode_name = AirSimSettings::singleton().simmode_name;

	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	//spawn at origin. We will use this to do global NED transforms, for ex, non-vehicle objects in environment
	if (simmode_name == AirSimSettings::kSimModeTypeMultirotor)
		SimMode = GetWorld()->SpawnActor<ASimModeWorldMultiRotor>(FVector::ZeroVector, FRotator::ZeroRotator, spawnParams);
	else if (simmode_name == AirSimSettings::kSimModeTypeCar)
		SimMode = GetWorld()->SpawnActor<ASimModeCar>(FVector::ZeroVector, FRotator::ZeroRotator, spawnParams);
	else if (simmode_name == AirSimSettings::kSimModeTypeComputerVision)
		SimMode = GetWorld()->SpawnActor<ASimModeComputerVision>(FVector::ZeroVector, FRotator::ZeroRotator, spawnParams);
	else {
		UAirBlueprintLib::ShowMessage(EAppMsgType::Ok, std::string("SimMode is not valid: ") + simmode_name, "Error");
		UAirBlueprintLib::LogMessageString("SimMode is not valid: ", simmode_name, LogDebugLevel::Failure);
	}
}

void ASimHUD::CreateHUDWidget()
{
	//create main widget
	if (HUDWidgetClass != nullptr) {
		APlayerController* player_controller = this->GetWorld()->GetFirstPlayerController();
		auto* pawn = player_controller->GetPawn();
		if (pawn) {
			std::string pawn_name = std::string(TCHAR_TO_ANSI(*pawn->GetName()));
			common_utils::Utils::log(pawn_name);
		}
		else {
			UAirBlueprintLib::ShowMessage(EAppMsgType::Ok, std::string("There were no compatible vehicles created for current SimMode! Check your settings.json."), "Error");
			UAirBlueprintLib::LogMessage(TEXT("There were no compatible vehicles created for current SimMode! Check your settings.json."), TEXT(""), LogDebugLevel::Failure);
		}

		HUDWidget = CreateWidget<USimHUDWidget>(player_controller, HUDWidgetClass);
	}
	else {
		HUDWidget = nullptr;
		UAirBlueprintLib::LogMessage(TEXT("Cannot instantiate BP_SimHUDWidget blueprint!"), TEXT(""), LogDebugLevel::Failure);
	}

	InitializeSubWindows();

	HUDWidget->AddToViewport();

	//synchronize PIP views
	HUDWidget->initializeForPlay();
	if (SimMode)
		HUDWidget->setReportVisible(SimMode->EnableReport);
	HUDWidget->setOnToggleRecordingHandler(std::bind(&ASimHUD::ToggleRecordHandler, this));
	HUDWidget->setRecordButtonVisibility(AirSimSettings::singleton().is_record_ui_visible);
	UpdateWidgetSubwindowVisibility();
}

void ASimHUD::InitializeSubWindows()
{
	if (!SimMode)
		return;

	auto default_vehicle_sim_api = SimMode->GetVehicleSimApi();

	if (default_vehicle_sim_api) {
		auto camera_count = default_vehicle_sim_api->getCameraCount();

		//setup defaults
		if (camera_count > 0) {
			SubwindowCameras[0] = default_vehicle_sim_api->getCamera("");
			SubwindowCameras[1] = default_vehicle_sim_api->getCamera(""); //camera_count > 3 ? 3 : 0
			SubwindowCameras[2] = default_vehicle_sim_api->getCamera(""); //camera_count > 4 ? 4 : 0
		}
		else
			SubwindowCameras[0] = SubwindowCameras[1] = SubwindowCameras[2] = nullptr;
	}

	for (size_t window_index = 0; window_index < AirSimSettings::kSubwindowCount; ++window_index) {

		const auto& subwindow_setting = AirSimSettings::singleton().subwindow_settings.at(window_index);
		auto vehicle_sim_api = SimMode->GetVehicleSimApi(subwindow_setting.vehicle_name);

		if (vehicle_sim_api) {
			if (vehicle_sim_api->getCamera(subwindow_setting.camera_name) != nullptr)
				SubwindowCameras[subwindow_setting.window_index] = vehicle_sim_api->getCamera(subwindow_setting.camera_name);
			else
				UAirBlueprintLib::LogMessageString("CameraID in <SubWindows> element in settings.json is invalid",
					std::to_string(window_index), LogDebugLevel::Failure);
		}
		else
			UAirBlueprintLib::LogMessageString("Vehicle in <SubWindows> element in settings.json is invalid",
				std::to_string(window_index), LogDebugLevel::Failure);

	}
}

void ASimHUD::SetupInputBindings()
{
	APlayerController* controller = GetOwningPlayerController();
	if (controller)
	{
		controller->InputComponent->BindAction("ToggleRecording", IE_Pressed, this, &ASimHUD::OnToggleRecording);
		controller->InputComponent->BindAction("ToggleReport", IE_Pressed, this, &ASimHUD::OnToggleReport);
		controller->InputComponent->BindAction("ToggleHelp", IE_Pressed, this, &ASimHUD::OnToggleHelp);
		controller->InputComponent->BindAction("ToggleTrace", IE_Pressed, this, &ASimHUD::OnToggleTrace);
		controller->InputComponent->BindAction("ToggleSubwindow0", IE_Pressed, this, &ASimHUD::OnToggleSubwindow0);
		controller->InputComponent->BindAction("ToggleSubwindow1", IE_Pressed, this, &ASimHUD::OnToggleSubwindow1);
		controller->InputComponent->BindAction("ToggleSubwindow2", IE_Pressed, this, &ASimHUD::OnToggleSubwindow2);
		controller->InputComponent->BindAction("ToggleAllSubwindows", IE_Pressed, this, &ASimHUD::OnToggleAllSubwindows);
		controller->InputComponent->BindAction("Pause", IE_Pressed, this, &ASimHUD::OnPause);
	}
}

// Input Bindings

void ASimHUD::OnToggleRecording()
{
	ToggleRecordHandler();
}

void ASimHUD::OnToggleReport()
{
	SimMode->EnableReport = !SimMode->EnableReport;
	HUDWidget->setReportVisible(SimMode->EnableReport);
}

void ASimHUD::OnToggleHelp()
{
	HUDWidget->toggleHelpVisibility();
}

void ASimHUD::OnToggleTrace()
{
	SimMode->GetVehicleSimApi()->toggleTrace();
}

void ASimHUD::OnToggleSubwindow0()
{
	GetSubWindowSettings().at(0).visible = !GetSubWindowSettings().at(0).visible;
	UpdateWidgetSubwindowVisibility();
}

void ASimHUD::OnToggleSubwindow1()
{
	GetSubWindowSettings().at(1).visible = !GetSubWindowSettings().at(1).visible;
	UpdateWidgetSubwindowVisibility();
}

void ASimHUD::OnToggleSubwindow2()
{
	GetSubWindowSettings().at(2).visible = !GetSubWindowSettings().at(2).visible;
	UpdateWidgetSubwindowVisibility();
}

void ASimHUD::OnToggleAllSubwindows()
{
	GetSubWindowSettings().at(0).visible = !GetSubWindowSettings().at(0).visible;
	GetSubWindowSettings().at(1).visible = GetSubWindowSettings().at(2).visible = GetSubWindowSettings().at(0).visible;
	UpdateWidgetSubwindowVisibility();
}

void ASimHUD::OnPause()
{
	if (UGameplayStatics::IsGamePaused(this))
	{
		UnpauseSimulation();
	}
	else {
		PauseSimulation();
	}
}

void ASimHUD::ToggleRecordHandler()
{
	SimMode->ToggleRecording();
}

void ASimHUD::UpdateWidgetSubwindowVisibility()
{
	for (int window_index = 0; window_index < AirSimSettings::kSubwindowCount; ++window_index) {
		APIPCamera* camera = SubwindowCameras[window_index];
		msr::airlib::ImageCaptureBase::ImageType camera_type = GetSubWindowSettings().at(window_index).image_type;

		bool is_visible = GetSubWindowSettings().at(window_index).visible && camera != nullptr;

		if (camera != nullptr)
			camera->setCameraTypeEnabled(camera_type, is_visible);

		HUDWidget->setSubwindowVisibility(window_index,
			is_visible,
			is_visible ? camera->getRenderTarget(camera_type, false) : nullptr
		);
	}
}

std::vector<ASimHUD::AirSimSettings::SubwindowSetting>& ASimHUD::GetSubWindowSettings()
{
    return AirSimSettings::singleton().subwindow_settings;
}

void ASimHUD::PauseSimulation()
{
	if (!IsValid(PauseMenu))
	{
		PauseMenu = CreateWidget<UUserWidget>(GetWorld(), PauseMenuClass);
	}

	APlayerController* ctrl = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ctrl->bShowMouseCursor = true;
	ctrl->SetInputMode(FInputModeUIOnly());

	PauseMenu->AddToViewport();

	//UGameplayStatics::SetGamePaused(this, true);
	SimMode->PauseSimulation(true);
}

void ASimHUD::UnpauseSimulation()
{
	if (IsValid(PauseMenu))
	{
		APlayerController* ctrl = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		ctrl->bShowMouseCursor = false;
		ctrl->SetInputMode(FInputModeGameOnly());

		PauseMenu->RemoveFromViewport();

		//UGameplayStatics::SetGamePaused(this, false);
		SimMode->PauseSimulation(false);
	}
}