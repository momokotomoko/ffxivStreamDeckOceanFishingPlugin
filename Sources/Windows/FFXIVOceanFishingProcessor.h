//==============================================================================
/**
@file       FFXIVOceanFishingProcessor.h
@brief      Computes Ocean Fishing Times
@copyright  (c) 2023, Momoko Tomoko
**/
//==============================================================================

#pragma once

#include <string>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <vector>
#include "Common.h"

#include "../Vendor/json/src/json.hpp"
using json = nlohmann::json;

class FFXIVOceanFishingProcessor
{
public:
	FFXIVOceanFishingProcessor(const std::string& dataFile);
	FFXIVOceanFishingProcessor(const json& j);
	~FFXIVOceanFishingProcessor() {};

	bool isInit() { return mIsInit; };
	std::string getErrorMessage() { return mErrorMessage; };
	std::string getRouteName() { return mRouteName; };

	void loadDatabase(const json& j);

	bool getNextVoyage(uint32_t& nextVoyageId, const time_t& startTime, const std::unordered_set<uint32_t>& voyageIds, const uint32_t skips = 0);
	bool getSecondsUntilNextVoyage(
		uint32_t& secondsTillNextVoyage,
		uint32_t& secondsLeftInWindow,
		uint32_t& nextVoyageId,
		const time_t& startTime,
		const std::unordered_set<uint32_t>& voyageIds,
		const uint32_t skips = 0
	);
	std::string getNextVoyageName(const time_t& t, const uint32_t skips = 0);

	std::unordered_set<uint32_t> getVoyageIdByTracker(const std::string& tracker, const std::string& name);
	void getImageNameAndLabel(
		std::string& imageName,
		std::string& buttonLabel,
		const std::string& tracker,
		const std::string& name,
		const time_t& startTime,
		const PRIORITY priority,
		const uint32_t skips
	);

	json getTargetsJson();
	json getTrackerTypesJson();
private:
	bool mIsInit = false;
	std::string mRouteName;
	std::string mErrorMessage;

	bool isBadKey(const json& j, const std::string& key, const std::string& msg)
	{
		if (!j.contains(key))
		{
			mErrorMessage = msg + "\nJson Dump:\n" + j.dump(4);
			return true;
		}
		return false;
	}

	struct locations_t
	{
		const std::string name;
		const std::vector<std::string> time;
	};

	struct fish_t
	{
		const std::string shortName;
		const std::vector<locations_t> locations;
	};


	std::unordered_map<std::string, std::string> mStops;
	std::unordered_map<std::string, std::unordered_map<std::string, fish_t>> mFishes;
	std::unordered_map<std::string, std::unordered_set<uint32_t>> mAchievements;
	std::map<std::string, fish_t> mBlueFishNames;

	struct stop_t
	{
		const locations_t location;
		const std::unordered_set<std::string> fish;
	};

	struct voyage_t
	{
		const std::string shortName;
		const uint32_t id;
		const std::vector<stop_t> stops;
		const std::set<std::string> achievements;
		std::string blueFishPattern;
	};
	std::unordered_map <std::string, voyage_t> mVoyages;

	uint32_t mPatternOffset = 0;
	std::vector<uint32_t> mVoyagePattern;

	struct targets_t
	{
		const std::string labelName;
		const std::string imageName;
		std::unordered_set <uint32_t> ids;
	};
	// hiearchy is target type -> target name -> struct with vector of voyage ids
	// ie: "Blue Fish" -> "Sothis" -> {shortName, {id1, id2...}}
	std::unordered_map <std::string, std::map<std::string, targets_t>> mTargetToVoyageIdMap;
	std::unordered_map <uint32_t, std::string> mVoyageIdToNameMap;

	time_t convertBlockIndexToTime(const uint32_t blockIdx);
	uint32_t convertTimeToBlockIndex(const time_t& t);
	uint32_t getVoyagePatternIndex(const uint32_t blockIdx, const uint32_t jump = 0);

	std::string createAchievementName(const std::string& voyageName);
	std::string createImageNameFromVoyageId(const uint32_t& voyageId, PRIORITY priority);
	std::string createButtonLabelFromVoyageId(const uint32_t& voyageId, PRIORITY priority);

	// database loading functions
	bool loadSchedule(const json& j);
};