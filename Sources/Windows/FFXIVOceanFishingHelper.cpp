//==============================================================================
/**
@file       FFXIVOceanFishingHelper.cpp
@brief      Computes Ocean Fishing Times
@copyright  (c) 2020, Momoko Tomoko
**/
//==============================================================================

#include "pch.h"
#include "FFXIVOceanFishingHelper.h"
#include <time.h>
#include <iostream>
#include <fstream>
#include "../Vendor/json/src/json.hpp"
using json = nlohmann::json;

FFXIVOceanFishingHelper::FFXIVOceanFishingHelper()
{
	// TODO make this a setting
	loadDatabase("oceanFishingDatabase.json");
}

void FFXIVOceanFishingHelper::loadDatabase(const std::string dataFile)
{
	// TODO handle parse errors
	// TODO refactor this function
	std::ifstream ifs("oceanFishingDatabase.json");
	json j = j.parse(ifs);
	ifs.close();

	// get pattern
	for (const auto& data : j["schedule"]["pattern"])
	{
		mRoutePattern.push_back(data.get<uint32_t>());
	}
	mPatternOffset = j["schedule"]["offset"].get<uint32_t>();

	// get stops
	for (const auto& stops : j["stops"].get<json::object_t>())
	{
		mStops.insert({ stops.first, stops.second["shortform"].get<std::string>() });
	}

	// get blue fish
	for (const auto& fishType : j["targets"]["fish"].get<json::object_t>())
	{
		mFishes.insert({ fishType.first, {} });
		for (const auto& fish : fishType.second.get<json::object_t>())
		{
			std::vector<locations_t> locations;
			for (const auto& location : fish.second["locations"])
			{
				locations.push_back({ location["name"].get<std::string>(), location["time"].get<std::string>() });
			}
			mFishes.at(fishType.first).insert({ fish.first,
					{fish.second["shortform"].get<std::string>(),
					 locations}
				});
			if (fishType.first == "Blue Fish")
				mBlueFishNames.insert({ fish.first, mFishes.at(fishType.first).at(fish.first)});
		}
	}

	// get achievements
	for (const auto& achievement : j["targets"]["achievements"].get<json::object_t>())
	{
		std::unordered_set <uint32_t> ids;
		for (const auto routeId : achievement.second["routeIds"])
		{
			ids.insert(routeId.get<uint32_t>());
		}
		mAchievements.insert({achievement.first, ids});
	}

	// get routes
	for (const auto& route : j["routes"].get<json::object_t>())
	{
		std::string routeName = route.first;
		if (mRoutes.find(routeName) != mRoutes.end())
		{
			throw std::runtime_error("Error: duplicate route name in json: " + routeName);
		}

		std::vector<stop_t> stops;
		for (const auto& stop : route.second["stops"])
		{
			const std::string stopName = stop["name"].get<std::string>();
			const std::string stopTime = stop["time"].get<std::string>();
			std::unordered_set<std::string> fishList;
			// double check that the stops exist, and create list of fishes
			if (mStops.find(stopName) == mStops.end())
			{
				throw std::runtime_error("Error: stop " + stopName + " in route " + routeName + " does not exist in j[\"stops\"]");
			}
			for (const auto& fishType : mFishes)
			{
				for (const auto& fish : fishType.second)
				{
					for (const auto& location : fish.second.locations)
					{
						if (location.name == stopName && location.time == stopTime)
						{
							fishList.insert(fish.first);
						}
					}
				}
			}
			stops.push_back({ {stopName, stopTime} , fishList });
		}

		const uint32_t id = route.second["id"].get<uint32_t>();

		// load achievements into route
		std::unordered_set<std::string> achievements;
		for (const auto& achievement : mAchievements)
		{
			if (achievement.second.find(id) != achievement.second.end())
				achievements.insert(achievement.first);
		}

		mRoutes.insert({routeName,
			{
				route.second["shortform"].get<std::string>(),
				id,
				stops,
				achievements,
				"" // bluefishpatter, generated later
			}
		});
		mRouteIdToNameMap.insert({ id, routeName });
	}

	// construct search target mapping
	// targets by blue fish per route
	mTargetToRouteIdMap.insert({ "Blue Fish Pattern", {} });
	for (const auto& route : mRoutes)
	{
		std::string blueFishPattern;
		std::unordered_set<std::string> blueFish;

		// create pattern string as fish1-fish2-fish3, and use X if there is no blue fish
		for (const auto& stop : route.second.stops)
		{
			bool blueFishFound = false;
			for (const auto& fish : stop.fish) // go through all the possible fishes at this stop
			{
				if (mBlueFishNames.find(fish) != mBlueFishNames.end()) // we only care about the blue fish
				{
					blueFishFound = true;
					blueFishPattern += mBlueFishNames.at(fish).shortName;
					blueFish.insert(fish);
					break;
				}
			}
			if (!blueFishFound)
			{
				blueFishPattern += "X";
			}

			blueFishPattern += "-";
		}
		// remove the last dash
		if (blueFishPattern.length() > 0)
			blueFishPattern = blueFishPattern.substr(0, blueFishPattern.length() - 1);

		// if only 1 blue fish, use that as the image name without the Xs
		std::string imageName = blueFishPattern;
		if (blueFish.size() == 1)
			imageName = *blueFish.begin();

		if (blueFishPattern != "X-X-X")
		{
			mTargetToRouteIdMap.at("Blue Fish Pattern").insert({ blueFishPattern, 
				{
					blueFishPattern, // label name
					imageName, // image name
					{} // route ids
				}
			});
			mTargetToRouteIdMap.at("Blue Fish Pattern").at(blueFishPattern).ids.insert(route.second.id);
			mRoutes.at(route.first).blueFishPattern = blueFishPattern;
		}
	}
	
	// achievements targets:
	mTargetToRouteIdMap.insert({ "Achievement", {} });
	for (const auto& achievement : mAchievements)
	{
		mTargetToRouteIdMap.insert({ achievement.first, {} });
		std::unordered_set <uint32_t> ids;
		for (const auto& routeId : achievement.second)
		{
			ids.insert(routeId);
		}
		mTargetToRouteIdMap.at("Achievement").insert({ achievement.first, 
			{
				achievement.first, // achievement label and imagename are the same as just the acheivement name
				achievement.first,
				ids
			}
		});
	}

	// fish targets:
	for (const auto& fishType : mFishes)
	{
		mTargetToRouteIdMap.insert({ fishType.first, {} });
		for (const auto& fish : fishType.second)
		{
			const std::string fishName = fish.first;
			std::unordered_set <uint32_t> ids;
			for (const auto& route : mRoutes)
			{
				for (const auto& stop : route.second.stops)
				{
					if (stop.fish.find(fish.first) != stop.fish.end())
					{
						ids.insert(route.second.id);
					}
				}
			}
			mTargetToRouteIdMap.at(fishType.first).insert({ fishName,
				{
					fish.first, // fish label and imagename are the same as just the fish name
					fish.first,
					ids
				}
			});
		}
	}

	// targets by route name:
	mTargetToRouteIdMap.insert({ "Routes", {} });
	for (const auto& route : mRoutes)
	{
		// create an name for this route. Priority goes to achievement, then to the route bluefishpattern
		std::string name = route.first;
		// TODO: This assumes only 1 achievement per route, although can have more. Right now just grab the first one and use that as the icon
		if (route.second.achievements.size() != 0)
			name = *route.second.achievements.begin();
		else if (route.second.blueFishPattern.length() > 0)
			name = route.second.blueFishPattern;
		
		std::string lastStop = route.second.stops.back().location.name;
		if (mStops.find(lastStop) != mStops.end())
		{
			std::string lastStopShortName = mStops.at(lastStop);

			if (mTargetToRouteIdMap.find(lastStopShortName) == mTargetToRouteIdMap.end())
				mTargetToRouteIdMap.at("Routes").insert({ lastStopShortName, {"", "", {}} });
			mTargetToRouteIdMap.at("Routes").at(lastStopShortName).ids.insert(route.second.id);
		}

		mTargetToRouteIdMap.at("Routes").insert({ route.first, {name, "", {route.second.id}} });
	}
	
	// special targets:
	mTargetToRouteIdMap.insert({ "Other", {}});
	mTargetToRouteIdMap.at("Other").insert({ "Next Route (Achievements Priority)", {"", "", {}} });
	mTargetToRouteIdMap.at("Other").insert({ "Next Route (Blue Fish Priority)", {"", "", {}} });
}

