//==============================================================================
/**
@file       FFXIVOceanFishingJsonLoadUtils.h
@brief      Functions that helps with loading the json database.
@copyright  (c) 2023, Momoko Tomoko
**/
//==============================================================================

#pragma once

#include <optional>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include "Common.h"
#include "../Vendor/json/src/json.hpp"

namespace jsonLoadUtils
{
	using json = nlohmann::json;

	bool isBadKey(const json& j, const std::string& key, const json::value_t& type);

	std::optional<std::string> loadStops(std::unordered_map<std::string, std::string>& stops, const json& j);
	std::optional<std::string> loadRouteName(std::string& routeName, const json& j);
	std::optional<std::string> loadVoyageSchedule(std::vector<uint32_t>& voyagePattern, uint32_t& offset, const json& j);
	std::unordered_set<std::string> loadJsonStringArray(const json& j);
	std::optional<std::string> loadFishLocations(std::unordered_map<std::string, locations_t>& locations, const json& fish);
	std::optional<std::string> loadFish(
		std::unordered_map<std::string, std::unordered_map<std::string, fish_t>>& fishes,
		std::set<std::string>& blueFishNames,
		const json& j);
	std::optional<std::string> loadAchievements(
		std::unordered_map<std::string, std::unordered_set<uint32_t>>& achievements,
		const json& j);
	std::unordered_set<std::string> getFishAtStop(
		const std::string stopName,
		const std::string stopTime,
		const std::unordered_map<std::string, std::unordered_map<std::string, fish_t>>& fishes
	);
	std::optional<std::string> loadVoyageStops(
		std::vector<stop_t>& voyageStops,
		const std::unordered_map<std::string, std::unordered_map<std::string, fish_t>>& fishes,
		const json& j
	);
	std::unordered_set<std::string> getAchievementsForVoyage(
		const uint32_t voyageId,
		const std::unordered_map<std::string, std::unordered_set<uint32_t>>& achievements
	);
	std::vector<std::unordered_set<std::string>> getBlueFishAtStops(
		const std::vector<stop_t>& stops,
		const std::set<std::string>& blueFishNames);
	std::string implodeStringVector(const std::vector<std::string>& strings, const char delim = '-');
	std::string createBlueFishPattern(
		const std::vector<std::unordered_set<std::string>>& blueFishPerStop,
		const std::unordered_map<std::string, fish_t>& blueFishes
	);
	std::optional<std::string> loadVoyages(
		std::unordered_map <std::string, voyage_t>& voyages,
		std::unordered_map <uint32_t, std::string>& voyageIdToNameMap,
		const std::unordered_map<std::string, std::unordered_map<std::string, fish_t>>& fishes,
		const std::set<std::string>& blueFishNames,
		const std::unordered_map<std::string, std::unordered_set<uint32_t>>& achievements,
		const json& j
	);
	template <typename T>
	void targetInserter(std::map<std::string, targets_t>& map, T& newTargets);
	void setBlueFishTargets(
		std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
		std::unordered_map <std::string, voyage_t>& voyages,
		const std::set<std::string>& blueFishNames
	);
	void setAchievementTargets(
		std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
		const std::unordered_map<std::string, std::unordered_set<uint32_t>>& achievements
	);
	void setFishTargets(
		std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
		const std::unordered_map <std::string, voyage_t>& voyages,
		const std::unordered_map<std::string, std::unordered_map<std::string, fish_t>>& fishes
	);
	void setVoyageTargets(
		std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
		const std::unordered_map <std::string, voyage_t>& voyages,
		const std::unordered_map<std::string, std::string>& stops
	);
	void setSpecialTargets(
		std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
		const std::unordered_map <std::string, voyage_t>& voyages
	);
}