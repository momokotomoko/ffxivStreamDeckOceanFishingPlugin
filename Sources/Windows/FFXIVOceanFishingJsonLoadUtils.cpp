//==============================================================================
/**
@file       FFXIVOceanFishingJsonLoadUtils.cpp
@brief      Functions that helps with loading the json database.
@copyright  (c) 2023, Momoko Tomoko
**/
//==============================================================================

#pragma once
#include "pch.h"
#include "FFXIVOceanFishingJsonLoadUtils.h"
#include <optional>
#include <string>
#include <map>
#include <algorithm>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <ostream>
#include "Common.h"
#include "../Vendor/json/src/json.hpp"

namespace jsonLoadUtils
{
	using json = nlohmann::json;

	/**
		@brief check if json contains key

		@param[in] j json to check
		@param[in] key the key to look for

		@return true if invalid key
	**/
	bool isBadKey(const json& j, const std::string& key, const json::value_t& value)
	{
		return !(j.contains(key) && ((j[key].type() == value)));
	}

	/**
		@brief loads stops from json database

		@param[out] stops the stops, mapped by name -> shortform name
		@param[in] j json to load

		@return error message string, or std::nullopt if no errors
	**/
	std::optional<std::string> loadStops(std::unordered_map<std::string, std::string>& stops, const json& j)
	{
		if (isBadKey(j, "stops", json::value_t::object)) return "Invalid/missing stops in database.\nJson Dump:\n" + j.dump(4);

		for (const auto& [stopname, jsonData] : j["stops"].get<json::object_t>())
		{
			std::string shortform = stopname;
			if (!isBadKey(jsonData, "shortform", json::value_t::string))
				shortform = jsonData["shortform"].get<std::string>();
			stops.insert({ stopname, shortform });
		}
		return std::nullopt;
	}

	/**
		@brief loads a route name

		@param[out] routeName the name of the route
		@param[in] j json to load

		@return error message string, or std::nullopt if no errors
	**/
	std::optional<std::string> loadRouteName(std::string& routeName, const json& j)
	{
		if (isBadKey(j, "name", json::value_t::string)) return "Invalid/missing route name in database.\nJson Dump:\n" + j.dump(4);
		routeName = j["name"].get<std::string>();
		return std::nullopt;
	}

	/**
		@brief loads voyage schedule, consisting of the pattern and offset

		@param[out] voyagePattern the array of ids for the voyage pattern
		@param[out] offset the offset number
		@param[in] j json to load

		@return error message string, or std::nullopt if no errors
	**/
	std::optional<std::string> loadVoyageSchedule(std::vector<uint32_t>& voyagePattern, uint32_t& offset, const json& j)
	{
		if (isBadKey(j, "schedule", json::value_t::object)) return "Invalid/missing schedule in database.\nJson Dump:\n" + j.dump(4);
		if (isBadKey(j["schedule"], "pattern", json::value_t::array)) return "Invalid/missing pattern in schedule.\nJson Dump:\n" + j["schedule"].dump(4);
		if (isBadKey(j["schedule"], "offset", json::value_t::number_unsigned)) return "Invalid/missing offset in schedule.\nJson Dump:\n" + j["schedule"].dump(4);

		for (const auto& id : j["schedule"]["pattern"])
		{
			if (!id.is_number_unsigned())
				return "Invalid pattern in schedule: " + id.dump(4) + "\n" + j["schedule"].dump(4);
			voyagePattern.push_back(id.get<uint32_t>());
		}

		offset = j["schedule"]["offset"].get<uint32_t>();

		if (voyagePattern.empty()) return "Voyage pattern was empty:\n" + j.dump(4);

		return std::nullopt;
	}

	/**
		@brief get either a single or list of values from json
	**/
	std::unordered_set<std::string> loadJsonStringArray(const json& j)
	{
		return j
			| std::views::transform([&](json element) { return element.get<std::string>(); })
			| std::ranges::to<std::unordered_set<std::string>>();
	}

