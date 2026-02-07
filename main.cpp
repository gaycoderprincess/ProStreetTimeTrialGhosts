#include <windows.h>
#include <mutex>
#include <filesystem>
#include <format>
#include <toml++/toml.hpp>

#include "nya_commonhooklib.h"
#include "nfsps.h"

#include "include/chloemenulib.h"

bool __thiscall DeterminePlatformSceneHooked(void* pThis, char* out) {
	strcpy(out, "PracticeGeneric");
	return true;
}

class ChallengeSeriesEvent {
public:
	std::string sTrackName;
	std::string sEventName;
	std::string sCarPreset;
	int nLapCountOverride = 0;

	ChallengeSeriesEvent(const char* trackName, const char* eventName, const char* carPreset, int lapCount = 0) : sTrackName(trackName), sEventName(eventName), sCarPreset(carPreset), nLapCountOverride(lapCount) {}

	int GetTrackID() const {
		for (int i = 0; i < TrackInfo::NumTrackInfo; i++) {
			if (sTrackName == TrackInfo::TrackInfoTable[i].RegionName) {
				return TrackInfo::TrackInfoTable[i].TrackNumber;
			}
		}
		MessageBoxA(nullptr, std::format("Failed to find track {}", sTrackName).c_str(), "nya?!~", MB_ICONERROR);
		return -1;
	}

	std::string GetTrackName() const {
		if (auto track = TrackInfo::GetTrackInfo(GetTrackID())) {
			return track->Name;
		}
		return "NULL";
	}

	void SetupEvent() const {
		RaceParameters::InitWithDefaults(&TheRaceParameters);
		SkipFETrackNumber = TheRaceParameters.TrackNumber = GetTrackID();
		SkipFENumAICars = 7;
		SkipFERaceID = sEventName.c_str();

		auto baseRace = GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(SkipFERaceID));
		auto race = GRaceDatabase::AllocCustomRace(GRaceDatabase::mObj, baseRace);

		GRaceCustom::SetNumLaps(race, GRaceParameters::GetIsLoopingRace(baseRace) ? 2 : 1);
		GRaceCustom::SetIsPracticeMode(race, false);
		GRaceCustom::SetIsSlotcarRace(race, false);
		GRaceCustom::SetTrafficDensity(race, 0.0);
		GRaceCustom::SetNumOpponents(race, SkipFENumAICars);
		GRaceCustom::SetDifficulty(race, GRace::kRaceDifficulty_Insane);
		GRaceCustom::SetCopsEnabled(race, false);

		GRaceDatabase::SetStartupRace(GRaceDatabase::mObj, race, GRace::kRaceContext_QuickRace);
		GRaceDatabase::FreeCustomRace(GRaceDatabase::mObj, race);

		SkipFEPlayerCar = sCarPreset.c_str();
		NyaHookLib::Fill(0x4D4B86, 0x90, 6);

		GameFlowManager::ReloadTrack(&TheGameFlowManager);
	}
};

std::vector<ChallengeSeriesEvent> aNewChallengeSeries = {
	ChallengeSeriesEvent("L6R_ChicagoAirfield", "1.gr.1", "player_d_day"),
};

ChallengeSeriesEvent* pEventToStart = nullptr;
void MainLoop() {
	if (pEventToStart) {
		pEventToStart->SetupEvent();
		pEventToStart = nullptr;
	}
}

void DebugMenu() {
	ChloeMenuLib::BeginMenu();

	for (auto& event : aNewChallengeSeries) {
		if (DrawMenuOption(std::format("{} - {}", event.GetTrackName(), event.sEventName))) {
			pEventToStart = &event;
		}
	}

	ChloeMenuLib::EndMenu();
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
			NyaHooks::WorldServiceHook::Init();
			NyaHooks::WorldServiceHook::aFunctions.push_back(MainLoop);

			//NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4E1610, &DeterminePlatformSceneHooked);

			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x5BEA1E, 0x6DBC10); // ReloadTrack instead of UnloadTrack

			SkipFE = true;
			SkipFEForever = true;
			SkipFEPlayerCar = "player_d_day";
			//SkipFERaceID = "75.sd.1";
			//SkipFERaceID = "75.gr.1";
			//SkipFERaceID = "1.gr.1";
			SkipFETrackNumber = 6000;
			//SkipFETrackNumber = 6170;

			SkipFETractionControlLevel = 0;
			SkipFEStabilityControlLevel = 0;
			SkipFEAntiLockBrakesLevel = 0;
			SkipFEDriftAssistLevel = 0;
			SkipFERacelineAssistLevel = 0;
			SkipFEBrakingAssistLevel = 0;
		} break;
		default:
			break;
	}
	return TRUE;
}