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
		if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) return;

		RaceParameters::InitWithDefaults(&TheRaceParameters);
		SkipFETrackNumber = TheRaceParameters.TrackNumber = GetTrackID();
		SkipFENumAICars = bChallengesOneGhostOnly ? 1 : 7;
		SkipFERaceID = sEventName.c_str();

		/*auto baseRace = GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(SkipFERaceID));
		auto race = GRaceDatabase::AllocCustomRace(GRaceDatabase::mObj, baseRace);

		GRaceCustom::SetNumLaps(race, GRaceParameters::GetIsLoopingRace(baseRace) ? 2 : 1);
		GRaceCustom::SetIsPracticeMode(race, false);
		GRaceCustom::SetIsSlotcarRace(race, false);
		GRaceCustom::SetTrafficDensity(race, 0.0);
		GRaceCustom::SetNumOpponents(race, SkipFENumAICars);
		GRaceCustom::SetDifficulty(race, GRace::kRaceDifficulty_Insane);
		GRaceCustom::SetCopsEnabled(race, false);

		GRaceDatabase::SetStartupRace(GRaceDatabase::mObj, race, GRace::kRaceContext_QuickRace);
		GRaceDatabase::FreeCustomRace(GRaceDatabase::mObj, race);*/

		SkipFEPlayerCar = sCarPreset.c_str();
		NyaHookLib::Fill(0x4D4B86, 0x90, 6);

		GameFlowManager::ReloadTrack(&TheGameFlowManager);
	}
};

std::vector<ChallengeSeriesEvent> aNewChallengeSeries = {
		ChallengeSeriesEvent("L6R_ChicagoAirfield", "1.gr.1", "player_d_day"),
};

ChallengeSeriesEvent* GetChallengeEvent(uint32_t hash) {
	for (auto& event : aNewChallengeSeries) {
		if (!GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(event.sEventName.c_str()))) {
			MessageBoxA(0, std::format("Failed to find event {}", event.sEventName).c_str(), "nya?!~", MB_ICONERROR);
			exit(0);
		}
	}
	for (auto& event : aNewChallengeSeries) {
		if (Attrib::StringHash32(event.sEventName.c_str()) == hash) return &event;
	}
	return nullptr;
}

ChallengeSeriesEvent* GetChallengeEvent(const std::string& str) {
	for (auto& event : aNewChallengeSeries) {
		if (event.sEventName == str) return &event;
	}
	return nullptr;
}

void OnChallengeSeriesEventPB() {
	auto event = GetChallengeEvent(GRaceParameters::GetEventID(GRaceStatus::fObj->mRaceParms));
	if (!event) return;
	//event->ClearPBGhost();
}

ChallengeSeriesEvent* pEventToStart = nullptr;
void ChallengeSeriesMenu() {
	for (auto& event : aNewChallengeSeries) {
		if (DrawMenuOption(std::format("{} - {}", event.GetTrackName(), event.sEventName))) {
			pEventToStart = &event;
		}
	}
}