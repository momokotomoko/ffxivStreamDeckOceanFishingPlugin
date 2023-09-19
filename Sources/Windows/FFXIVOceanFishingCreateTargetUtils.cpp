//==============================================================================
/**
@file       FFXIVOceanFishingCreateTargetUtils.cpp
@brief      Functions that helps with creating the tracker targets.
@copyright  (c) 2023, Momoko Tomoko
**/
//==============================================================================

#pragma once
#include "pch.h"
#include "FFXIVOceanFishingCreateTargetUtils.h"
#include <string>
#include <map>
#include <algorithm>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include "Common.h"

namespace createTargetUtils
{
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
                    map.emplace(newItem);
            }
        );
    }

    /**
        @brief create targets for blue fish

        @param[out] targetToVoyageIdMap map of target -> voyage id
        @param[in] voyages the container of all the voyages
        @param[in] blueFishNames all the blue fish names
    **/
    void setBlueFishTargets(
        std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
        const std::unordered_map <std::string, voyage_t>& voyages,
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
            | std::views::transform([&](const auto& voyage) -> std::vector<std::string>
                {
                    return voyage.second.stops
                        | std::views::transform(blueFishAtStop)
                        | std::views::join
                        | std::ranges::to<std::vector<std::string>>();
                });

        // function to make targets
        auto makeTargets = [](const auto& zipped) -> std::pair<std::string, targets_t>
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
                [](const auto& zipped) -> bool
                {
                    const auto& [blueFish, _] = zipped;
                    return !(blueFish.empty());
                })
            | std::views::transform(makeTargets);

                targetInserter(targetToVoyageIdMap["Blue Fish Pattern"], newTargets);
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
        targetInserter(targetToVoyageIdMap["Achievement"], newTargets);
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
        const std::unordered_map<std::string, fish_t>& fishes
    )
    {
        // function for checking if a voyage contains a fish with specific name at one of its stops
        auto doesVoyageContainFish = [](const auto voyage, const std::string& fishName) -> bool
            {
                // TODO: would converting to set be faster?
                auto fishesInVoyage = voyage.second.stops
                    | std::views::transform([&](const auto& stop) { return stop.fish; })
                    | std::views::join; // get all the fishes from all the stops
                return std::ranges::contains(fishesInVoyage, fishName);
            };

        auto makeFishTargets = [&](const auto& kv_pair) -> std::pair<std::string, std::pair<std::string, targets_t>>
            {
                const auto& [fishName, fishData] = kv_pair;
                std::unordered_set <uint32_t> ids = voyages
                    | std::views::filter([&](const auto& voyage) { return  doesVoyageContainFish(voyage, fishName); })
                    | std::views::transform([&](const auto& voyage) { return voyage.second.id; })
                    | std::ranges::to<std::unordered_set <uint32_t >>();
                // fish label and imagename are the same as just the fish name
                return std::make_pair(fishData.type, std::make_pair(fishName, targets_t{ fishName, fishName, ids }));
            };

        std::ranges::for_each(fishes | std::views::transform(makeFishTargets),
            [&](const auto& kv_pair)
            {
                const auto& [type, target] = kv_pair;
                targetToVoyageIdMap[type].emplace(target);
            });
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
        // TODO merge makeVoyageTarget with makeVoyageLastnameTarget when std::views:concat is in c++23
        auto makeVoyageTarget = [&](const auto& voyage) -> std::pair<std::string, targets_t>
            {
                const auto& [voyageName, voyageData] = voyage;
                return std::make_pair(voyageName, targets_t{ "", "", { voyageData.id } });
            };

        targetToVoyageIdMap["Voyages"] = voyages
            | std::views::transform(makeVoyageTarget)
            | std::ranges::to<std::map<std::string, targets_t>>();

        // range of shortform names of the last stop
        auto voyageLastStopShortName = voyages
            | std::views::transform([&](const auto& voyage) { return stops.at(voyage.second.stops.back().location.name); });

        // function to make a target given zipped (voyage_t and voyage last stop name)
        auto makeVoyageLastnameTarget = [&](const auto& zipped) -> std::pair<std::string, targets_t>
            {
                const auto& [voyage, lastStopShortName] = zipped;
                const auto& [voyageName, voyageData] = voyage;
                return std::make_pair(lastStopShortName, targets_t{ "", "", { voyageData.id } });
            };

        // create the targets and save them
        auto newTargets = std::views::zip(voyages, voyageLastStopShortName)
            | std::views::transform(makeVoyageLastnameTarget);
        targetInserter(targetToVoyageIdMap["Voyages"], newTargets);
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
        for (const auto& [_, voyageData] : voyages)
            targetToVoyageIdMap["Other"]["Next Voyage"].ids.insert(voyageData.id);
    }
}