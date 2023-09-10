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
#include "FFXIVOceanFishingJsonLoadUtils.hpp"
using json = nlohmann::json;

FFXIVOceanFishingProcessor::FFXIVOceanFishingProcessor(const std::string& dataFile)
{
	std::ifstream ifs(dataFile);

	if (ifs.fail())
	{
		mErrorMessage = "Failed to open datafile: " + dataFile;
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
		mErrorMessage = "Failed to parse dataFile into json object.";
	}
	ifs.close();

	if (jsonIsGood)
		loadDatabase(j);
}

FFXIVOceanFishingProcessor::FFXIVOceanFishingProcessor(const json& j)
{
	loadDatabase(j);
}

void FFXIVOceanFishingProcessor::loadDatabase(const json& j)
{
	//TODO see if we can remove mStops and locations_t

	mErrorMessage = jsonLoadUtils::loadRouteName(mRouteName, j);
	if (mErrorMessage) return;

	mErrorMessage = jsonLoadUtils::loadVoyageSchedule(mVoyagePattern, mPatternOffset, j);
	if (mErrorMessage) return;

	mErrorMessage = jsonLoadUtils::loadStops(mStops, j);
	if (mErrorMessage) return;

	mErrorMessage = jsonLoadUtils::loadFish(mFishes, mBlueFishNames, j);
	if (mErrorMessage) return;

	mErrorMessage = jsonLoadUtils::loadAchievements(mAchievements, j);
	if (mErrorMessage) return;

	mErrorMessage = jsonLoadUtils::loadVoyages(mVoyages, mVoyageIdToNameMap, mFishes, mAchievements, j);
	if (mErrorMessage) return;

	// construct search target mapping
	jsonLoadUtils::setBlueFishTargets(mTargetToVoyageIdMap, mVoyages, mBlueFishNames);
	

	//TODO: move these into FFXIVOceanFishingProcessor
	// achievements targets:
	jsonLoadUtils::setAchievementTargets(mTargetToVoyageIdMap, mAchievements);

	// fish targets:
	jsonLoadUtils::setFishTargets(mTargetToVoyageIdMap, mVoyages, mFishes);

	// targets by voyage name:
	jsonLoadUtils::setVoyageTargets(mTargetToVoyageIdMap, mVoyages, mStops);
	
	// special targets:
	jsonLoadUtils::setSpecialTargets(mTargetToVoyageIdMap, mVoyages);

	mIsInit = true;
}

/**
	@brief gets the next voyage number

	@param[out] nextVoyageId the voyage id used
	@param[in] startTime the time to start counting from.
	@param[in] voyageIds A set of voyage ids we are looking for. The closest time is returned out of all the voyages.
	@param[in] skips number of windows to skip over. Default is 0.

	@return true if successful
**/
bool FFXIVOceanFishingProcessor::getNextVoyage(
	uint32_t& nextVoyageId,
	const time_t& startTime,
	const std::unordered_set<uint32_t>& voyageIds,
	const uint32_t skips
)
{
	uint32_t relativeSecondsTillNextVoyage = 0;
	uint32_t relativeWindowTime = 0;
	return getSecondsUntilNextVoyage(relativeSecondsTillNextVoyage, relativeWindowTime, nextVoyageId, startTime, voyageIds, skips);
}

