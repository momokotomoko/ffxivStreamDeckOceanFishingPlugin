//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"
#include "JsonLoadCommon.h"
#include "../FFXIVOceanFishingJsonLoadUtils.h"


namespace LoadJsonTests
{
    namespace LoadVoyageHelpers
    {
        class LoadVoyageHelpersBase
        {
        public:
            std::unordered_map<std::string, fish_t> fishes = {
                {expectedFishA.name, expectedFishA},
                {expectedFishB.name, expectedFishB},
                {expectedFishC.name, expectedFishC},
                {expectedFishD.name, expectedFishD},
                {expectedFishNoLocation.name, expectedFishNoLocation}
            };
        };

        class GetFishAtStopTestFixture :
            public LoadVoyageHelpersBase,
            public ::testing::TestWithParam<
            ::testing::tuple<std::string, std::string, std::unordered_set<std::string>>>
        {
        public:

        };

        INSTANTIATE_TEST_CASE_P(
            GetFishAtStopTests,
            GetFishAtStopTestFixture,
            ::testing::Values(
                std::make_tuple("", "", std::unordered_set<std::string>{}),
                std::make_tuple("locA", "day", std::unordered_set<std::string>{"Afish", "Bfish", "Cfish"}),
                std::make_tuple("locC", "sunset", std::unordered_set<std::string>{"Cfish"}),
                std::make_tuple("locB", "day", std::unordered_set<std::string>{"Bfish"}),
                std::make_tuple("locB", "sunset", std::unordered_set<std::string>{}),
                std::make_tuple("invalidLoc", "day", std::unordered_set<std::string>{})
            )
        );

        TEST_P(GetFishAtStopTestFixture, TestLoadFish) {
            const auto& [stopName, stopTime, expectedResult] = GetParam();
            EXPECT_EQ(jsonLoadUtils::getFishAtStop(stopName, stopTime, fishes), expectedResult);
        }



        class LoadVoyageStopsTestFixture :
            public LoadVoyageHelpersBase,
            public ::testing::TestWithParam<::testing::tuple<jstruct_t, std::optional<std::vector<stop_t>>, bool>> {};

        INSTANTIATE_TEST_CASE_P(
            LoadVoyageStops,
            LoadVoyageStopsTestFixture,
            ::testing::Values(
                std::make_tuple(jstruct_t(R"({"stops": [ {  "name": "locA", "time": "day" }, { "name": "locB", "time": "night" }, { "name": "locC", "time": "sunset" } ]})"),
                    std::vector<stop_t>{ {{"locA", { "day" }}, { expectedFishA.name, expectedFishB.name, expectedFishC.name }}, { {"locB", {"night"}}, {expectedFishB.name} }, { { "locC", {"sunset"} }, { expectedFishC.name } } }, NO_ERROR),
                std::make_tuple(jstruct_t(R"({"stops": [ {  "badnamekey": "locA", "time": "day" }]})"),
                    std::nullopt, HAS_ERROR),
                std::make_tuple(jstruct_t(R"({"stops": [ {  "name": "locA", "badtimekey": "day" }]})"),
                    std::nullopt, HAS_ERROR)
            )
        );

        TEST_P(LoadVoyageStopsTestFixture, TestLoadVoyageStops) {
            const auto& [jStruct, expectedResult, expectedError] = GetParam();

            std::vector<stop_t> voyageStops;
            EXPECT_EQ(isError(jsonLoadUtils::loadVoyageStops(voyageStops, fishes, jStruct.j)), expectedError);

            if (!expectedError)
                EXPECT_EQ(voyageStops, expectedResult.value());
        }

        class GetAchievementsForVoyageTestFixture :
            public ::testing::TestWithParam<
            ::testing::tuple<uint32_t, std::unordered_set<std::string>>> {
        protected:
            const std::unordered_map<std::string, std::unordered_set<uint32_t>> achievements{ expectedAchievementA, expectedAchievementB, expectedAchievementC };
        };

