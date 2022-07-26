//==============================================================================
/**
@file       FFXIVOceanFishingHelper.h
@brief      Computes Ocean Fishing Times
@copyright  (c) 2020, Momoko Tomoko
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

class FFXIVOceanFishingHelper
{
public:
	FFXIVOceanFishingHelper();
	FFXIVOceanFishingHelper(const std::string& dataFile);
	~FFXIVOceanFishingHelper() {};

	void loadDatabase(const std::string& dataFile);

	bool getNextRoute(uint32_t& nextRoute, const time_t& startTime, const std::unordered_set<uint32_t>& routeIds, const uint32_t skips = 0);
	bool getSecondsUntilNextRoute(int& secondsTillNextRoute, int& secondsLeftInWindow, uint32_t& nextRoute, const time_t& startTime, const std::unordered_set<uint32_t>& routeIds, const uint32_t skips = 0);
	std::string getNextRouteName(const time_t& t, const unsigned int skips = 0);

	std::unordered_set<uint32_t> getRouteIdByTracker(const std::string& tracker, const std::string& name);
	void getImageNameAndLabel(std::string& imageName, std::string& buttonLabel, const std::string& tracker, const std::string& name, const PRIORITY priority, const uint32_t skips);
	json getTargetsJson();
	json getTrackerTypesJson();
private:
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

	struct route_t
	{
		const std::string shortName;
		const uint32_t id;
		const std::vector<stop_t> stops;
		const std::unordered_set<std::string> achievements;
		std::string blueFishPattern;
	};
	std::unordered_map <std::string, route_t> mRoutes;

	uint32_t mPatternOffset;
	std::vector<uint32_t> mRoutePattern;

	struct targets_t
	{
		const std::string labelName;
		const std::string imageName;
		std::unordered_set <uint32_t> ids;
	};
	// hiearchy is target type -> target name -> struct with vector of route ids
	// ie: "Blue Fish" -> "Sothis" -> {shortName, {id1, id2...}}
	std::unordered_map <std::string, std::map<std::string, targets_t>> mTargetToRouteIdMap;
	std::unordered_map <uint32_t, std::string> mRouteIdToNameMap;

	time_t convertBlockIndexToTime(const unsigned int blockIdx);
	unsigned int convertTimeToBlockIndex(const time_t& t);
	unsigned int getRoutePatternIndex(const unsigned int blockIdx, const unsigned int jump = 0);

	std::string createImageNameFromRouteId(const uint32_t& routeId, PRIORITY priority);
	std::string createButtonLabelFromRouteId(const uint32_t& routeId, PRIORITY priority);
};