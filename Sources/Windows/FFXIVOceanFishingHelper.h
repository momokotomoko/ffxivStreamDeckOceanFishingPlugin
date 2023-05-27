//==============================================================================
/**
@file       FFXIVOceanFishingHelper.h
@brief      Handles multiple instances of processor for various fishing routes
@copyright  (c) 2023, Momoko Tomoko
**/
//==============================================================================

#pragma once

#include "FFXIVOceanFishingProcessor.h"
#include <string>
#include <vector>
#include <unordered_map>

#include "../Vendor/json/src/json.hpp"
using json = nlohmann::json;

class FFXIVOceanFishingHelper
{
public:
	FFXIVOceanFishingHelper(const std::vector<std::string>& dataFiles);
	~FFXIVOceanFishingHelper() {};

	bool isInit()
	{
		for (const auto& processor : processors)
			if (!processor.second->isInit())
				return false;
		return true;
	};

	std::string getErrorMessage()
	{
		std::string err;
		for (const auto& processor : processors)
		{
			std::string msg = processor.second->getErrorMessage();

			if (!msg.empty())
				err += processor.first
					+ " error message:\n"
					+ msg;
		}
		return err;
	};

	bool getSecondsUntilNextVoyage(
		uint32_t& secondsTillNextVoyage,
		uint32_t& secondsLeftInWindow,
		const time_t& startTime,
		const std::unordered_set<uint32_t>& voyageIds,
		const std::string& routeNameUsed,
		const uint32_t skips = 0
	);

	std::unordered_set<uint32_t> getVoyageIdByTracker(
		const std::string& routeName,
		const std::string& tracker,
		const std::string& name
	);

	void getImageNameAndLabel(
		std::string& imageName,
		std::string& buttonLabel,
		const std::string& routeName,
		const std::string& tracker,
		const std::string& name,
		const time_t& startTime,
		const PRIORITY priority,
		const uint32_t skips
	);
	json getTargetsJson(const std::string& routeName);
	json getTrackerTypesJson(const std::string& routeName);
	json getRouteNames();

private:
	std::unordered_map<std::string, std::unique_ptr<FFXIVOceanFishingProcessor>> processors;
};