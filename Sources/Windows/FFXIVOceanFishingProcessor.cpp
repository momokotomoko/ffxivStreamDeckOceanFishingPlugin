//==============================================================================
/**
@file       FFXIVOceanFishingProcessor.cpp
@brief      Computes Ocean Fishing Times
@copyright  (c) 2023, Momoko Tomoko
**/
//==============================================================================

#include "pch.h"
#include "FFXIVOceanFishingProcessor.h"
#include <time.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "../Vendor/json/src/json.hpp"
using json = nlohmann::json;

FFXIVOceanFishingProcessor::FFXIVOceanFishingProcessor(const std::string& dataFile)
{
	std::ifstream ifs(dataFile);

	if (ifs.fail())
	{
		errorMessage = "Failed to open datafile: " + dataFile;
		return;
	}

	json j;
	bool jsonIsGood = false;
	try
	{
		j = j.parse(ifs);
		jsonIsGood = true;
	}
	catch (...)
	{
		errorMessage = "Failed to parse dataFile into json object.";
	}
	ifs.close();

	if (jsonIsGood)
		loadDatabase(j);
}

FFXIVOceanFishingProcessor::FFXIVOceanFishingProcessor(const json& j)
{
	loadDatabase(j);
}

bool FFXIVOceanFishingProcessor::loadSchedule(const json& j)
{
	if (isBadKey(j, "name", "Missing route name in database.")) return false;
	if (isBadKey(j, "schedule", "Missing schedule in database.")) return false;
	if (isBadKey(j["schedule"], "pattern", "Missing pattern in schedule.")) return false;
	if (isBadKey(j["schedule"], "offset", "Missing offset in schedule.")) return false;

	// get route name
	if (j["name"].is_string())
		mRouteName = j["name"].get<std::string>();
	else
	{
		errorMessage = "Invalid route name:\n" + j["schedule"].dump(4);
		return false;
	}

	// get the pattern and store it in mRoutePattern
	for (const auto& id : j["schedule"]["pattern"])
		if (id.is_number_unsigned())
			mRoutePattern.push_back(id.get<uint32_t>());
		else
		{
			errorMessage = "Invalid pattern in schedule: " + id.dump(4) + "\n" + j["schedule"].dump(4);
			return false;
		}

	// get the offset and store it in mPatternOffset
	if (j["schedule"]["offset"].is_number_unsigned())
		mPatternOffset = j["schedule"]["offset"].get<uint32_t>();
	else
	{
		errorMessage = "Invalid offset in schedule:\n" + j["schedule"].dump(4);
		return false;
	}
	return true;
}

