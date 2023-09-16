//==============================================================================
/**
@file       Common.h
@brief      Common objects
@copyright  (c) 2020, Momoko Tomoko
**/
//==============================================================================
#pragma once
#include "pch.h"

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <optional>

// priority to show achievement or blue fish
enum class PRIORITY
{
    ACHIEVEMENTS,
    BLUE_FISH,
};

struct locations_t
{
    const std::string name;
    const std::unordered_set<std::string> time;

    bool operator==(const locations_t& location) const { return name == location.name && time == location.time; }
};

struct fish_t
{
    const std::string name;
    const std::string type;
    const std::optional<std::string> shortName;
    const std::unordered_map<std::string, locations_t> locations;

    bool operator==(const fish_t& fish) const { return name == fish.name && shortName == fish.shortName && locations == fish.locations; }
};

struct stop_t
{
    const locations_t location;
    const std::unordered_set<std::string> fish;

    bool operator==(const stop_t& stop) const { return location == stop.location && fish == stop.fish; }
};

struct voyage_t
{
    const std::string shortName;
    const uint32_t id;
    const std::vector<stop_t> stops;
    const std::unordered_set<std::string> achievements;
    std::string blueFishPattern;
};

struct targets_t
{
    const std::string labelName;
    const std::string imageName;
    std::unordered_set <uint32_t> ids;
};