/**
	@brief gets the next route number

	@param[out] nextRoute the routeId used
	@param[in] startTime the time to start counting from.
	@param[in] routeIds A set of routeIds we are looking for. The closest time is returned out of all the routes. Set to empty set for any route.
	@param[in] skips number of windows to skip over. Default is 0.

	@return true if successful
**/
bool FFXIVOceanFishingHelper::getNextRoute(uint32_t& nextRoute, const time_t& startTime, const std::unordered_set<uint32_t>& routeIds, const uint32_t skips)
{
	int relativeSecondsTillNextRoute = 0;
	int relativeWindowTime = 0;
	return getSecondsUntilNextRoute(relativeSecondsTillNextRoute, relativeWindowTime, nextRoute, startTime, routeIds, skips);
}

/**
	@brief gets the number of seconds until the next window. If already in a window, it will also get the seconds left in that window

	@param[out] secondsTillNextRoute number of seconds until the next window, not including the one we are currently in
	@param[out] secondsLeftInWindow number of seconds left in the current window. Is set to 0 if not in a current window
	@param[out] nextRoute the routeId used
	@param[in] startTime the time to start counting from.
	@param[in] routeIds A set of routeIds we are looking for. The closest time is returned out of all the routes. Set to empty set for any route.
	@param[in] skips number of windows to skip over. Default is 0.

	@return true if successful
**/
bool FFXIVOceanFishingHelper::getSecondsUntilNextRoute(int& secondsTillNextRoute, int& secondsLeftInWindow, uint32_t& nextRoute, const time_t& startTime, const std::unordered_set<uint32_t> & routeIds, const uint32_t skips)
{
	bool nextRouteUpdated = false;
	secondsLeftInWindow = 0;

	// Get the status of where we are currently
	unsigned int currBlockIdx = convertTimeToBlockIndex(startTime);

	// Cycle through the route pattern until we get a match to a route we are looking for.
	unsigned int skipcounts = 0;
	unsigned int maxCycles = 1000; // limit cycles just in case
	for (unsigned int i = 0; i < maxCycles; i++)
	{
		// Current place in the pattern we are looking at.
		unsigned int wrappedIdx = getRoutePatternIndex(currBlockIdx, i);

		// Check to see if we match any of our desired routes
		bool routeMatch = false;
		if (routeIds.size() == 0)
			routeMatch = true;
		for (const auto& id : routeIds)
		{
			if (id == mRoutePattern[wrappedIdx])
			{
				routeMatch = true;
				break;
			}
		}

		if (routeMatch)
		{
			// Find the difference in time from the pattern position to the current time
			time_t routeTime = convertBlockIndexToTime(currBlockIdx + i);
			int timeDifference = static_cast<int>(difftime(routeTime, startTime));

			// If the time of the route is more than 15m behind us, then it's not a valid route
			if (timeDifference < -60 * 15)
			{
				continue;
			}

			// If we are skipping routes, skip now
			if (skipcounts < skips)
			{
				skipcounts++;
				continue;
			}

			// If we reach here we found a valid route.
			// If timeDifference <= 0, that means we are in a window,
			// but we still want the time of the next route, so don't return
			// and continue for another cycle to get a positive timeDifference

			if (!nextRouteUpdated) // remember the next route. Use the current window as the route if we are in window
			{
				nextRoute = mRoutePattern[wrappedIdx];
				nextRouteUpdated = true;
			}
			if (timeDifference <= 0) // needs to have the = also otherwise trigging this on the turn of the hour will not record that we're in a window
			{
				secondsLeftInWindow = 60 * 15 + timeDifference;
			}
			else
			{
				secondsTillNextRoute = timeDifference;
				return true;
			}
		}
	}
	return false;
}