/**
	@brief gets the number of seconds until the next window. If already in a window, it will also get the seconds left in that window

	@param[out] secondsTillNextVoyage number of seconds until the next window, not including the one we are currently in
	@param[out] secondsLeftInWindow number of seconds left in the current window. Is set to 0 if not in a current window
	@param[out] nextVoyageId the voyage id used
	@param[in] startTime the time to start counting from.
	@param[in] voyageIds A set of voyage ids we are looking for. The closest time is returned out of all the voyages.
	@param[in] skips number of windows to skip over. Default is 0.

	@return true if successful
**/
bool FFXIVOceanFishingProcessor::getSecondsUntilNextVoyage(
	uint32_t& secondsTillNextVoyage,
	uint32_t& secondsLeftInWindow,
	uint32_t& nextVoyageId,
	const time_t& startTime,
	const std::unordered_set<uint32_t> & voyageIds,
	const uint32_t skips
)
{
	bool nextVoyageUpdated = false;
	secondsTillNextVoyage = UINT32_MAX;
	secondsLeftInWindow = 0;

	if (voyageIds.empty())
		return false;

	// Get the status of where we are currently
	uint32_t currBlockIdx = convertTimeToBlockIndex(startTime);

	// Cycle through the voyage pattern until we get a match to a voyage we are looking for.
	uint32_t skipcounts = 0;
	uint32_t maxCycles = 1000; // limit cycles just in case
	for (uint32_t i = 0; i < maxCycles; i++)
	{
		// Current place in the pattern we are looking at.
		uint32_t wrappedIdx = getVoyagePatternIndex(currBlockIdx, i);

		// Check to see if we match any of our desired voyages
		// TODO: exit cycle loop if we went through entire pattern with no match, remove maxCycles
		if (voyageIds.contains(mVoyagePattern[wrappedIdx]))
		{
			// Find the difference in time from the pattern position to the current time
			time_t voyageTime = convertBlockIndexToTime(currBlockIdx + i);
			int timeDifference = static_cast<int>(difftime(voyageTime, startTime));

			// If the time of the voyage is more than 15m behind us, then it's not a valid voyage
			if (timeDifference < -60 * 15)
			{
				continue;
			}

			// If we are skipping voyages, skip now
			if (skipcounts < skips)
			{
				skipcounts++;
				continue;
			}

			// If we reach here we found a valid voyage.
			// If timeDifference <= 0, that means we are in a window,
			// but we still want the time of the next voyage, so don't return
			// and continue for another cycle to get a positive timeDifference

			if (!nextVoyageUpdated) // remember the next voyage. Use the current window as the voyage if we are in window
			{
				nextVoyageId = mVoyagePattern[wrappedIdx];
				nextVoyageUpdated = true;
			}
			if (timeDifference <= 0) // needs to have the = also otherwise trigging this on the turn of the hour will not record that we're in a window
			{
				secondsLeftInWindow = static_cast<uint32_t>(60 * 15 + timeDifference);
			}
			else
			{
				secondsTillNextVoyage = static_cast<uint32_t>(timeDifference);
				return true;
			}
		}
	}
	return false;
}

