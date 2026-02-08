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
bool bChallengeSeriesMode = true;

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

auto OnEventFinished_orig = (void(__thiscall*)(GRaceStatus*, int))nullptr;
void __thiscall OnEventFinished(GRaceStatus* a1, int a2) {
	OnEventFinished_orig(a1, a2);

	auto veh = GetLocalPlayerVehicle();
	if (veh->IsDestroyed()) return;
	if (veh->mCOMObject->Find<IDamageable>()->IsDestroyed()) return;
	if (GetOpponentDamage(GetLocalPlayerSimable()) >= Tweak_TotalledDamage) return;

	auto racer = GRaceStatus::GetRacerInfo(GRaceStatus::fObj, GetLocalPlayerSimable());
	if (racer->mOpponent->GetDamageSeverity() >= GIOpponent::kDamageSeverity_Totalled) return;
	if (racer->mOpponent->GetDamageLevel() >= Tweak_TotalledDamage) return;

	DLLDirSetter _setdir;
	OnFinishRace();

	// do a config save when finishing a race
	DoConfigSave();
}

auto OnRestartRace_orig = (void(__thiscall*)(void*))nullptr;
void __thiscall OnRestartRace(void* a1) {
	OnRestartRace_orig(a1);
	OnRaceRestart();
}

void MainLoop() {
	static double simTime = -1;

	if (!Sim::Exists() || Sim::GetState() != Sim::STATE_ACTIVE) {
		simTime = -1;
		return;
	}
	if (simTime == Sim::GetTime()) return;
	simTime = Sim::GetTime();

	DLLDirSetter _setdir;
	TimeTrialLoop();

	if (pEventToStart) {
		pEventToStart->SetupEvent();
		pEventToStart = nullptr;
	}
}

void RenderLoop() {
	VerifyTimers();
	TimeTrialRenderLoop();
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x16AA080 && NyaHookLib::GetEntryPoint() != 0x428C25) {
				MessageBoxA(nullptr, "Unsupported game version! Make sure you're using v1.1 (.exe size of 3765248 or 28739656 bytes)", "nya?!~", MB_ICONERROR);
				return TRUE;
			}

			gDLLPath = std::filesystem::current_path();
			GetCurrentDirectoryW(MAX_PATH, gDLLDir);

			ChloeMenuLib::RegisterMenu("Time Trial Ghosts", &DebugMenu);

			if (std::filesystem::exists("NFSPSTimeTrialGhosts_gcp.toml")) {
				auto config = toml::parse_file("NFSPSTimeTrialGhosts_gcp.toml");
				if (config["tank_unslapper"].value_or(false)) {
					NyaHookLib::Fill(0x49296F + 2, 0x90, 4);
					NyaHookLib::Fill(0x494059 + 2, 0x90, 4);
					NyaHookLib::Patch<uint16_t>(0x49296F, 0xEED9);
					NyaHookLib::Patch<uint16_t>(0x494059, 0xEED9);
				}
			}

			NyaHooks::SkipFEFixes::Init();

			NyaHooks::SimServiceHook::Init();
			NyaHooks::SimServiceHook::aFunctions.push_back(MainLoop);
			NyaHooks::LateInitHook::Init();
			NyaHooks::LateInitHook::aPreFunctions.push_back(FileIntegrity::VerifyGameFiles);
			NyaHooks::LateInitHook::aFunctions.push_back([]() {
				NyaHooks::PlaceD3DHooks();
				NyaHooks::WorldServiceHook::aPreFunctions.push_back(CollectPlayerPos);
				NyaHooks::WorldServiceHook::aPostFunctions.push_back(CheckPlayerPos);
				NyaHooks::D3DEndSceneHook::aFunctions.push_back(D3DHookMain);
				NyaHooks::D3DResetHook::aFunctions.push_back(OnD3DReset);

				ApplyVerificationPatches();

				*(double*)0x9FABF8 = 1.0 / 120.0; // set sim framerate
				*(void**)0xACDDD4 = (void*)&VehicleConstructHooked;
				if (GetModuleHandleA("NFSPSExtraOptions.asi") || std::filesystem::exists("NFSPSExtraOptionsSettings.ini") || std::filesystem::exists("scripts/NFSPSExtraOptionsSettings.ini")) {
					MessageBoxA(nullptr, "Potential unfair advantage detected! Please remove NFSPSExtraOptions from your game before using this mod.", "nya?!~", MB_ICONERROR);
					exit(0);
				}
			});

			OnEventFinished_orig = (void(__thiscall*)(GRaceStatus*, int))(*(uint32_t*)(0x6F68BB + 1));
			NyaHookLib::Patch(0x6F68BB + 1, &OnEventFinished);

			//NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4E1610, &DeterminePlatformSceneHooked);

			OnRestartRace_orig = (void(__thiscall*)(void*))NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x6B4613, &OnRestartRace);

			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x5BEA1E, 0x6DBC10); // ReloadTrack instead of UnloadTrack, for the totaled prompt

			SetRacerAIEnabled(false);

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

			DoConfigLoad();

			WriteLog("Mod initialized");
		} break;
		default:
			break;
	}
	return TRUE;
}