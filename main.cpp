#include <windows.h>
#include <mutex>
#include <filesystem>
#include <format>
#include <toml++/toml.hpp>

#include "nya_dx9_hookbase.h"
#include "nya_commonhooklib.h"
#include "nya_commonmath.h"
#include "nfsps.h"
#include "chloemenulib.h"

bool bCareerMode = false;
bool bChallengeSeriesMode = false;

#include "util.h"
#include "d3dhook.h"
#include "../MostWantedTimeTrialGhosts/timetrials.h"
#include "../MostWantedTimeTrialGhosts/verification.h"
#include "hooks/carrender.h"

#include "challengeseries.h"

RideInfo* pPlayerRideInfo = nullptr;
ISimable* VehicleConstructHooked(Sim::Param params) {
	DLLDirSetter _setdir;

	auto vehicle = (VehicleParams*)params.mSource;
	if (vehicle->carClass == DRIVER_HUMAN) {
		pPlayerRideInfo = vehicle->rideInfo;
	}
	if (vehicle->carClass == DRIVER_RACER) {
		// copy player car for all opponents
		auto player = GetLocalPlayerVehicle();
		vehicle->matched = nullptr;
		vehicle->pvehicle = Attrib::Instance(Attrib::FindCollection(Attrib::StringHash32("pvehicle"), player->GetVehicleKey()), 0);
		vehicle->rideInfo = pPlayerRideInfo;

		// do a config save in every loading screen
		DoConfigSave();
	}
	return PVehicle::Construct(params);
}

bool __thiscall DeterminePlatformSceneHooked(void* pThis, char* out) {
	strcpy(out, "PracticeGeneric");
	return true;
}

ChallengeSeriesEvent* pEventToStart = nullptr;
void MainLoop() {
	if (pEventToStart) {
		pEventToStart->SetupEvent();
		pEventToStart = nullptr;
	}
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x16AA080) {
				MessageBoxA(nullptr, "Unsupported game version! Make sure you're using v1.1 (.exe size of 3765248 bytes)", "nya?!~", MB_ICONERROR);
				return TRUE;
			}

			ChloeMenuLib::RegisterMenu("Time Trial Ghosts", &DebugMenu);

			NyaHooks::SkipFEFixes::Init();

			NyaHooks::SimServiceHook::Init();
			NyaHooks::SimServiceHook::aFunctions.push_back(MainLoop);
			NyaHooks::LateInitHook::Init();
			NyaHooks::LateInitHook::aPreFunctions.push_back(FileIntegrity::VerifyGameFiles);
			NyaHooks::LateInitHook::aFunctions.push_back([]() {
				NyaHooks::PlaceD3DHooks();
				NyaHooks::D3DEndSceneHook::aPreFunctions.push_back(CollectPlayerPos);
				NyaHooks::D3DEndSceneHook::aFunctions.push_back(D3DHookMain);
				NyaHooks::D3DEndSceneHook::aFunctions.push_back(CheckPlayerPos);
				NyaHooks::D3DResetHook::aFunctions.push_back(OnD3DReset);

				ApplyVerificationPatches();

				*(double*)0x9FABF8 = 1.0 / 120.0; // set sim framerate
				*(void**)0xACDDD4 = (void*)&VehicleConstructHooked;
				if (GetModuleHandleA("NFSPSExtraOptions.asi") || std::filesystem::exists("NFSPSExtraOptionsSettings.ini") || std::filesystem::exists("scripts/NFSPSExtraOptionsSettings.ini")) {
					MessageBoxA(nullptr, "Potential unfair advantage detected! Please remove NFSPSExtraOptions from your game before using this mod.", "nya?!~", MB_ICONERROR);
					exit(0);
				}
			});

			//NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4E1610, &DeterminePlatformSceneHooked);

			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x5BEA1E, 0x6DBC10); // ReloadTrack instead of UnloadTrack, for the totaled prompt

			SkipFE = true;
			SkipFEForever = true;
			SkipFEPlayerCar = "player_d_day";
			SkipFETrackNumber = 6000;

			SkipFETractionControlLevel = 0;
			SkipFEStabilityControlLevel = 0;
			SkipFEAntiLockBrakesLevel = 0;
			SkipFEDriftAssistLevel = 0;
			SkipFERacelineAssistLevel = 0;
			SkipFEBrakingAssistLevel = 0;

			ApplyCarRenderHooks();
		} break;
		default:
			break;
	}
	return TRUE;
}