/**
	@brief gets the voyage name at a selected time, with option to skip. If in a window, that window is the voyages name. If not, the next window will be the name.

	@param[in] t the time to start the check
	@param[in] skips number of windows to skip over. Default is 0.

	@return name of the voyage
**/
std::string FFXIVOceanFishingProcessor::getNextVoyageName(const time_t& t, const uint32_t skips)
{
	uint32_t currBlockIdx = convertTimeToBlockIndex(t);

	uint32_t skipcounts = 0;
	const uint32_t maxCycles = 1000; // limit cycles just in case
	for (uint32_t i = 0; i < maxCycles; i++)
	{
		// Find the difference in time from the pattern position to the current time
		time_t voyageTime = convertBlockIndexToTime(currBlockIdx + i);
		int timeDifference = static_cast<int>(difftime(voyageTime, t));

		// If the time of the voyage is more than 15m behind us, then it's not a valid voyage
		if (timeDifference < -60 * 15)
		{
			continue;
		}

		// If we are skipping voyages, skip now
		if (skipcounts < skips)
		{
			skipcounts++;
			continue;
		}

		// get voyage index
		uint32_t voyageIdx = mVoyagePattern[getVoyagePatternIndex(currBlockIdx + i)];

		// convert from id
		if (mVoyageIdToNameMap.contains(voyageIdx))
		{
			const std::string voyageName = mVoyageIdToNameMap.at(voyageIdx);
			if (mVoyages.contains(voyageName))
				return voyageName;
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
time_t FFXIVOceanFishingProcessor::convertBlockIndexToTime(const uint32_t blockIdx)
{
	return static_cast<time_t>(blockIdx - (mPatternOffset - 1)) * 60 * 60 * 2;
}

/**
	@brief converts a time to a block index
	       this algorithm was obtained from https://github.com/proyebat/FFXIVOceanFishingTimeCalculator

	@param[in] t the time_t to convert

	@return the block index
**/
uint32_t FFXIVOceanFishingProcessor::convertTimeToBlockIndex(const time_t& t)
{
	struct tm t_struct {};
	localtime_s(&t_struct, &t);

	// if we are within the 15 boat window, we are still inside the block. Account for this.
	if (t_struct.tm_min < 15)
	{
		t_struct.tm_min -= 15;
	}
	time_t alignedTime = mktime(&t_struct);
	return static_cast<uint32_t>(std::ceil(alignedTime / (60 * 60 * 2))) + mPatternOffset;
}

/**
	@brief gets the voyage's id from the pattern given a block index

	@param[in] blockIdx the block to convert
	@param[in] jump number of steps to jump ahead, default is 0

	@return the voyage id at the blockIdx + jump
**/
uint32_t FFXIVOceanFishingProcessor::getVoyagePatternIndex(const uint32_t blockIdx, const uint32_t jump)
{
	return (blockIdx + jump) % mVoyagePattern.size();
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
	for (const auto& achievement : mVoyages.at(voyageName).achievements)
	{
		if (!firstAchievement)
			achievementName += "-";
		achievementName += achievement;
		firstAchievement = false;
	}
	
	return achievementName;
}

/**
	@brief converts a voyageId to image name

	@param[in] voyageId the voyageId to get the image for
	@param[in] priority whether to prioritize achievement name or blue fish name

	@return the image name

	@relatesalso getImageName
**/
std::string FFXIVOceanFishingProcessor::createImageNameFromVoyageId(const uint32_t& voyageId, PRIORITY priority)
{
	if (!mVoyageIdToNameMap.contains(voyageId))
		return "";
	std::string voyageName = mVoyageIdToNameMap.at(voyageId);
	if (!mVoyages.contains(voyageName))
		return "";

	std::string blueFishName = "";
	std::string blueFishPattern = mVoyages.at(voyageName).blueFishPattern;
	if (mTargetToVoyageIdMap.contains("Blue Fish Pattern") &&
		mTargetToVoyageIdMap.at("Blue Fish Pattern").contains(blueFishPattern))
		blueFishName = mTargetToVoyageIdMap.at("Blue Fish Pattern").at(blueFishPattern).imageName;
	std::string achievementName = createAchievementName(voyageName);

	if ((priority == PRIORITY::ACHIEVEMENTS && !achievementName.empty()) || blueFishName.empty())
		return achievementName;
	else
		return blueFishName;
}

/**
	@brief converts a voyageId to button label

	@param[in] voyageId the voyageId to get the image for
	@param[in] priority whether to prioritize achievement name or blue fish name

	@return the button label

	@relatesalso getButtonLabel
**/
std::string FFXIVOceanFishingProcessor::createButtonLabelFromVoyageId(const uint32_t& voyageId, PRIORITY priority)
{
	if (!mVoyageIdToNameMap.contains(voyageId))
		return "";
	std::string voyageName = mVoyageIdToNameMap.at(voyageId);
	if (!mVoyages.contains(voyageName))
		return "";

	std::string blueFishName = mVoyages.at(voyageName).blueFishPattern;
	std::string achievementName = createAchievementName(voyageName);

	if ((priority == PRIORITY::ACHIEVEMENTS && !achievementName.empty()) || blueFishName.empty())
		return achievementName;
	else
		return blueFishName;
}

/**
	@brief get the image name and label for a particular tracking

	@param[out] imageName the name of the png image for this tracker
	@param[out] buttonLabel the string label for this button
	@param[in] voyageName the name of the voyage
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
	if (mTargetToVoyageIdMap.contains(tracker))
	{
		if (mTargetToVoyageIdMap.at(tracker).contains(name))
		{
			buttonLabel = mTargetToVoyageIdMap.at(tracker).at(name).labelName;
			imageName = mTargetToVoyageIdMap.at(tracker).at(name).imageName;

			// if labelName or imageName not provided, we need to create them
			if (buttonLabel.empty() || imageName.empty())
			{
				// first get voyageId for this tracker
				std::unordered_set<uint32_t> voyageIds = getVoyageIdByTracker(tracker, name);

				// get next voyage
				uint32_t nextVoyage;
				if (getNextVoyage(nextVoyage, startTime, voyageIds, skips))
				{
					// create the names
					if (buttonLabel.empty())
						buttonLabel = createButtonLabelFromVoyageId(nextVoyage, priority);
					if (imageName.empty())
						imageName = createImageNameFromVoyageId(nextVoyage, priority);
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
	for (const auto& type : mTargetToVoyageIdMap)
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
	for (const auto& type : mTargetToVoyageIdMap)
		j.push_back(type.first);
	std::sort(j.begin(), j.end());
	return j;
}

/**
	@brief converts a target to a set of voyage ids that matches the target

	@param[in] type the targets type
	@param[in] name the targets name

	@return a set of voyage ids if found, and a null set if the voyage is not found
**/
std::unordered_set<uint32_t> FFXIVOceanFishingProcessor::getVoyageIdByTracker(const std::string& tracker, const std::string& name)
{
	// this map was precomputed in initializer
	if (mTargetToVoyageIdMap.contains(tracker))
		if (mTargetToVoyageIdMap.at(tracker).contains(name))
			return mTargetToVoyageIdMap.at(tracker).at(name).ids;
	return {};
}