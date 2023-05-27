//==============================================================================
/**
@file       FFXIVOceanFishingHelper.cpp
@brief      Handles multiple instances of processor for various fishing routes
@copyright  (c) 2023, Momoko Tomoko
**/
//==============================================================================

#include "pch.h"
#include "FFXIVOceanFishingHelper.h"


FFXIVOceanFishingHelper::FFXIVOceanFishingHelper(const std::vector<std::string>& dataFiles)
{
	for (const auto & dataFile : dataFiles)
	{
		auto newProcessor =
			std::make_unique<FFXIVOceanFishingProcessor>(
				FFXIVOceanFishingProcessor(dataFile)
			);
		processors.insert({ newProcessor->getRouteName(), std::move(newProcessor) });
	}
}

/**
	@brief returns the name of all processor route names as a json

	@return json of route names
**/
json FFXIVOceanFishingHelper::getRouteNames()
{
	json j;
	for (const auto& processor : processors)
		j.push_back(processor.first);
	return j;
}

/**
	@brief wrapper around getSecondsUntilNextVoyage for each route processor

	@param[out] secondsTillNextVoyage number of seconds until the next window, not including the one we are currently in
	@param[out] secondsLeftInWindow number of seconds left in the current window. Is set to 0 if not in a current window
	@param[in] startTime the time to start counting from.
	@param[in] voyageIds A set of voyageIds we are looking for per route name.
	@param[in] routeName the name of the route
	@param[in] skips number of windows to skip over

	@return true if successful
**/
bool FFXIVOceanFishingHelper::getSecondsUntilNextVoyage(
	uint32_t& secondsTillNextVoyage,
	uint32_t& secondsLeftInWindow,
	const time_t& startTime,
	const std::unordered_set<uint32_t>& voyageIds,
	const std::string& routeName,
	const uint32_t skips
)
{
	if (!processors.contains(routeName))
	{
		secondsTillNextVoyage = UINT_MAX;
		secondsLeftInWindow = 0;
		return false;
	}

	uint32_t returnedVoyageId;
	return processors[routeName]->getSecondsUntilNextVoyage(
		secondsTillNextVoyage,
		secondsLeftInWindow,
		returnedVoyageId, // unused here
		startTime,
		voyageIds, // ids
		skips
	);
}

/**
	@brief wrapper around getImageNameAndLabel for each route processor

	@param[out] imageName the name of the png image for this tracker
	@param[out] buttonLabel the string label for this button
	@param[in] routeName the name of the route
	@param[in] tracker the name of the tracker type (ie: Blue Fish, Achievement)
	@param[in] name the name of the actual thing to track (ie: name of fish, name of Achievement)
	@param[in] priority whether to prioritize achievement name or blue fish name
	@param[in] skips number of windows to skip over
**/
void FFXIVOceanFishingHelper::getImageNameAndLabel(
	std::string& imageName,
	std::string& buttonLabel,
	const std::string& routeName,
	const std::string& tracker,
	const std::string& name,
	const time_t& startTime,
	const PRIORITY priority,
	const uint32_t skips
)
{
	if (!processors.contains(routeName))
		return;

	processors[routeName]->getImageNameAndLabel(
		imageName,
		buttonLabel,
		tracker,
		name,
		startTime,
		priority,
		skips
	);
}

/**
	@brief gets targets as json

	@param[in] routeName the name of the route

	@return json containing targets
**/
json FFXIVOceanFishingHelper::getTargetsJson(const std::string& routeName)
{
	json j;
	if (!processors.contains(routeName))
		return j;

	return processors[routeName]->getTargetsJson();
}

/**
	@brief gets tracker types as json

	@param[in] routeName the name of the route

	@return json containing tracker types
**/
json FFXIVOceanFishingHelper::getTrackerTypesJson(const std::string& routeName)
{
	json j;
	if (!processors.contains(routeName))
		return j;

	return processors[routeName]->getTrackerTypesJson();
}

/**
	@brief converts a target to a set of voyage ids that matches the target

	@param[in] routeName the name of the route
	@param[in] type the targets type
	@param[in] name the targets name

	@return a list of voyage ids if found
**/
std::unordered_set<uint32_t> FFXIVOceanFishingHelper::getVoyageIdByTracker(
	const std::string& routeName,
	const std::string& tracker,
	const std::string& name
)
{
	if (!processors.contains(routeName))
		return {};

	return processors[routeName]->getVoyageIdByTracker(tracker, name);
}