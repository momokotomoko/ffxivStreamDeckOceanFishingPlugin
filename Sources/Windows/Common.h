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
};

struct fish_t
{
	const std::string shortName;
	const std::unordered_map<std::string, locations_t> locations;
};

struct stop_t
{
	const locations_t location;
	const std::unordered_set<std::string> fish;
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

