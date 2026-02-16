#include <windows.h>
#include <mutex>
#include <filesystem>
#include <format>
#include <thread>
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

	if (auto ply = GetLocalPlayerVehicle()) {
		auto profile = &UserProfile::spUserProfiles[0]->mOptionsSettings;
		profile->TheGameplaySettings.NamesAboveOn = 0;
		gIngameSettings.SpeedoUnits = profile->TheGameplaySettings.SpeedoUnits;
		gIngameSettings.MasterVol = profile->TheAudioSettings.MasterVol;
		gIngameSettings.SpeechVol = profile->TheAudioSettings.SpeechVol;
		gIngameSettings.FEMusicVol = profile->TheAudioSettings.FEMusicVol;
		gIngameSettings.IGMusicVol = profile->TheAudioSettings.IGMusicVol;
		gIngameSettings.SoundEffectsVol = profile->TheAudioSettings.SoundEffectsVol;
		gIngameSettings.EngineVol = profile->TheAudioSettings.EngineVol;
		gIngameSettings.CarVol = profile->TheAudioSettings.CarVol;
		gIngameSettings.AmbientVol = profile->TheAudioSettings.AmbientVol;
		gIngameSettings.SpeedVol = profile->TheAudioSettings.SpeedVol;
		gIngameSettings.EATraxMode = profile->TheAudioSettings.EATraxMode;

		if (auto settings = GetLocalPlayer()->GetSettings()) {
			settings->BestLineOn = gIngameSettings.BestLineOn;
			settings->Transmission = gIngameSettings.Transmission;
			settings->GripTransmission = gIngameSettings.Transmission;
			settings->DriftTransmission = gIngameSettings.Transmission;
			settings->SpeedTransmission = gIngameSettings.Transmission;
		}

		for (int i = 0; i < NUM_DRIVER_AIDS; i++) {
			if (ply->GetDriverAidLevel((DriverAidType)i) == 0) continue;
			ply->SetDriverAidLevel((DriverAidType)i, 0, true);
		}

		if (auto racer = GRaceStatus::GetRacerInfo(GRaceStatus::fObj, GetLocalPlayerSimable())) {
			if (ply->IsStaging()) {
				racer->mOpponent->RepairDamage();
			}
			racer->mStats.local.mDriftFactor = 1.0;
		}

		auto cars = GetActiveVehicles();
		for (auto& car : cars) {
			car->SetDraftZoneModifier(0.0);
		}
	}

	if (pEventToStart) {
		pEventToStart->SetupEvent();
		pEventToStart = nullptr;
	}
}

void RenderLoop() {
	VerifyTimers();
	TimeTrialRenderLoop();
}

auto InputTest_orig = (void(__stdcall*)(void*, void*))nullptr;
void __stdcall InputTest(void* a1, void* a2) {
	InputTest_orig(a1, a2);
	if (auto ply = GetLocalPlayerInterface<IInput>()) {
		//if (GRaceStatus::fObj && GRaceStatus::fObj->mRaceParms) {
		//	if (GRaceStatus::fObj->mRaceMasterTimer.GetTime() < 2) ply->SetControlBrake(0);
		//}
		//ply->SetControlBrake(1 - ply->GetControlBrake());
		//ply->SetControlGas(1 - ply->GetControlGas());

		if (ply->GetControlGas() > 0.85) {
			ply->SetControlGas(1.0);
		}
		//ply->SetControlClutch(0.0);
	}
}

// enable resetting at any speed if race restart on reset is on
float ResetSpeedHooked(float a1) {
	if (bFastRestart) return 0.0;
	return std::abs(a1);
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
				if (config["gas_pedal_hack"].value_or(false)) {
					InputTest_orig = (void(__stdcall*)(void*, void*))NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x41B5A8, &InputTest);
					//NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x41B3FE, &InputTestASM);
				}
			}

			NyaHooks::SkipFEFixes::Init();

			NyaHooks::SimServiceHook::Init();
			NyaHooks::SimServiceHook::aFunctions.push_back(MainLoop);
			NyaHooks::LateInitHook::Init();
			NyaHooks::LateInitHook::aPreFunctions.push_back(FileIntegrity::VerifyGameFiles);
			NyaHooks::LateInitHook::aFunctions.push_back([]() {
				NyaHooks::PlaceD3DHooks();
				NyaHooks::WorldServiceHook::Init();
				NyaHooks::WorldServiceHook::aPreFunctions.push_back(CollectPlayerPos);
				NyaHooks::WorldServiceHook::aPostFunctions.push_back(CheckPlayerPos);
				NyaHooks::D3DEndSceneHook::aFunctions.push_back(D3DHookMain);
				NyaHooks::D3DResetHook::aFunctions.push_back(OnD3DReset);

				NyaHookLib::Patch<double>(0x9FABF8, 1.0 / 120.0); // set sim framerate
				NyaHookLib::Patch<float>(0x9EE934, 1.0 / 120.0); // set sim max framerate

				ApplyVerificationPatches();

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

			NyaHookLib::Patch<uint8_t>(0x6DD308 + 4, 0); // remove steering wheel bonus for drifting

			// disable drafting
			NyaHookLib::Patch<uint8_t>(0x477155, 0xEB);
			NyaHookLib::Patch<uint8_t>(0x4773BF, 0xEB);
			NyaHookLib::Patch<uint8_t>(0x4773DF, 0xEB);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x477440, 0x4775EB);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x477615, 0x4776C8);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x477706, 0x4778B3);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4771E0, 0x4773A5);

			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x41B0EB, &ResetSpeedHooked);

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