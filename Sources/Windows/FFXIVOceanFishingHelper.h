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
#include <set>

#define OCEAN_FISHING_PATTERN_SIZE 144

class FFXIVOceanFishingHelper
{
public:

	FFXIVOceanFishingHelper();
	~FFXIVOceanFishingHelper() {};

	bool getSecondsUntilNextRoute(int& secondsTillNextRoute, int& secondsLeftInWindow, const time_t& startTime, const std::set<unsigned int>& routeId, const unsigned int skips = 0);
	std::string getNextRouteName(const time_t& t, const unsigned int skips = 0);

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
		{4, {{"Open Rhotano Sea", "Rhotano"}, {"Stonescale", "Stone"}}},
		{5, {{"Cieldalaes Margin", "Ciel"}, {"Hafgufa", "Hafg"}}},
		{6, {{"Open Bloodbrine Sea", "Blood"}, {"Seafaring Toad", "Toad"}}},
		{7, {{"Outer Rothlyt Sound", "Rothlyt"}, {"Placodus", "Placo"}}}
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
		{7, {"Hafg-Elas-X", {5, 3, 6}, {true, true, false}}},
		{8, {"Manta", {5, 3, 6}, {false, false, false}}},
		{9, {"Crab", {5, 3, 6}, {false, false, true}}},
		{10, {"Hafg-X-Placo", {5, 4, 7}, {true, false, true}}},
		{11, {"Balloons", {5, 4, 7}, {false, true, false}}},
		{12, {"Balloons", {5, 4, 7}, {false, false, false}}}
	};
	std::unordered_map<std::string, unsigned int> mRouteNameToIndexMap;

	// this pattern was obtained from https://github.com/proyebat/FFXIVOceanFishingTimeCalculator
	const unsigned int OFFSET = 85;
	const unsigned int mRoutePattern[OCEAN_FISHING_PATTERN_SIZE] = {
		7,10,1,4,8,11,2,5,12,3,6,
		7,10,1,4,8,11,2,5,9,3,6,
		7,10,1,4,8,11,2,5,9,12,6,
		7,10,1,4,8,11,2,5,9,12,3,
		7,10,1,4,8,11,2,5,9,12,3,6,
		10,1,4,8,11,2,5,9,12,3,6,
		7,1,4,8,11,2,5,9,12,3,6,
		7,10,4,8,11,2,5,9,12,3,6,
		7,10,1,8,11,2,5,9,12,3,6,
		7,10,1,4,11,2,5,9,12,3,6,
		7,10,1,4,8,2,5,9,12,3,6,
		7,10,1,4,8,11,5,9,12,3,6,
		7,10,1,4,8,11,2,9,12,3,6 };

	time_t convertBlockIndexToTime(const unsigned int blockIdx);
	unsigned int convertTimeToBlockIndex(const time_t& t);
	unsigned int getRoutePatternIndex(const unsigned int blockIdx, const unsigned int jump = 0);
};