	/**
		@brief gets locations for a single fish from json

		@param[out] locations the locations the fish can be found at
		@param[in] fish the fish entry in the json database

		@return error message string, or std::nullopt if no errors
	**/
	std::optional<std::string> loadFishLocations(std::unordered_map<std::string, locations_t>& locations, const json& fishJson)
	{
		if (!fishJson.contains("locations")) return std::nullopt; // having no locations is allowed, return no error

		// if fishJson is an object, convert to array. Otherwise we only accept arrays.
		json j = fishJson["locations"];
		if (fishJson["locations"].type() == json::value_t::object)
			j = json::array({ j });
		else if (j.type() != json::value_t::array) return "Invalid location for fish.\nJson Dump:\n" + fishJson.dump(4);

		for (const auto& location : j)
		{
			bool noTime = isBadKey(location, "time", json::value_t::string) && isBadKey(location, "time", json::value_t::array);
			bool noName = isBadKey(location, "name", json::value_t::string);
			if (noName && noTime) continue; // null location is allowed, just skip
			if (noName && !noTime) return "Location has time but missing name: \n" + location.dump(4);

			// location["time"] can be a single entry ("time": "day") or an array ("time": ["day", night"])
			// an empty time vector means any time is allowed
			std::unordered_set<std::string> times;
			if (!noTime) times = loadJsonStringArray(location["time"]);
			std::string locationName = location["name"].get<std::string>();
			locations.insert({ locationName, {locationName, times} });
		}
		return std::nullopt;
	}

	/**
		@brief loads fish from json database

		@param[out] fishes map of fishes as name -> fish_t struct
		@param[out] blueFisheNames map of blue fishes as name -> fish_t struct
		@param[in] j json to load

		@return error message string, or std::nullopt if no errors
	**/
	std::optional<std::string> loadFish(
		std::unordered_map<std::string, std::unordered_map<std::string, fish_t>>& fishes,
		std::set<std::string>& blueFishNames,
		const json& j)
	{
		if (isBadKey(j, "targets", json::value_t::object)) return "Invalid/missing targets in database.\nJson Dump:\n" + j.dump(4);
		if (isBadKey(j["targets"], "fish", json::value_t::object)) return "Invalid/missing fish in targets.\nJson Dump:\n" + j.dump(4);

		std::optional<std::string> err;
		for (const auto& fishType : j["targets"]["fish"].get<json::object_t>())
		{
			const auto& [fishTypeName, fishJson] = fishType;
			fishes.insert({ fishTypeName, {} });
			for (const auto& fish : fishJson.get<json::object_t>())
			{
				const auto& [fishName, fishJson] = fish;

				std::optional<std::string> fishShortName = std::nullopt;
				if (fishJson.contains("shortform"))
					fishShortName = fishJson["shortform"].get<std::string>();

				std::unordered_map<std::string, locations_t> locations;
				err = loadFishLocations(locations, fishJson);
				if (err) return err;

				fish_t newFish = {
					.name = fishName,
					.type = fishTypeName,
					.shortName = fishShortName,
					.locations = locations
				};
				fishes.at(fishTypeName).insert({ fishName, newFish });
				if (fishTypeName == "Blue Fish")
					blueFishNames.insert(fishName);
			}
		}
		return std::nullopt;
	}

	/**
		@brief loads achievements from json database

		@param[out] achievements as a map of name -> associated voyage ids
		@param[in] j json to load

		@return error message string, or std::nullopt if no errors
	**/
	std::optional<std::string> loadAchievements(
		std::unordered_map<std::string, std::unordered_set<uint32_t>>& achievements,
		const json& j)
	{
		if (isBadKey(j, "targets", json::value_t::object)) return "Invalid/missing targets in database.\nJson Dump:\n" + j.dump(4);
		if (isBadKey(j["targets"], "achievements", json::value_t::object)) return "Invalid/missing achievements in targets.\nJson Dump:\n" + j["targets"].dump(4);

		std::optional<std::string> err;
		for (const auto& achievement : j["targets"]["achievements"].get<json::object_t>())
		{
			const auto& [achievementName, achievementDataJson] = achievement;
			if (isBadKey(achievementDataJson, "voyageIds", json::value_t::array)) return "Invalid/missing voyageId in achievements.\nJson Dump:\n" + achievementDataJson.dump(4);

			std::unordered_set <uint32_t> ids = achievementDataJson["voyageIds"]
				| std::views::transform([&](json j) { return j.get<uint32_t>(); })
				| std::ranges::to< std::unordered_set <uint32_t>>();
			achievements.insert({ achievementName, ids });
		}
		return std::nullopt;
	}