/**
	@brief gets the route name at a selected time, with option to skip. If in a window, that window is the routes name. If not, the next window will be the name.

	@param[in] t the time to start the check
	@param[in] skips number of windows to skip over. Default is 0.

	@return name of the route
**/
std::string FFXIVOceanFishingHelper::getNextRouteName(const time_t& t, const unsigned int skips)
{
	unsigned int currBlockIdx = convertTimeToBlockIndex(t);

	unsigned int skipcounts = 0;
	const unsigned int maxCycles = 1000; // limit cycles just in case
	for (unsigned int i = 0; i < maxCycles; i++)
	{
		// Find the difference in time from the pattern position to the current time
		time_t routeTime = convertBlockIndexToTime(currBlockIdx + i);
		int timeDifference = static_cast<int>(difftime(routeTime, t));

		// If the time of the route is more than 15m behind us, then it's not a valid route
		if (timeDifference < -60 * 15)
		{
			continue;
		}

		// If we are skipping routes, skip now
		if (skipcounts < skips)
		{
			skipcounts++;
			continue;
		}

		// get route index
		unsigned int routeIdx = mRoutePattern[getRoutePatternIndex(currBlockIdx + i)];

		// convert from id
		if (mRouteIdToNameMap.find(routeIdx) != mRouteIdToNameMap.end())
		{
			const std::string routeName = mRouteIdToNameMap.at(routeIdx);
			if (mRoutes.find(routeName) != mRoutes.end())
				return routeName;
		}
	}
	return "";
}

