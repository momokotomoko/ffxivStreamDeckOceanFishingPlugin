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


FFXIVOceanFishingHelper::FFXIVOceanFishingHelper()
{
	// init reverse mappings for faster lookup

	// map from route name -> route index
	for (const auto& route : mRoutes)
	{
		mRouteNameToIndexMap.insert({ route.second.name, route.first });
	}

	// map from blue fish name -> route index
	// first get all the blue fish names
	for (const auto& loc : mLocations)
	{
		mBlueFishNames.insert(loc.second.blueFish.name);

		// also init the map data structure
		mBlueFishToRouteMap.insert({ loc.second.blueFish.name, {} });
	}
	// then go through all the routes, and if there is a blue fish in that route
	// add that route index to the set corresponding to the blue fish's name
	for (const auto& route : mRoutes)
	{
		const unsigned int routeStops = 3;
		for (unsigned int i = 0; i < routeStops; i++)
		{
			if (route.second.blueFishAvailable[i])
			{
				unsigned int locationIdx = route.second.routeOrder[i];
				std::string blueFishName = mLocations.at(locationIdx).blueFish.name;
				if (mBlueFishToRouteMap.find(blueFishName) != mBlueFishToRouteMap.end())
				{
					mBlueFishToRouteMap.at(blueFishName).insert(route.first);
				}
			}
		}
	}
}

/**
	@brief gets the number of seconds until the next window. If already in a window, it will also get the seconds left in that window

	@param[out] secondsTillNextRoute number of seconds until the next window, not including the one we are currently in
	@param[out] secondsLeftInWindow number of seconds left in the current window. Is set to 0 if not in a current window
	@param[in] startTime the time to start counting from.
	@param[in] routeId A set of routeIds we are looking for. The closest time is returned out of all the routes. Set to empty set for any route.
	@param[in] skips number of windows to skip over. Default is 0.

	@return true if successful
**/
bool FFXIVOceanFishingHelper::getSecondsUntilNextRoute(int& secondsTillNextRoute, int& secondsLeftInWindow, const time_t& startTime, const std::set<unsigned int> & routeId, const unsigned int skips)
{
	secondsLeftInWindow = 0;

	// Get the status of where we are currently
	unsigned int currBlockIdx = convertTimeToBlockIndex(startTime);

	// Cycle through the route pattern until we get a match to a route we are looking for.
	unsigned int skipcounts = 0;
	const unsigned int maxCycles = 1000; // limit cycles just in case
	for (unsigned int i = 0; i < maxCycles; i++)
	{
		// Current place in the pattern we are looking at.
		unsigned int wrappedIdx = getRoutePatternIndex(currBlockIdx, i);

		// Check to see if we match any of our desired routes
		bool routeMatch = false;
		if (routeId.size() == 0)
			routeMatch = true;
		for (const auto& id : routeId)
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

		unsigned int routeIdx = mRoutePattern[getRoutePatternIndex(currBlockIdx + i)];
		if (mRoutes.find(routeIdx) != mRoutes.end())
			return mRoutes.at(routeIdx).name;
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
	return static_cast<time_t>(blockIdx - 16) * 60 * 60 * 2;
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
	const unsigned int offset = 17;
	return static_cast<unsigned int>(std::ceil(alignedTime / (60 * 60 * 2))) + offset;
}

/**
	@brief gets the route's id from the pattern given a block index

	@param[in] blockIdx the block to convert
	@param[in] jump number of steps to jump ahead, default is 0

	@return the route id at the blockIdx + jump
**/
unsigned int FFXIVOceanFishingHelper::getRoutePatternIndex(const unsigned int blockIdx, const unsigned int jump)
{
	return (blockIdx + jump) % OCEAN_FISHING_PATTERN_SIZE;
}

/**
	@brief gets a set of all the blue fish names

	@return a set of all blue fish names
**/
std::set<std::string> FFXIVOceanFishingHelper::getAllBlueFishNames()
{
	// this was precomputed in initializer
	return mBlueFishNames;
}

/**
	@brief gets a set of all the route names

	@return a set of all route names
**/
std::set<std::string> FFXIVOceanFishingHelper::getAllRouteNames()
{
	std::set<std::string> names;
	for (const auto& loc : mRoutes)
	{
		names.insert(loc.second.name);
	}
	return names;
}

/**
	@brief gets a set of all the route ids that has a certain blue fish available

	@param[in] blueFish name of the blue fish to be searched for

	@return a set of all route ids that has the blue fish available, and a null set if the blue fish is not found
**/
std::set<unsigned int> FFXIVOceanFishingHelper::getRoutesWithBlueFish(const std::string& blueFish)
{
	// this map was precomputed in initializer
	if (mBlueFishToRouteMap.find(blueFish) != mBlueFishToRouteMap.end())
	{
		return mBlueFishToRouteMap.at(blueFish);
	}
	return {};
}

/**
	@brief converts the route name to a route id

	@param[in] name the route name

	@return a set of a single route id if found, and a null set if the route is not found
**/
std::set<unsigned int> FFXIVOceanFishingHelper::getRouteIdFromName(const std::string& name)
{
	if (mRouteNameToIndexMap.find(name) != mRouteNameToIndexMap.end())
	{
		return { mRouteNameToIndexMap.at(name) };
	}
	return {};
}