	/**
		@brief gets the available fish at a stop that matches the stop's name and time

		@param[in] stopName the name of the stop
		@param[in] stopTime the time of the stop
		@param[in] fishes the container of all the fishes

		@return set of fishes that match
	**/
	std::unordered_set<std::string> getFishAtStop(
		const std::string stopName,
		const std::string stopTime,
		const std::unordered_map<std::string, std::unordered_map<std::string, fish_t>>& fishes
	)
	{
		// function to check if fish matches the stop
		auto isFishAtStop = [&](auto const& fish)
			{
				const auto& [_, fishData] = fish;
				return fishData.locations.contains(stopName) &&
					(fishData.locations.at(stopName).time.empty() ||
						fishData.locations.at(stopName).time.contains(stopTime));
			};

		// go through all fishes, return all the ones that match the time
		return fishes
			| std::views::transform([&](auto const& fish)
				{
					const auto& [_, fishes] = fish; // [fish type name, fishes]
					return fishes
						| std::views::filter(isFishAtStop) // remove non-matching times
						| std::views::transform([](auto const& fish) { return fish.first; }); // get name of the fish
				}) // matching fishes per fish type
			| std::views::join
					| std::ranges::to<std::unordered_set<std::string>>();
	}

	/**
		@brief loads voyage stops from json database

		@param[out] voyageStops vector of stops for a voyage
		@param[in] fishes the container of all the fishes
		@param[in] j json to load

		@return error message string, or std::nullopt if no errors
	**/
	std::optional<std::string> loadVoyageStops(
		std::vector<stop_t>& voyageStops,
		const std::unordered_map<std::string, std::unordered_map<std::string, fish_t>>& fishes,
		const json& j
	)
	{
		if (isBadKey(j, "stops", json::value_t::array)) return "Invalid/missing stops in voyage.\nJson Dump:\n" + j.dump(4);

		for (const auto& stop : j["stops"])
		{
			if (isBadKey(stop, "name", json::value_t::string)) return "Invalid/missing name in stops.\nJson Dump:\n" + j.dump(4);
			if (isBadKey(stop, "time", json::value_t::string)) return "Invalid/missing time in stops.\nJson Dump:\n" + j.dump(4);

			const std::string stopName = stop["name"].get<std::string>();
			const std::string stopTime = stop["time"].get<std::string>();

			voyageStops.push_back({ {stopName, {stopTime} } , getFishAtStop(stopName, stopTime, fishes) });
		}
		return std::nullopt;
	}

	/**
		@brief gets all achievements associated with a voyage

		@param[in] voyageId the id of the voyage to get achievements for
		@param[in] achievements the container of all the achievements

		@return set of achievement names associated with the voyage id
	**/
	std::unordered_set<std::string> getAchievementsForVoyage(
		const uint32_t voyageId,
		const std::unordered_map<std::string, std::unordered_set<uint32_t>>& achievements
	)
	{
		return achievements
			| std::views::filter([&](auto const& kv_pair) { return kv_pair.second.contains(voyageId); })
			| std::views::transform([](auto const& kv_pair) { return kv_pair.first; })
			| std::ranges::to<std::unordered_set<std::string>>();
	}

	/**
		@brief gets all the blue fish at each stop

		@param[in] voyageStops vector of stops for a voyage
		@param[in] blueFishNames the names of blue fish

		@return a vector of stops containing the unordered_set of blue fish at that stop
	**/
	std::vector<std::unordered_set<std::string>> getBlueFishAtStops(
		const std::vector<stop_t>& stops,
		const std::set<std::string>& blueFishNames)
	{
		return stops
			| std::views::transform([&](const stop_t& stop) -> std::unordered_set<std::string>
				{
					return stop.fish
						| std::views::filter([&](auto const& fishName) { return blueFishNames.contains(fishName); })
						| std::ranges::to<std::unordered_set<std::string>>(); // set of blue fish at this stop
				})
			| std::ranges::to<std::vector<std::unordered_set<std::string>>>();
	}