/**
	@brief converts a block id to a time
	       this algorithm was obtained from https://github.com/proyebat/FFXIVOceanFishingTimeCalculator

	@param[in] blockIdx the block index to convert

	@return the time in time_t
**/
time_t FFXIVOceanFishingHelper::convertBlockIndexToTime(const unsigned int blockIdx)
{
	return static_cast<time_t>(blockIdx - (mPatternOffset - 1)) * 60 * 60 * 2;
}

/**
	@brief converts a time to a block index
	       this algorithm was obtained from https://github.com/proyebat/FFXIVOceanFishingTimeCalculator

	@param[in] t the time_t to convert

	@return the block index
**/
unsigned int FFXIVOceanFishingHelper::convertTimeToBlockIndex(const time_t& t)
{
	struct tm t_struct {};
	localtime_s(&t_struct, &t);

	// if we are within the 15 boat window, we are still inside the block. Account for this.
	if (t_struct.tm_min < 15)
	{
		t_struct.tm_min -= 15;
	}
	time_t alignedTime = mktime(&t_struct);
	return static_cast<unsigned int>(std::ceil(alignedTime / (60 * 60 * 2))) + mPatternOffset;
}

/**
	@brief gets the route's id from the pattern given a block index

	@param[in] blockIdx the block to convert
	@param[in] jump number of steps to jump ahead, default is 0

	@return the route id at the blockIdx + jump
**/
unsigned int FFXIVOceanFishingHelper::getRoutePatternIndex(const unsigned int blockIdx, const unsigned int jump)
{
	return (blockIdx + jump) % mRoutePattern.size();
}


/**
	@brief converts a routeId to image name

	@param[in] routeId the routeId to get the image for
	@param[in] priority whether to prioritize achievement name or blue fish name

	@return the image name

	@relatesalso getImageName
**/
std::string FFXIVOceanFishingHelper::createImageNameFromRouteId(const uint32_t& routeId, PRIORITY priority)
{
	if (mRouteIdToNameMap.find(routeId) == mRouteIdToNameMap.end())
		return "";
	std::string routeName = mRouteIdToNameMap.at(routeId);
	if (mRoutes.find(routeName) == mRoutes.end())
		return "";

	std::string blueFishName = "";
	std::string blueFishPattern = mRoutes.at(routeName).blueFishPattern;
	if (mTargetToRouteIdMap.find("Blue Fish Pattern") != mTargetToRouteIdMap.end() &&
		mTargetToRouteIdMap.at("Blue Fish Pattern").find(blueFishPattern) != mTargetToRouteIdMap.at("Blue Fish Pattern").end())
		blueFishName = mTargetToRouteIdMap.at("Blue Fish Pattern").at(blueFishPattern).imageName;
	std::string achievementName = "";
	if (mRoutes.at(routeName).achievements.size() != 0)
		achievementName = *mRoutes.at(routeName).achievements.begin(); // TODO: right now we assume there is only one achievement, so just return the first one

	if ((priority == ACHIEVEMENTS && !achievementName.empty()) || blueFishName.empty())
		return achievementName;
	else
		return blueFishName;
}