void FFXIVOceanFishingProcessor::loadDatabase(const json& j)
{
	// TODO handle parse errors
	// TODO refactor this function

	// get schedule
	if (!loadSchedule(j)) return;

	// get stops
	for (const auto& stops : j["stops"].get<json::object_t>())
	{
		mStops.insert({ stops.first, stops.second["shortform"].get<std::string>() });
	}

	// get fish
	if (j["targets"].contains("fish"))
	{
		for (const auto& fishType : j["targets"]["fish"].get<json::object_t>())
		{
			mFishes.insert({ fishType.first, {} });
			for (const auto& fish : fishType.second.get<json::object_t>())
			{
				std::vector<locations_t> locations;
				for (const auto& location : fish.second["locations"])
				{
					// construct a vector of times the fish is available
					// an empty vector means any time is allowed

					std::vector<std::string> times;
					if (location.contains("time"))
					{
						// location["time"] can be a single entry ("time": "day") or an array ("time": ["day", night"])
						if (location["time"].is_array())
							for (const auto& time : location["time"])
								times.push_back(time.get<std::string>());
						else
							times.push_back(location["time"].get<std::string>());
					}

					locations.push_back({ location["name"].get<std::string>(), times });
				}

				std::string shortformName = fish.first; // by default the shortform name is just the fish name
				if (fish.second.contains("shortform"))
					shortformName = fish.second["shortform"].get<std::string>();

				mFishes.at(fishType.first).insert({ fish.first,
						{shortformName,
						 locations}
					});
				if (fishType.first == "Blue Fish")
					mBlueFishNames.insert({ fish.first, mFishes.at(fishType.first).at(fish.first) });
			}
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
	std::unordered_set<uint32_t> allRouteIds;
	for (const auto& route : j["routes"].get<json::object_t>())
	{
		std::string routeName = route.first;
		if (mRoutes.contains(routeName))
			throw std::runtime_error("Error: duplicate route name in json: " + routeName);
		if (!route.second["id"].is_number_unsigned())
			throw std::runtime_error("Error: invalid route ID in json: " + route.second["id"].dump(4));
		const uint32_t id = route.second["id"].get<uint32_t>();
		if (allRouteIds.contains(id))
			throw std::runtime_error("Error: duplidcate route id in json: " + id);
		allRouteIds.insert(id);

		std::vector<stop_t> stops;
		for (const auto& stop : route.second["stops"])
		{
			const std::string stopName = stop["name"].get<std::string>();
			const std::string stopTime = stop["time"].get<std::string>();
			std::unordered_set<std::string> fishList;
			// double check that the stops exist, and create list of fishes
			if (!mStops.contains(stopName))
			{
				throw std::runtime_error("Error: stop " + stopName + " in route " + routeName + " does not exist in j[\"stops\"]");
			}
			for (const auto& fishType : mFishes)
			{
				for (const auto& fish : fishType.second)
				{
					for (const auto& location : fish.second.locations)
					{
						if (location.name != stopName)
							continue;

						bool isTimeMatch = false;
						// empty time vector means any time is allowed
						if (location.time.empty())
							isTimeMatch = true;
						else
							// go through each time and check for any match
							for (const auto& time : location.time)
							{
								if (time == stopTime)
								{
									isTimeMatch = true;
									break;
								}
							}

						if (!isTimeMatch)
							continue;

						fishList.insert(fish.first);
						break;
					}
				}
			}
			stops.push_back({ {stopName, {stopTime} } , fishList });
		}

		// load achievements into route
		std::set<std::string> achievements;
		for (const auto& achievement : mAchievements)
		{
			if (achievement.second.contains(id))
				achievements.insert(achievement.first);
		}

		mRoutes.insert({routeName,
			{
				route.second["shortform"].get<std::string>(),
				id,
				stops,
				achievements,
				"" // bluefishpattern, generated later
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
				if (mBlueFishNames.contains(fish)) // we only care about the blue fish
				{
					blueFishFound = true;
					blueFishPattern += mBlueFishNames.at(fish).shortName;
					blueFish.insert(fish);
					break;
				}
			}
			if (!blueFishFound)
				blueFishPattern += "X";
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
					if (stop.fish.contains(fish.first))
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
		if (mStops.contains(lastStop))
		{
			std::string lastStopShortName = mStops.at(lastStop);

			if (!mTargetToRouteIdMap.contains(lastStopShortName))
				mTargetToRouteIdMap.at("Routes").insert({ lastStopShortName, {"", "", {}} });
			mTargetToRouteIdMap.at("Routes").at(lastStopShortName).ids.insert(route.second.id);
		}

		mTargetToRouteIdMap.at("Routes").insert({ route.first, {name, "", {route.second.id}} });
	}
	
	// special targets:
	mTargetToRouteIdMap.insert({ "Other", {}});
	mTargetToRouteIdMap.at("Other").insert({ "Next Route", {"", "", allRouteIds} });

	mIsInit = true;
}

/**
	@brief gets the next route number

	@param[out] nextRoute the routeId used
	@param[in] startTime the time to start counting from.
	@param[in] routeIds A set of routeIds we are looking for. The closest time is returned out of all the routes. =
	@param[in] skips number of windows to skip over. Default is 0.

	@return true if successful
**/
bool FFXIVOceanFishingProcessor::getNextRoute(
	uint32_t& nextRoute,
	const time_t& startTime,
	const std::unordered_set<uint32_t>& routeIds,
	const uint32_t skips
)
{
	uint32_t relativeSecondsTillNextRoute = 0;
	uint32_t relativeWindowTime = 0;
	return getSecondsUntilNextRoute(relativeSecondsTillNextRoute, relativeWindowTime, nextRoute, startTime, routeIds, skips);
}

/**
	@brief gets the number of seconds until the next window. If already in a window, it will also get the seconds left in that window

	@param[out] secondsTillNextRoute number of seconds until the next window, not including the one we are currently in
	@param[out] secondsLeftInWindow number of seconds left in the current window. Is set to 0 if not in a current window
	@param[out] nextRoute the routeId used
	@param[in] startTime the time to start counting from.
	@param[in] routeIds A set of routeIds we are looking for. The closest time is returned out of all the routes.
	@param[in] skips number of windows to skip over. Default is 0.

	@return true if successful
**/
bool FFXIVOceanFishingProcessor::getSecondsUntilNextRoute(
	uint32_t& secondsTillNextRoute,
	uint32_t& secondsLeftInWindow,
	uint32_t& nextRoute,
	const time_t& startTime,
	const std::unordered_set<uint32_t> & routeIds,
	const uint32_t skips
)
{
	bool nextRouteUpdated = false;
	secondsTillNextRoute = UINT32_MAX;
	secondsLeftInWindow = 0;

	if (routeIds.empty())
		return false;

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
		// TODO: exit cycle loop if we went through entire pattern with no match, remove maxCycles
		if (routeIds.contains(mRoutePattern[wrappedIdx]))
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
				secondsLeftInWindow = static_cast<uint32_t>(60 * 15 + timeDifference);
			}
			else
			{
				secondsTillNextRoute = static_cast<uint32_t>(timeDifference);
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
std::string FFXIVOceanFishingProcessor::getNextRouteName(const time_t& t, const unsigned int skips)
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
		if (mRouteIdToNameMap.contains(routeIdx))
		{
			const std::string routeName = mRouteIdToNameMap.at(routeIdx);
			if (mRoutes.contains(routeName))
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
time_t FFXIVOceanFishingProcessor::convertBlockIndexToTime(const unsigned int blockIdx)
{
	return static_cast<time_t>(blockIdx - (mPatternOffset - 1)) * 60 * 60 * 2;
}

/**
	@brief converts a time to a block index
	       this algorithm was obtained from https://github.com/proyebat/FFXIVOceanFishingTimeCalculator

	@param[in] t the time_t to convert

	@return the block index
**/
unsigned int FFXIVOceanFishingProcessor::convertTimeToBlockIndex(const time_t& t)
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
unsigned int FFXIVOceanFishingProcessor::getRoutePatternIndex(const unsigned int blockIdx, const unsigned int jump)
{
	return (blockIdx + jump) % mRoutePattern.size();
}

/**
	@brief creates an achievement string name given a voyage name

	@param[in] voyageName the name of the voyage

	@return the achievement name
**/
std::string FFXIVOceanFishingProcessor::createAchievementName(const std::string & voyageName)
{
	std::string achievementName = "";
	bool firstAchievement = true;
	for (const auto& achievement : mRoutes.at(voyageName).achievements)
	{
		if (!firstAchievement)
			achievementName += "-";
		achievementName += achievement;
		firstAchievement = false;
	}
	
	return achievementName;
}

/**
	@brief converts a routeId to image name

	@param[in] routeId the routeId to get the image for
	@param[in] priority whether to prioritize achievement name or blue fish name

	@return the image name

	@relatesalso getImageName
**/
std::string FFXIVOceanFishingProcessor::createImageNameFromRouteId(const uint32_t& routeId, PRIORITY priority)
{
	if (!mRouteIdToNameMap.contains(routeId))
		return "";
	std::string routeName = mRouteIdToNameMap.at(routeId);
	if (!mRoutes.contains(routeName))
		return "";

	std::string blueFishName = "";
	std::string blueFishPattern = mRoutes.at(routeName).blueFishPattern;
	if (mTargetToRouteIdMap.contains("Blue Fish Pattern") &&
		mTargetToRouteIdMap.at("Blue Fish Pattern").contains(blueFishPattern))
		blueFishName = mTargetToRouteIdMap.at("Blue Fish Pattern").at(blueFishPattern).imageName;
	std::string achievementName = createAchievementName(routeName);

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
std::string FFXIVOceanFishingProcessor::createButtonLabelFromRouteId(const uint32_t& routeId, PRIORITY priority)
{
	if (!mRouteIdToNameMap.contains(routeId))
		return "";
	std::string routeName = mRouteIdToNameMap.at(routeId);
	if (!mRoutes.contains(routeName))
		return "";

	std::string blueFishName = mRoutes.at(routeName).blueFishPattern;
	std::string achievementName = createAchievementName(routeName);

	if ((priority == ACHIEVEMENTS && !achievementName.empty()) || blueFishName.empty())
		return achievementName;
	else
		return blueFishName;
}

/**
	@brief get the image name and label for a particular tracking

	@param[out] imageName the name of the png image for this tracker
	@param[out] buttonLabel the string label for this button
	@param[in] routeName the name of the route
	@param[in] tracker the name of the tracker type (ie: Blue Fish, Achievement)
	@param[in] name the name of the actual thing to track (ie: name of fish, name of Achievement)
	@param[in] startTime the time to start counting from.
	@param[in] priority whether to prioritize achievement name or blue fish name
	@param[in] skips number of windows to skip over
**/
void FFXIVOceanFishingProcessor::getImageNameAndLabel(
	std::string& imageName,
	std::string& buttonLabel,
	const std::string& tracker,
	const std::string& name,
	const time_t& startTime,
	const PRIORITY priority,
	const uint32_t skips
)
{
	imageName = "";
	buttonLabel = "";

	// first look for tracker+name in the map
	if (mTargetToRouteIdMap.contains(tracker))
	{
		if (mTargetToRouteIdMap.at(tracker).contains(name))
		{
			buttonLabel = mTargetToRouteIdMap.at(tracker).at(name).labelName;
			imageName = mTargetToRouteIdMap.at(tracker).at(name).imageName;

			// if labelName or imageName not provided, we need to create them
			if (buttonLabel.empty() || imageName.empty())
			{
				// first get routeId for this tracker
				std::unordered_set<uint32_t> routeIds = getRouteIdByTracker(tracker, name);

				// get next route
				uint32_t nextRoute;
				if (getNextRoute(nextRoute, startTime, routeIds, skips))
				{
					// create the names
					if (buttonLabel.empty())
						buttonLabel = createButtonLabelFromRouteId(nextRoute, priority);
					if (imageName.empty())
						imageName = createImageNameFromRouteId(nextRoute, priority);
				}
			}
		}
	}
}

/**
	@brief gets targets as json

	@return json containing targets
**/
json FFXIVOceanFishingProcessor::getTargetsJson()
{
	json j;
	for (const auto& type : mTargetToRouteIdMap)
		for (const auto& target : type.second)
			j.emplace(target.first, type.first);

	return j;
}

/**
	@brief gets tracker types as json

	@return json containing tracker types
**/
json FFXIVOceanFishingProcessor::getTrackerTypesJson()
{
	json j;
	for (const auto& type : mTargetToRouteIdMap)
		j.push_back(type.first);
	std::sort(j.begin(), j.end());
	return j;
}

/**
	@brief converts a target to a set of route ids that matches the target

	@param[in] type the targets type
	@param[in] name the targets name

	@return a set of route ids if found, and a null set if the route is not found
**/
std::unordered_set<unsigned int> FFXIVOceanFishingProcessor::getRouteIdByTracker(const std::string& tracker, const std::string& name)
{
	// this map was precomputed in initializer
	if (mTargetToRouteIdMap.contains(tracker))
		if (mTargetToRouteIdMap.at(tracker).contains(name))
			return mTargetToRouteIdMap.at(tracker).at(name).ids;
	return {};
}