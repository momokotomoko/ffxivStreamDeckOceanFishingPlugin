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
#include <vector>

#define OCEAN_FISHING_PATTERN_SIZE 72

class FFXIVOceanFishingHelper
{
public:

	FFXIVOceanFishingHelper();
	~FFXIVOceanFishingHelper() {};

	bool getSecondsUntilNextRoute(int& secondsTillNextRoute, int& secondsLeftInWindow, const time_t& startTime, const std::set<unsigned int>& routeId, const unsigned int skips = 0);
	unsigned int getRoutePatternIndex(const unsigned int blockIdx, const unsigned int jump = 0);

	std::set<unsigned int> getRoutesWithBlueFish(const std::string& blueFish);
	std::set<unsigned int> getRouteIdFromName(const std::string& name);

	std::set<std::string> getAllBlueFishNames();
	std::set<std::string> getAllRouteNames();
private:
	struct names_t
	{
		const std::string name;
		const std::string shortName;
	};

	struct locations_t
	{
		const names_t location;
		const names_t blueFish;
	};

	const std::unordered_map <unsigned int, locations_t> mLocations =
	{
		{1, {{"Outer Gladion Bay", "Gladion"}, {"Sothis", "Sothis"}}},
		{2, {{"The Southern Strait of Merlthor", "Southern"}, {"Coral Manta", "Coral"}}},
		{3, {{"The Northern Strait of Merlthor", "Northern"}, {"Elasmosaurus", "Elasmo"}}},
		{4, {{"Open Rhotano Sea", "Rhotano"}, {"Stonescale", "Stone"}}}
	};

	std::set<std::string> mBlueFishNames;
	std::unordered_map<std::string, std::set<unsigned int>> mBlueFishToRouteMap;

	struct route_t
	{
		const std::string name;
		const int routeOrder[3];
		const bool blueFishAvailable[3];
	};

	const std::unordered_map <unsigned int, route_t> mRoutes =
	{
		{1, {"Dragon", {2, 1, 3}, {true, false, false}}},
		{2, {"Octo", {2, 1, 3}, {false, false, false}}},
		{3, {"X-Soth-Elas", {2, 1, 3}, {false, true, true}}},
		{4, {"Soth-X-Stone", {1, 2, 4}, {true, false, true}}},
		{5, {"Jelly", {1, 2, 4}, {false, false, false}}},
		{6, {"Shark", {1, 2, 4}, {false, true, false}}},
	};
	std::unordered_map<std::string, unsigned int> mRouteNameToIndexMap;

	// this pattern was obtained from https://github.com/proyebat/FFXIVOceanFishingTimeCalculator
	const unsigned int mRoutePattern[OCEAN_FISHING_PATTERN_SIZE] = {
		1, 4, 2, 5, 3, 6, 1, 4, 2, 5, 3, 6,
		4, 1, 5, 2, 6, 3, 4, 1, 5, 2, 6, 3,
		2, 5, 3, 6, 1, 4, 2, 5, 3, 6, 1, 4,
		5, 2, 6, 3, 4, 1, 5, 2, 6, 3, 4, 1,
		3, 6, 1, 4, 2, 5, 3, 6, 1, 4, 2, 5,
		6, 3, 4, 1, 5, 2, 6, 3, 4, 1, 5, 2 };

	time_t convertBlockIndexToTime(const unsigned int blockIdx);
	unsigned int convertTimeToBlockIndex(const time_t& t);
};