	/**
		@brief combines a container strings with delimiter in between

		@param[in] itr container of strings to combine
		@param[in] delim delimiter to insert between the strings

		@return combined string
	**/
	std::string implodeStringVector(const std::vector<std::string>& strings, const char delim)
	{
		return std::ranges::fold_left(strings | std::views::join_with(delim), std::string{}, std::plus<>{});
	}

	/**
		@brief create pattern string as fish1-fish2-fish3, and use X if there is no blue fish

		@param[in] blueFishPerStop set of blue fish per stop
		@param[in] blueFishes the container of all the blue fishes

		@return blue fish pattern string, with X-X-X if there are no blue fish
	**/
	std::string createBlueFishPattern(
		const std::vector<std::unordered_set<std::string>>& blueFishPerStop,
		const std::unordered_map<std::string, fish_t>& blueFishes
	)
	{
		// TODO this currently only handles names for when there is only one blue fish per stop,
		// if this changes in the future need to change *blueFishes.begin() to something else
		const std::string noBlueFishDefaultName = "X";
		std::vector<std::string> shortenedNames = blueFishPerStop
			| std::views::transform(
				[&](const auto& fishes) -> const std::string {
					if (fishes.empty()) return noBlueFishDefaultName;
					const std::string& fishName = *fishes.begin();
					if (!blueFishes.contains(fishName)) return fishName;
					return blueFishes.at(fishName).shortName.value_or(fishName);
				}) // convert name to shortform name, and nullopt to X
			| std::ranges::to<std::vector<std::string>>();
				return implodeStringVector(shortenedNames);
	}

	/**
		@brief loads voyages from json database

		@param[out] voyages map of voyage name -> voyage_t
		@param[out] voyageIdToNameMap map of voyage id to voyage name
		@param[in] fishes the container of all the fishes
		@param[in] blueFishNames the names of blue fish
		@param[in] achievements the container of all the achievements
		@param[in] j json to load

		@return error message string, or std::nullopt if no errors
	**/
	std::optional<std::string> loadVoyages(
		std::unordered_map <std::string, voyage_t>& voyages,
		std::unordered_map <uint32_t, std::string>& voyageIdToNameMap,
		const std::unordered_map<std::string, std::unordered_map<std::string, fish_t>>& fishes,
		const std::set<std::string>& blueFishNames,
		const std::unordered_map<std::string, std::unordered_set<uint32_t>>& achievements,
		const json& j
	)
	{
		if (isBadKey(j, "voyages", json::value_t::object)) return "Invalid/missing voyages in database.\nJson Dump:\n" + j.dump(4);

		for (const auto& voyage : j["voyages"].get<json::object_t>())
		{
			const auto& [voyageName, voyageData] = voyage;
			if (voyages.contains(voyageName)) return "Error: duplicate voyage name in json: " + voyageName; // TODO: json parse might remove duplicates already
			if (isBadKey(voyageData, "id", json::value_t::number_unsigned)) return "Invalid/missing id for voyage.\nJson Dump:\n" + voyageData.dump(4);
			const uint32_t id = voyageData["id"].get<uint32_t>();
			if (voyageIdToNameMap.contains(id)) return "Error: duplicate voyage id in json: " + id;
			if (isBadKey(voyageData, "shortform", json::value_t::string)) return "Invalid/missing shortform name for voyage.\nJson Dump:\n" + voyageData.dump(4);

			std::vector<stop_t> voyageStops;
			auto err = loadVoyageStops(voyageStops, fishes, voyageData);
			if (err) return err;

			std::string blueFishPattern = "";
			auto blueFishPerStop = getBlueFishAtStops(voyageStops, blueFishNames);
			bool isNoBlueFish = std::ranges::find_if(blueFishPerStop, [](const auto& blueFishes) { return !blueFishes.empty(); }) == blueFishPerStop.end();

			voyages.insert({ voyageName,
				{
					voyageData["shortform"].get<std::string>(),
					id,
					voyageStops,
					getAchievementsForVoyage(id, achievements),
					// by default with 0 blue fish use "" as the pattern
					isNoBlueFish ? "" : createBlueFishPattern(blueFishPerStop, fishes.at("Blue Fish"))
				}
				});
			voyageIdToNameMap.insert({ id, voyageName });
		}
		return std::nullopt;
	}

