class ChallengeSeriesEvent {
public:
	std::string sTrackName;
	std::string sEventName;
	std::string sCarPreset;
	int nLapCountOverride = 0;

	bool bPBGhostLoading = false;
	bool bTargetGhostLoading = false;
	tReplayGhost PBGhost = {};
	tReplayGhost aTargetGhosts[NUM_DIFFICULTY] = {};
	int nNumGhosts[NUM_DIFFICULTY] = {};

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
		if (sTrackName == "L6R_AutobahnDrift") return "Autobahn";
		if (sTrackName == "L6R_Autopolis") return "Autopolis";
		if (sTrackName == "L6R_ChicagoAirfield") return "Chicago Airfield";
		if (sTrackName == "L6R_Ebisu") return "Ebisu";
		if (sTrackName == "L6R_INFINEON") return "Infineon";
		if (sTrackName == "L6R_LEIPZIG") return "Leipzig";
		if (sTrackName == "L6R_MondelloPark") return "Mondello Park";
		if (sTrackName == "L6R_NevadaDrift") return "Nevada Desert";
		if (sTrackName == "L6R_PortlandRaceway") return "Portland Raceway";
		if (sTrackName == "L6R_ShutoDrift") return "Shuto Drift";
		if (sTrackName == "L6R_ShutoExpressway") return "Shuto Expressway";
		if (sTrackName == "L6R_TexasSpeedway") return "Texas Speedway";
		if (sTrackName == "L6R_WillowSprings") return "Willow Springs";
		return "NULL";
	}

	/*std::string GetTrackName() const {
		if (auto track = TrackInfo::GetTrackInfo(GetTrackID())) {
			return track->Name;
		}
		return "NULL";
	}*/

	std::string GetEventTypeName() const {
		auto baseRace = GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(sEventName.c_str()));
		if (!baseRace) return "NULL";

		switch (GRaceParameters::GetRaceType(baseRace)) {
			case GRace::kRaceType_SpeedChallenge_HeadsUp:
			case GRace::kRaceType_SpeedChallenge_Mixed:
			case GRace::kRaceType_SpeedChallenge_Class:
				return "Speed Challenge";
			case GRace::kRaceType_Circuit_HeadsUp:
			case GRace::kRaceType_Circuit_Mixed:
			case GRace::kRaceType_Circuit_Class:
				return "Grip";
			case GRace::kRaceType_SectorShootout_HeadsUp:
			case GRace::kRaceType_SectorShootout_Mixed:
			case GRace::kRaceType_SectorShootout_Class:
				return "Sector Shootout";
			case GRace::kRaceType_TimeAttack_HeadsUp:
			case GRace::kRaceType_TimeAttack_Mixed:
			case GRace::kRaceType_TimeAttack_Class:
				return "Time Attack";
			case GRace::kRaceType_Drag_HeadsUp:
			case GRace::kRaceType_Drag_Mixed:
			case GRace::kRaceType_Drag_Class:
			case GRace::kRaceType_Drag_Wheelie_HeadsUp:
			case GRace::kRaceType_Drag_Wheelie_Mixed:
			case GRace::kRaceType_Drag_Wheelie_Class:
				return "Drag";
			case GRace::kRaceType_Drift_Solo_HeadsUp:
			case GRace::kRaceType_Drift_Solo_Mixed:
			case GRace::kRaceType_Drift_Solo_Class:
			case GRace::kRaceType_Drift_Race_HeadsUp:
			case GRace::kRaceType_Drift_Race_Mixed:
			case GRace::kRaceType_Drift_Race_Class:
			case GRace::kRaceType_Drift_Tandem_HeadsUp:
			case GRace::kRaceType_Drift_Tandem_Mixed:
			case GRace::kRaceType_Drift_Tandem_Class:
				return "Drift";
			case GRace::kRaceType_Knockout:
				return "Lap Knockout";
			case GRace::kRaceType_SpeedTrap:
				return "Speedtrap";
			case GRace::kRaceType_Checkpoint:
				return "Checkpoint";
			case GRace::kRaceType_Challenge:
				return "Challenge";
			default:
				return "NULL";
		}
	}

	GRaceParameters* GetRace() const {
		return GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(sEventName.c_str()));
	}

	int GetLapCount() {
		if (nLapCountOverride > 0) return nLapCountOverride;
		return nLapCountOverride = GRaceParameters::GetNumLaps(GetRace());
	}

	void ClearPBGhost() {
		PBGhost = {};
	}

	tReplayGhost GetPBGhost() {
		while (bPBGhostLoading) { Sleep(0); }

		if (PBGhost.nFinishTime != 0) return PBGhost;

		bPBGhostLoading = true;
		tReplayGhost temp;
		LoadPB(&temp, sCarPreset, sEventName, GetLapCount(), 0, nullptr);
		temp.aTicks.clear(); // just in case
		PBGhost = temp;
		bPBGhostLoading = false;
		return temp;
	}

	tReplayGhost GetTargetGhost() {
		while (bTargetGhostLoading) { Sleep(0); }

		if (aTargetGhosts[nDifficulty].nFinishTime != 0) return aTargetGhosts[nDifficulty];

		bTargetGhostLoading = true;
		tReplayGhost targetTime;
		auto times = CollectReplayGhosts(sCarPreset, sEventName, GetLapCount(), nullptr);
		if (!times.empty()) {
			times[0].aTicks.clear(); // just in case
			targetTime = aTargetGhosts[nDifficulty] = times[0];
		}
		nNumGhosts[nDifficulty] = times.size();
		bTargetGhostLoading = false;
		return targetTime;
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
	ChallengeSeriesEvent("L6R_AutobahnDrift", "14.gr.1", "grip_king"),
	ChallengeSeriesEvent("L6R_Autopolis", "19.gr.2", "grip_king"),
	ChallengeSeriesEvent("L6R_AutobahnDrift", "34.td.1", "drift_king"),
	ChallengeSeriesEvent("L6R_LEIPZIG", "lg.9.1.6", "showdown_entourage_2_drift"),
	ChallengeSeriesEvent("L6R_NevadaDrift", "82.hs.2", "sc_king"),
	ChallengeSeriesEvent("L6R_ChicagoAirfield", "7.gr.1", "showdown_king_final_grip"),
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
	event->ClearPBGhost();
}

