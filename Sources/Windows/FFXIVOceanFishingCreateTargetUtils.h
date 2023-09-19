//==============================================================================
/**
@file       FFXIVOceanFishingCreateTargetUtils.h
@brief      Functions that helps with creating the tracker targets.
@copyright  (c) 2023, Momoko Tomoko
**/
//==============================================================================

#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include "Common.h"

namespace createTargetUtils
{
    template <typename T>
    void targetInserter(std::map<std::string, targets_t>& map, T& newTargets);
    void setBlueFishTargets(
        std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
        const std::unordered_map <std::string, voyage_t>& voyages,
        const std::set<std::string>& blueFishNames
    );
    void setAchievementTargets(
        std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
        const std::unordered_map<std::string, std::unordered_set<uint32_t>>& achievements
    );
    void setFishTargets(
        std::unordered_map <std::string, std::map<std::string, targets_t>>& targetToVoyageIdMap,
        const std::unordered_map <std::string, voyage_t>& voyages,
        const std::unordered_map<std::string, fish_t>& fishes
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