	/**
		@brief inserts new target into map of targets, which requires merging the ids

		@param[in] map the map of targets
		@param[in] newTargets the new target to insert
	**/
	template <typename T>
	void targetInserter(std::map<std::string, targets_t>& map, T& newTargets)
	{
		std::ranges::for_each(newTargets,
			[&](const auto& newItem)
			{
				const auto& [name, target] = newItem;
				if (map.contains(name))
					map.at(name).ids.insert(target.ids.begin(), target.ids.end());
				else
					map.insert(newItem);
			}
		);
	}

	/**
		@brief create targets for blue fish

		@param[out] targetToVoyageIdMap map of target -> voyage id
		@param[in] voyages the container of all the voyages
		@param[in] blueFishNames the names of blue fish
	**/
	void setBlueFishTargets(
		std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
		std::unordered_map <std::string, voyage_t>& voyages,
		const std::set<std::string>& blueFishNames
	)
	{
		// function to return all the blue fish given a stop
		auto blueFishAtStop = [&](const auto& stop)
			{
				return stop.fish
					| std::views::filter([&](const auto& fish) { return blueFishNames.contains(fish); });
			};

		// function to return all the blue fish in a voyage
		auto blueFishInVoyage = voyages
			| std::views::transform([&](const auto& voyage) {
			return voyage.second.stops
				| std::views::transform(blueFishAtStop)
				| std::views::join
				| std::ranges::to<std::vector<std::string>>();
				});

		// function to make targets
		auto makeTargets = [](const auto& zipped)
			{
				const auto& [blueFish, voyageMap] = zipped;
				const auto& [_, voyage] = voyageMap;
				std::string imageName = voyage.blueFishPattern;
				if (blueFish.size() == 1)
					imageName = blueFish.front();
				targets_t newTarget{
						.labelName = voyage.blueFishPattern,
						.imageName = imageName,
						.ids = {voyage.id}
				};
				return std::make_pair(voyage.blueFishPattern, newTarget);
			};

		// make all the blue fish targets and store them
		auto newTargets = std::views::zip(blueFishInVoyage, voyages)
			| std::views::filter(
				[](const auto& zipped)
				{
					const auto& [blueFish, _] = zipped;
					return !(blueFish.empty());
				})
			| std::views::transform(makeTargets);

				targetToVoyageIdMap.insert({ "Blue Fish Pattern", {} });
				targetInserter(targetToVoyageIdMap.at("Blue Fish Pattern"), newTargets);
	}

	/**
		@brief create targets for achievements

		@param[out] targetToVoyageIdMap map of target -> voyage id
		@param[in] achievements the container of all the achievements
	**/
	void setAchievementTargets(
		std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
		const std::unordered_map<std::string, std::unordered_set<uint32_t>>& achievements
	)
	{
		auto newTargets = achievements
			| std::views::transform([](const auto& achievement) -> std::pair<std::string, targets_t>
				{
					const auto& [achievementName, ids] = achievement;
					// achievement label and imagename are the same as just the acheivement name
					targets_t newTarget{
							.labelName = achievementName,
							.imageName = achievementName,
							.ids = ids
					};
					return std::make_pair(achievementName, newTarget);
				});
		targetToVoyageIdMap.insert({ "Achievement", {} });
		targetInserter(targetToVoyageIdMap.at("Achievement"), newTargets);
	}