ChallengeSeriesEvent* pEventToStart = nullptr;
void ChallengeSeriesMenu() {
	for (auto& event : aNewChallengeSeries) {
		auto pb = event.GetPBGhost();
		auto target = event.GetTargetGhost();
		auto optionName = std::format("{} - {}", event.GetEventTypeName(), event.GetTrackName());

		auto targetName = GetRealPlayerName(target.sPlayerName);
		auto targetTime = std::format("Target Time - {} ({})", FormatTime(target.nFinishTime), targetName);
		if (event.GetEventTypeName() == "Drift") {
			targetTime = std::format("Target - {} ({})", FormatScore(target.nFinishPoints), targetName);
		}

		if (pb.nFinishTime != 0) {
			bool won = pb.nFinishTime <= target.nFinishTime;
			if (pb.nFinishPoints && target.nFinishPoints) {
				won = pb.nFinishPoints >= target.nFinishPoints;
			}
			if (won) {
				optionName += std::format(" - Completed");
			}
		}

		if (DrawMenuOption(optionName, targetTime)) {
			ChloeMenuLib::BeginMenu();
			DrawMenuOption(std::format("Track - {}", event.GetTrackName()));
			DrawMenuOption(std::format("Type - {}", event.GetEventTypeName()));
			DrawMenuOption(std::format("Car - {}", event.sCarPreset));
			DrawMenuOption(targetTime);
			if (pb.nFinishTime != 0) {
				DrawMenuOption(std::format("Personal Best - {}", event.GetEventTypeName() == "Drift" ? FormatScore(pb.nFinishPoints) : FormatTime(pb.nFinishTime)));
			}
			if (DrawMenuOption("Launch Event")) {
				pEventToStart = &event;
			}
			ChloeMenuLib::EndMenu();
		}
	}
}