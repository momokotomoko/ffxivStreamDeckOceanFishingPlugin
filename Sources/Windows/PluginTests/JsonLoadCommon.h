//copyright  (c) 2023, Momoko Tomoko

#pragma once
#include "pch.h"
#include "../Common.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <optional>

namespace LoadJsonTests
{
    struct jkeys_t
    {
        std::vector <std::string> keys;
    };

    struct locationData_t
    {
        json j;
        std::optional<std::pair<std::string, locations_t>> expectedLocation;
    };

    struct fishData_t
    {
        fish_t fish;
        std::optional<std::vector<locationData_t>> locations;
    };

    struct achievementData_t
    {
        json j;
        std::optional<std::pair<std::string, std::unordered_set<uint32_t>>> expectedAchievement;
    };

    void wrapKeys(json& jTail, const std::vector <std::string>& nestedKeys);
    bool isError(const std::optional<std::string>& errMsg);

    const std::pair<std::string, locations_t> expectedLocationA = { "locA", { "locA", {"day"} } };
    const std::pair<std::string, locations_t> expectedLocationB = { "locB", { "locB", {"day", "night"}} };
    const std::pair<std::string, locations_t> expectedLocationC = { "locC", { "locC", {} } };
    const std::pair<std::string, locations_t> expectedLocationD = { "locD", { "locD", {} } };
    const locationData_t locationA = { json::parse(R"({ "name": "locA", "time": "day" })"), expectedLocationA, };
    const locationData_t locationB = { json::parse(R"({ "name": "locB", "time": ["day", "night"] })"), expectedLocationB };
    const locationData_t locationC = { json::parse(R"({ "name": "locC", "time": [] })"), expectedLocationC };
    const locationData_t locationD = { json::parse(R"({ "name": "locD"})"), expectedLocationD };
    const locationData_t locationNull = { json::parse(R"({ })"), std::nullopt };
    const locationData_t locationInvalidName = { json::parse(R"({ "name": 1, "time": "day" })"), std::nullopt };
    const locationData_t locationNoName = { json::parse(R"({ "time": "day" })"), std::nullopt };

    const auto expectedAchievementA = std::make_pair< std::string, std::unordered_set <uint32_t>>("achA", { 1 });
    const auto expectedAchievementB = std::make_pair< std::string, std::unordered_set <uint32_t>>("achB", { 1, 2 });
    const auto expectedAchievementC = std::make_pair< std::string, std::unordered_set <uint32_t>>("achC", { });
    const achievementData_t achieveA = { json::parse(R"({ "achA": { "voyageIds" : [1] } })"), expectedAchievementA };
    const achievementData_t achieveB = { json::parse(R"({ "achB": { "voyageIds" : [1, 2] } })"), expectedAchievementB };
    const achievementData_t achieveC = { json::parse(R"({ "achC": { "voyageIds" : [] } })"), expectedAchievementC };
    const achievementData_t achieveNull = { json::parse(R"({ })"), std::nullopt };
    const achievementData_t achieveInvalid = { json::parse(R"({ "achC": { "badkey" : [] } })"), expectedAchievementC };

    const fish_t expectedFishA = { "Afish", "Blue Fish", "A", {expectedLocationA} };
    const fish_t expectedFishB = { "Bfish", "Blue Fish",  std::nullopt, {expectedLocationA, expectedLocationB} };
    const fish_t expectedFishC = { "Cfish", "Green Fish", "C", {expectedLocationA, expectedLocationC} };
    const fish_t expectedFishD = { "Dfish", "Green Fish", std::nullopt, {expectedLocationD} };
    const fish_t expectedFishNoLocation = { "NoLocFish", "Blue Fish", std::nullopt, { } };


    const fishData_t fishDataA = { expectedFishA, std::vector<locationData_t>{locationA} };
    const fishData_t fishDataB = { expectedFishB, std::vector<locationData_t>{locationA, locationB} };
    const fishData_t fishDataC = { expectedFishC, std::vector<locationData_t>{locationA, locationC} };
    const fishData_t fishDataD = { expectedFishD, std::vector<locationData_t>{locationD} };
    const fishData_t fishDataNullLocation = { expectedFishNoLocation, std::nullopt };
    const fishData_t fishDataEmptyLocation = { expectedFishNoLocation, std::vector<locationData_t>{} }; // todo add locationNull?
    const fishData_t fishDataInvalidLocation = { expectedFishNoLocation, std::vector<locationData_t>{locationInvalidName} };
    const fishData_t fishDataNoName = { expectedFishNoLocation, std::vector<locationData_t>{locationNoName} };

    const fish_t fishInvalidLocation = { "InvalidLocationFish", "Green Fish", std::nullopt, {} };
    const fish_t fishLocationMissingName = { "LocationMissingNameFish", "Green Fish", std::nullopt, {} };
}