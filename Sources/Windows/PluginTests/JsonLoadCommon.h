//copyright  (c) 2023, Momoko Tomoko

#pragma once
#include "pch.h"
#include "../Common.h"

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

    struct achievementData_t
    {
        json j;
        std::optional<std::pair<std::string, std::unordered_set<uint32_t>>> expectedAchievement;
    };

    struct fishData_t
    {
        std::string fishType;
        std::string name;
        std::optional<std::string> shortForm;
        std::optional<std::vector<locationData_t>> locations;
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

    const fishData_t fishA = { "Blue Fish", "Afish", "A", std::vector<locationData_t>{locationA} };
    const fishData_t fishB = { "Blue Fish", "Bfish", std::nullopt, std::vector<locationData_t>{locationA, locationB} };
    const fishData_t fishC = { "Green Fish", "Cfish", "C", std::vector<locationData_t>{locationA, locationC} };
    const fishData_t fishD = { "Green Fish", "Dfish", std::nullopt, std::vector<locationData_t>{locationD} };
    const fishData_t fishNoLocation = { "Blue Fish", "NoLocFish", std::nullopt, { } };
    const fishData_t fishInvalidLocation = { "Green Fish", "InvalidLocationFish", std::nullopt, std::vector<locationData_t>{locationInvalidName} };
    const fishData_t fishLocationMissingName = { "Green Fish", "LocationMissingNameFish", std::nullopt, std::vector<locationData_t>{locationNoName} };

    // TODO: make fish_t the same as fishData_t
    std::pair<std::string, fish_t> createFish(const fishData_t& data);
}