/**
	@brief converts a routeId to button label

	@param[in] routeId the routeId to get the image for
	@param[in] priority whether to prioritize achievement name or blue fish name

	@return the button label

	@relatesalso getButtonLabel
**/
std::string FFXIVOceanFishingHelper::createButtonLabelFromRouteId(const uint32_t& routeId, PRIORITY priority)
{
	if (mRouteIdToNameMap.find(routeId) == mRouteIdToNameMap.end())
		return "";
	std::string routeName = mRouteIdToNameMap.at(routeId);
	if (mRoutes.find(routeName) == mRoutes.end())
		return "";

	std::string blueFishName = mRoutes.at(routeName).blueFishPattern;
	std::string achievementName = "";
	if (mRoutes.at(routeName).achievements.size() != 0)
		achievementName = *mRoutes.at(routeName).achievements.begin(); // TODO: right now we assume there is only one achievement, so just return the first one

	if ((priority == ACHIEVEMENTS && !achievementName.empty()) || blueFishName.empty())
		return achievementName;
	else
		return blueFishName;
}

void FFXIVOceanFishingHelper::getImageNameAndLabel(std::string& imageName, std::string& buttonLabel, const std::string& tracker, const std::string& name, const uint32_t skips)
{
	imageName = "";
	buttonLabel = "";

	// first look for tracker+name in the map
	if (mTargetToRouteIdMap.find(tracker) != mTargetToRouteIdMap.end())
	{
		if (mTargetToRouteIdMap.at(tracker).find(name) != mTargetToRouteIdMap.at(tracker).end())
		{
			buttonLabel = mTargetToRouteIdMap.at(tracker).at(name).labelName;
			imageName = mTargetToRouteIdMap.at(tracker).at(name).imageName;

			// if labelName or imageName not provided, we need to create them
			if (buttonLabel.empty() || imageName.empty())
			{
				// first get routeId for this tracker
				std::unordered_set<uint32_t> routeIds = getRouteIdByTracker(tracker, name);

				// get next route
				time_t startTime = time(0);
				uint32_t nextRoute;
				if (getNextRoute(nextRoute, startTime, routeIds, skips))
				{
					PRIORITY priority = ACHIEVEMENTS;
					if (name == "Next Route (Blue Fish Priority)")
						priority = BLUE_FISH;

					// create the names
					if (buttonLabel.empty())
						buttonLabel = createButtonLabelFromRouteId(nextRoute, priority);
					if (imageName.empty())
						imageName = createImageNameFromRouteId(nextRoute, priority);
				}
			}
		}
	}

	return;
}

/**
	@brief gets targets as json

	@return json containing targets
**/
json FFXIVOceanFishingHelper::getTargetsJson()
{
	json j;
	
	for (const auto& type : mTargetToRouteIdMap)
	{
		for (const auto& target : type.second)
		{
			j.emplace(target.first, type.first);
		}
	}

	return j;
}

/**
	@brief converts a target to a set of route ids that matches the target

	@param[in] type the targets type
	@param[in] name the targets name

	@return a set of a single route id if found, and a null set if the route is not found
**/
std::unordered_set<unsigned int> FFXIVOceanFishingHelper::getRouteIdByTracker(const std::string& tracker, const std::string& name)
{
	// this map was precomputed in initializer
	if (mTargetToRouteIdMap.find(tracker) != mTargetToRouteIdMap.end())
	{
		if (mTargetToRouteIdMap.at(tracker).find(name) != mTargetToRouteIdMap.at(tracker).end())
		{
			return mTargetToRouteIdMap.at(tracker).at(name).ids;
		}
	}
	return {};
}