        INSTANTIATE_TEST_CASE_P(
            GetAchievementsForVoyageTests,
            GetAchievementsForVoyageTestFixture,
            ::testing::Values(
                std::make_tuple(1, std::unordered_set<std::string>{expectedAchievementA.first, expectedAchievementB.first}),
                std::make_tuple(2, std::unordered_set<std::string>{expectedAchievementB.first}),
                std::make_tuple(3, std::unordered_set<std::string>{})
            )
        );

        TEST_P(GetAchievementsForVoyageTestFixture, TestGetAchievements) {
            EXPECT_EQ(jsonLoadUtils::getAchievementsForVoyage(std::get<0>(GetParam()), achievements), std::get<1>(GetParam()));
        }

        
        class GetBlueFishAtStopsTestFixture :
            public LoadVoyageHelpersBase,
            public ::testing::TestWithParam<
            ::testing::tuple<std::vector<stop_t>, std::vector<std::unordered_set<std::string>>>> {};

        INSTANTIATE_TEST_CASE_P(
            GetBlueFishAtStopsTest,
            GetBlueFishAtStopsTestFixture,
            ::testing::Values(
                std::make_tuple(std::vector<stop_t>{ {{"locA", { "day" }}, { expectedFishA.name, expectedFishB.name, expectedFishC.name }}, { {"locC", {"sunset"}}, {expectedFishC.name} }, { { "locB", {"sunset"} }, { } } },
                    std::vector<std::unordered_set<std::string>>{{ "Afish", "Bfish" }, {}, {}}),
                std::make_tuple(std::vector<stop_t>{ {{"locA", { "day" }}, { }}, { {"locC", {"sunset"}}, {expectedFishC.name} }, { { "locB", {"sunset"} }, { } } },
                    std::vector<std::unordered_set<std::string>>{{}, { }, {}}),
                std::make_tuple(std::vector<stop_t>{ {{"locA", { "day" }}, { }}, { {"locC", {"sunset"}}, {expectedFishC.name} } },
                    std::vector<std::unordered_set<std::string>>{{}, {}}),
                std::make_tuple(std::vector<stop_t>{},
                    std::vector<std::unordered_set<std::string>>{})
            )
        );

        TEST_P(GetBlueFishAtStopsTestFixture, TestGetBlueFish) {
            const auto& [stops, expectedResult] = GetParam();
            std::set<std::string> blueFishNames;
            for (const auto& [fishName, fish] : fishes)
                if (fish.type == "Blue Fish")
                    blueFishNames.insert(fishName);
            EXPECT_EQ(jsonLoadUtils::getBlueFishAtStops(stops, blueFishNames), expectedResult);
        }

        class CreateBlueFishPatternTestFixture :
            public LoadVoyageHelpersBase,
            public ::testing::TestWithParam<::testing::tuple<std::vector<std::unordered_set<std::string>>, std::string>> {};

        INSTANTIATE_TEST_CASE_P(
            CreateBlueFishPatternTest,
            CreateBlueFishPatternTestFixture,
            ::testing::Values(
                std::make_tuple(std::vector < std::unordered_set<std::string>>{{"Afish"}, { "Bfish" }, { "NoLocFish" }}, "A-Bfish-NoLocFish"),
                std::make_tuple(std::vector < std::unordered_set<std::string>>{{"Afish"}, { "Bfish" }}, "A-Bfish"),
                std::make_tuple(std::vector < std::unordered_set<std::string>>{{"Afish"}}, "A"),
                std::make_tuple(std::vector < std::unordered_set<std::string>>{{"Bfish"}}, "Bfish"),
                std::make_tuple(std::vector < std::unordered_set<std::string>>{{"Afish"}, { "Bfish", "Cfish" }}, "A-Bfish"),
                std::make_tuple(std::vector < std::unordered_set<std::string>>{{"FishNotInDatabase"}}, "FishNotInDatabase")
            )
        );

        TEST_P(CreateBlueFishPatternTestFixture, TestCreateblueFishPattern) {
            const auto& [blueFishPerStop, expectedResult] = GetParam();
            EXPECT_EQ(jsonLoadUtils::createBlueFishPattern(blueFishPerStop, fishes), expectedResult);
        }
    }
}