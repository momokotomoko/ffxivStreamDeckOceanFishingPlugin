//==============================================================================
/**
@file       FFXIVOceanFishingProcessor.h
@brief      Computes Ocean Fishing Times
@copyright  (c) 2023, Momoko Tomoko
**/
//==============================================================================

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <map>
#include <unordered_set>
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
	std::string getErrorMessage() { return mErrorMessage.value_or(""); };
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
	std::optional<std::string> mErrorMessage;

	std::unordered_map<std::string, std::string> mStops;
	std::unordered_map<std::string, std::unordered_map<std::string, fish_t>> mFishes;
	std::unordered_map<std::string, std::unordered_set<uint32_t>> mAchievements;
	std::set<std::string> mBlueFishNames;

	std::unordered_map <std::string, voyage_t> mVoyages;

	uint32_t mPatternOffset = 0;
	std::vector<uint32_t> mVoyagePattern;

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
};