	/**
		@brief create targets for fishes

		@param[out] targetToVoyageIdMap map of target -> voyage id
		@param[in] voyages the container of all the voyages
		@param[in] fishes the container of all the fishes
	**/
	void setFishTargets(
		std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
		const std::unordered_map <std::string, voyage_t>& voyages,
		const std::unordered_map<std::string, std::unordered_map<std::string, fish_t>>& fishes
	)
	{
		// function for checking if a voyage contains a fish with specific name at one of its stops
		auto doesVoyageContainFish = [](const auto voyage, const std::string& fishName)
			{
				// TODO: would converting to set be faster?
				auto fishesInVoyage = voyage.second.stops
					| std::views::transform([&](const auto& stop) { return stop.fish; })
					| std::views::join; // get all the fishes from all the stops
				auto fishFound = std::ranges::find(fishesInVoyage, fishName);
				return fishFound != fishesInVoyage.end();
			};

		auto makeFishTargets = [&](const auto fish)
			{
				const auto& [fishName, _] = fish;

				std::unordered_set <uint32_t> ids = voyages
					| std::views::filter([&](const auto& voyage) { return  doesVoyageContainFish(voyage, fishName); })
					| std::views::transform([&](const auto& voyage) { return voyage.second.id; })
					| std::ranges::to<std::unordered_set <uint32_t >>();
				// fish label and imagename are the same as just the fish name
				return std::make_pair(fishName, targets_t{ fishName, fishName, ids });
			};

		auto newTargetMap = fishes
			| std::views::transform(
				[&](const auto fishType)
				{
					const auto& [fishTypeName, fishData] = fishType;
					auto newTargets = fishData
						| std::views::transform(makeFishTargets)
						| std::ranges::to<std::map<std::string, targets_t>>();
					return std::make_pair(fishTypeName, newTargets);
				});
		targetToVoyageIdMap.insert(newTargetMap.begin(), newTargetMap.end());
	}

	/**
		@brief create targets for voyages

		@param[out] targetToVoyageIdMap map of target -> voyage id
		@param[in] voyages the container of all the voyages
		@param[in] stops the container of all the stops
	**/
	void setVoyageTargets(
		std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
		const std::unordered_map <std::string, voyage_t>& voyages,
		const std::unordered_map<std::string, std::string>& stops
	)
	{
		// TODO merge this with later code blocks when std::views:concat is in c++23
		targetToVoyageIdMap.insert({ "Voyages",
			voyages
			| std::views::transform([&](const auto& voyage) -> std::pair<std::string, targets_t>
				{
					const auto& [voyageName, voyageData] = voyage;
					return std::make_pair(voyageName, targets_t{ "", "", { voyageData.id } });
				})
			| std::ranges::to<std::map<std::string, targets_t>>()
			});

		// range of shortform names of the last stop
		auto voyageLastStopShortName = voyages
			| std::views::transform([&](const auto& voyage) { return stops.at(voyage.second.stops.back().location.name); });

		// function to make a target given zipped (voyage_t and voyage last stop name)
		auto makeTarget = [&](const auto& zipped) -> std::pair<std::string, targets_t>
			{
				const auto& [voyage, lastStopShortName] = zipped;
				const auto& [voyageName, voyageData] = voyage;
				return std::make_pair(lastStopShortName, targets_t{ "", "", { voyageData.id } });
			};

		// create the targets and save them
		auto newTargets = std::views::zip(voyages, voyageLastStopShortName)
			| std::views::transform(makeTarget);
		targetInserter(targetToVoyageIdMap.at("Voyages"), newTargets);
	}

	/**
		@brief create special targets

		@param[out] targetToVoyageIdMap map of target -> voyage id
		@param[in] voyages the container of all the voyages
	**/
	void setSpecialTargets(
		std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
		const std::unordered_map <std::string, voyage_t>& voyages
	)
	{
		targetToVoyageIdMap.insert({ "Other", {} });
		targetToVoyageIdMap.at("Other").insert({ "Next Voyage", {"", "", {} } });
		for (const auto& voyage : voyages)
			targetToVoyageIdMap.at("Other").at("Next Voyage").ids.insert(voyage.second.id);
	}
}