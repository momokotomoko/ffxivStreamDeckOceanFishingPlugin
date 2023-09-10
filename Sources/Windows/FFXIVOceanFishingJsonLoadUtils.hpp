#pragma once
#include <optional>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
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
	bool isBadKey(const json& j, const std::string& key)
	{
		return !j.contains(key);
	}

	/**
		@brief loads stops from json database

		@param[out] stops the stops, mapped by name -> shortform name
		@param[in] j json to load

		@return error message string, or std::nullopt if no errors
	**/
	std::optional<std::string> loadStops(std::unordered_map<std::string, std::string> &stops, const json& j)
	{
		if (isBadKey(j, "stops")) return "Missing stops in database.\nJson Dump:\n" + j.dump(4);

		for (const auto& stop : j["stops"].get<json::object_t>())
			stops.insert({ stop.first, stop.second["shortform"].get<std::string>() });
		return std::nullopt;
	}

	/**
		@brief loads a route name

		@param[out] routeName the name of the route
		@param[in] j json to load

		@return error message string, or std::nullopt if no errors
	**/
    std::optional<std::string> loadRouteName(std::string &routeName, const json &j)
    {
		if (isBadKey(j, "name")) return "Missing route name in database.\nJson Dump:\n" + j.dump(4);
		if (!j["name"].is_string()) return "Invalid route name:\n" + j["name"].dump(4);

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
		if (isBadKey(j, "schedule")) return "Missing schedule in database.\nJson Dump:\n" + j.dump(4);
		if (isBadKey(j["schedule"], "pattern")) return "Missing pattern in schedule.\nJson Dump:\n" + j["schedule"].dump(4);
		if (isBadKey(j["schedule"], "offset")) return "Missing offset in schedule.\nJson Dump:\n" + j["schedule"].dump(4);

		for (const auto& id : j["schedule"]["pattern"])
		{
			if (!id.is_number_unsigned())
				return "Invalid pattern in schedule: " + id.dump(4) + "\n" + j["schedule"].dump(4);
			voyagePattern.push_back(id.get<uint32_t>());
		}
				
		if (!j["schedule"]["offset"].is_number_unsigned())
			return "Invalid offset in schedule:\n" + j["schedule"].dump(4);
		offset = j["schedule"]["offset"].get<uint32_t>();

		return std::nullopt;
	}

	/**
		@brief get either a single or list of values from json
	**/
	std::unordered_set<std::string> parseSingleOrArray(const json& j)
	{
		std::unordered_set<std::string> parsedResult;
		if (j.is_array())
			for (const auto& time : j)
				parsedResult.insert(time.get<std::string>());
		else
			parsedResult.insert(j.get<std::string>());
		return parsedResult;
	}

	/**
		@brief gets locations for a single fish from json

		@param[out] locations the locations the fish can be found at
		@param[in] fish the fish entry in the json database

		@return error message string, or std::nullopt if no errors
	**/
	std::optional<std::string> getFishLocations(std::unordered_map<std::string, locations_t> & locations, const json& fish)
	{
		if (isBadKey(fish, "locations")) return "Missing location for fish.\nJson Dump:\n" + fish.dump(4);;

		for (const auto& location : fish["locations"])
		{
			bool noTime = isBadKey(location, "time");
			bool noName = isBadKey(location, "name");
			if (noName && noTime) continue; // null location is allowed, just skip
			if (noName && !noTime) return "Location has time but missing name: \n" + location.dump(4);

			// location["time"] can be a single entry ("time": "day") or an array ("time": ["day", night"])
			// an empty time vector means any time is allowed
			std::unordered_set<std::string> times;
			if (!noTime) times = parseSingleOrArray(location["time"]);
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
		std::map<std::string, fish_t> &blueFisheNames,
		const json& j)
	{
		if (isBadKey(j, "targets")) return "Missing targets in database.\nJson Dump:\n" + j.dump(4);
		if (isBadKey(j["targets"], "fish")) return "Missing fish in targets.\nJson Dump:\n" + j.dump(4);

		std::optional<std::string> err;
		for (const auto& fishType : j["targets"]["fish"].get<json::object_t>())
		{
			const std::string fishTypeName = fishType.first;
			fishes.insert({ fishTypeName, {} });
			for (const auto& fish : fishType.second.get<json::object_t>())
			{
				// get fish name and location, create a new fish and insert it into containers

				const std::string fishName = fish.first;
				std::string shortformName = fishName; // by default the shortform name is just the fish name
				if (fish.second.contains("shortform"))
					shortformName = fish.second["shortform"].get<std::string>();

				std::unordered_map<std::string, locations_t> locations;
				err = getFishLocations(locations, fish.second);
				if (err) return err;

				fish_t newFish = { shortformName, locations };
				fishes.at(fishTypeName).insert({ fishName, newFish });
				if (fishTypeName == "Blue Fish")
					blueFisheNames.insert({ fishName, newFish });
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
		std::unordered_map<std::string, std::unordered_set<uint32_t>>&achievements,
		const json& j)
	{
		if (isBadKey(j, "targets")) return "Missing targets in database.\nJson Dump:\n" + j.dump(4);
		if (isBadKey(j["targets"], "achievements")) return "Missing achievements in targets.\nJson Dump:\n" + j["targets"].dump(4);

		std::optional<std::string> err;
		for (const auto& achievement : j["targets"]["achievements"].get<json::object_t>())
		{
			if (isBadKey(achievement.second, "voyageIds")) return "Missing voyageId in achievements.\nJson Dump:\n" + achievement.second.dump(4);

			std::unordered_set <uint32_t> ids;
			for (const auto voyageId : achievement.second["voyageIds"])
			{
				ids.insert(voyageId.get<uint32_t>());
			}
			achievements.insert({ achievement.first, ids });
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
		std::unordered_set<std::string> fishList;
		// create list of fishes at stop
		for (const auto& fishType : fishes)
		{
			for (const auto& fish : fishType.second)
			{
				for (const auto& location : fish.second.locations)
				{
					if (location.second.name != stopName)
						continue;

					bool isTimeMatch = false;
					// empty time vector means any time is allowed
					if (location.second.time.empty())
						isTimeMatch = true;
					else
						// go through each time and check for any match
						for (const auto& time : location.second.time)
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
		return fishList;
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
		if (isBadKey(j, "stops")) return "Missing stops in voyage.\nJson Dump:\n" + j.dump(4);

		for (const auto& stop : j["stops"])
		{
			if (isBadKey(stop, "name")) return "Missing name in stops.\nJson Dump:\n" + j.dump(4);
			if (isBadKey(stop, "time")) return "Missing time in stops.\nJson Dump:\n" + j.dump(4);

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
		std::unordered_set<std::string> voyageAchievements;
		for (const auto& achievement : achievements)
		{
			if (achievement.second.contains(voyageId))
				voyageAchievements.insert(achievement.first);
		}
		return voyageAchievements;
	}
	
	/**
		@brief loads voyages from json database

		@param[out] voyages map of voyage name -> voyage_t
		@param[out] voyageIdToNameMap map of voyage id to voyage name
		@param[in] fishes the container of all the fishes
		@param[in] achievements the container of all the achievements
		@param[in] j json to load

		@return error message string, or std::nullopt if no errors
	**/
	std::optional<std::string> loadVoyages(
		std::unordered_map <std::string, voyage_t> &voyages,
		std::unordered_map <uint32_t, std::string> &voyageIdToNameMap,
		const std::unordered_map<std::string, std::unordered_map<std::string, fish_t>> &fishes,
		const std::unordered_map<std::string, std::unordered_set<uint32_t>>& achievements,
		const json& j
	)
	{
		if (isBadKey(j, "voyages")) return "Missing voyages in database.\nJson Dump:\n" + j.dump(4);

		for (const auto& voyage : j["voyages"].get<json::object_t>())
		{
			std::string voyageName = voyage.first;
			if (voyages.contains(voyageName)) return "Error: duplicate voyage name in json: " + voyageName;
			if (isBadKey(voyage.second, "id")) return "Missing id for voyage.\nJson Dump:\n" + voyage.second.dump(4);
			if (!voyage.second["id"].is_number_unsigned()) return "Error: invalid voyage ID in json: " + voyage.second["id"].dump(4);
			const uint32_t id = voyage.second["id"].get<uint32_t>();
			if (voyageIdToNameMap.contains(id)) return "Error: duplidcate voyage id in json: " + id;
			if (isBadKey(voyage.second, "shortform")) return "Missing shortform name for voyage.\nJson Dump:\n" + voyage.second.dump(4);

			std::vector<stop_t> voyageStops;
			auto err = loadVoyageStops(voyageStops, fishes, voyage.second);
			if (err) return err;

			voyages.insert({ voyageName,
				{
					voyage.second["shortform"].get<std::string>(),
					id,
					voyageStops,
					getAchievementsForVoyage(id, achievements),
					"" // bluefishpattern, generated later
				}
				});
			voyageIdToNameMap.insert({ id, voyageName });
		}
		return std::nullopt;
	}

	/**
		@brief gets all the blue fish at each stop

		@param[in] voyageStops vector of stops for a voyage
		@param[in] blueFishNames container of all the blue fish

		@return vector with either a blue fish name or std::nullopt if no blue fish at that stop
	**/
	std::vector<std::optional<std::string>> getBlueFishAtStops(
		const std::vector<stop_t> & voyageStops,
		const std::map<std::string, fish_t>& blueFishNames)
	{
		// TODO make this be able to handle more than one blue fish per stop
		std::vector<std::optional<std::string>> blueFishAtStop;
		for (const auto& stop : voyageStops)
		{
			bool blueFishFound = false;
			for (const auto& fish : stop.fish) // go through all the possible fishes at this stop
			{
				if (blueFishNames.contains(fish)) // we only care about the blue fish
				{
					blueFishFound = true;
					blueFishAtStop.push_back(fish);
					break;
				}
			}
			if (!blueFishFound)
				blueFishAtStop.push_back(std::nullopt);
		}
		return blueFishAtStop;
	}


	/**
		@brief combines a vector of strings with delimiter in between

		@param[in] stringList vector of strings to combine
		@param[in] delim delimiter to insert between the strings

		@return combined string
	**/
	std::string implodeStringVector(std::vector<std::string> stringList, const char* const delim = "-")
	{
		std::ostringstream ss;
		std::copy(stringList.begin(), stringList.end(),
			std::ostream_iterator<std::string>(ss, delim));
		std::string implodedString = ss.str();
		implodedString.pop_back(); // remove the last dash
		return implodedString;
	}

	/**
		@brief create targets for blue fish

		@param[out] targetToVoyageIdMap map of target -> voyage id
		@param[in] voyages the container of all the voyages
		@param[in] blueFishNames the container of all the blue fish
	**/
    void setBlueFishTargets(
		std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
		std::unordered_map <std::string, voyage_t>& voyages,
		const std::map<std::string, fish_t>& blueFishNames
	)
	{
		// targets by blue fish per voyage
		targetToVoyageIdMap.insert({ "Blue Fish Pattern", {} });
		for (const auto& voyage : voyages)
		{
			auto blueFishAtStops = getBlueFishAtStops(voyage.second.stops, blueFishNames);
			std::unordered_set<std::string> blueFish;
			std::vector<std::string> names;
			// create pattern string as fish1-fish2-fish3, and use X if there is no blue fish
			for (const auto& fish : blueFishAtStops)
			{
				if (fish)
				{
					blueFish.insert(fish.value());
					names.push_back(blueFishNames.at(fish.value()).shortName);
				}
				else
					names.push_back("X");
			}
			std::string blueFishPattern = implodeStringVector(names);

			// if only 1 blue fish, use that as the image name without the Xs
			std::string imageName = blueFishPattern;
			if (blueFish.size() == 1)
				imageName = *blueFish.begin();

			if (blueFishPattern != "X-X-X")
			{
				targetToVoyageIdMap.at("Blue Fish Pattern").insert({ blueFishPattern,
					{
						blueFishPattern, // label name
						imageName, // image name
						{} // voyage ids
					}
					});
				targetToVoyageIdMap.at("Blue Fish Pattern").at(blueFishPattern).ids.insert(voyage.second.id);
				voyages.at(voyage.first).blueFishPattern = blueFishPattern;
			}
		}

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
		targetToVoyageIdMap.insert({ "Achievement", {} });

		for (const auto& achievement : achievements)
		{
			std::unordered_set <uint32_t> ids;
			for (const auto& voyageId : achievement.second)
			{
				ids.insert(voyageId);
			}
			targetToVoyageIdMap.at("Achievement").insert({ achievement.first,
				{
					achievement.first, // achievement label and imagename are the same as just the acheivement name
					achievement.first,
					ids
				}
				});
		}
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
		for (const auto& fishType : fishes)
		{
			targetToVoyageIdMap.insert({ fishType.first, {} });
			for (const auto& fish : fishType.second)
			{
				const std::string fishName = fish.first;
				std::unordered_set <uint32_t> ids;
				for (const auto& voyage : voyages)
				{
					for (const auto& stop : voyage.second.stops)
					{
						if (stop.fish.contains(fishName))
						{
							ids.insert(voyage.second.id);
						}
					}
				}
				targetToVoyageIdMap.at(fishType.first).insert({ fishName,
					{
						fish.first, // fish label and imagename are the same as just the fish name
						fish.first,
						ids
					}
					});
			}
		}
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
		targetToVoyageIdMap.insert({ "Voyages", {} });
		for (const auto& voyage : voyages)
		{
			std::string lastStop = voyage.second.stops.back().location.name;
			if (stops.contains(lastStop))
			{
				std::string lastStopShortName = stops.at(lastStop);

				if (!targetToVoyageIdMap.contains(lastStopShortName))
					targetToVoyageIdMap.at("Voyages").insert({ lastStopShortName, {"", "", {}} });
				targetToVoyageIdMap.at("Voyages").at(lastStopShortName).ids.insert(voyage.second.id);
			}

			targetToVoyageIdMap.at("Voyages").insert({ voyage.first, {"", "", {voyage.second.id}} });
		}
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

