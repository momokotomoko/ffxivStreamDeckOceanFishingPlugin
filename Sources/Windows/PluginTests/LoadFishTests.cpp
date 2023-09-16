//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"
#include "JsonLoadCommon.h"
#include "../FFXIVOceanFishingJsonLoadUtils.h"


namespace LoadJsonTests
{
    namespace LoadFish
    {
        json createFishJson(fishData_t fishData)
        {
            const auto& [fish, locationData] = fishData;
            json jFish = nlohmann::json::value_t::object;
            if (fish.shortName) jFish["shortform"] = fish.shortName.value();

            if (locationData)
            {
                jFish["locations"] = json::value_t::array;
                for (const auto& [locationJson, _] : locationData.value())
                    jFish["locations"].push_back(locationJson);
            }

            json j;
            j[fish.type][fish.name] = jFish;
            return j;
        }

        class LoadFishTestFixture :
            public ::testing::TestWithParam<
            ::testing::tuple<std::vector<std::string>, fishData_t, fishData_t, bool>> {};

        INSTANTIATE_TEST_CASE_P(
            NoError,
            LoadFishTestFixture,
            ::testing::Combine(
                ::testing::Values(std::vector<std::string>{ "targets", "fish" }),
                ::testing::Values(fishDataA, fishDataB, fishDataC, fishDataD, fishDataNullLocation, fishDataEmptyLocation),
                ::testing::Values(fishDataA, fishDataB, fishDataC, fishDataD, fishDataNullLocation, fishDataEmptyLocation),
                ::testing::Values(NO_ERROR)
            )
        );

        INSTANTIATE_TEST_CASE_P(
            InvalidKeys,
            LoadFishTestFixture,
            ::testing::Combine(
                ::testing::Values(
                    std::vector<std::string>{ "badtargetskey", "fish" },
                    std::vector<std::string>{ "targets", "badfishkey" }),
                ::testing::Values(fishDataA, fishDataC, fishDataNullLocation),
                ::testing::Values(fishDataA, fishDataC, fishDataNullLocation),
                ::testing::Values(HAS_ERROR)
            )
        );

        INSTANTIATE_TEST_CASE_P(
            InvalidFish,
            LoadFishTestFixture,
            ::testing::Combine(
                ::testing::Values(std::vector<std::string>{ "targets", "fish" }),
                ::testing::Values(fishDataNoName, fishDataInvalidLocation),
                ::testing::Values(fishDataNullLocation),
                ::testing::Values(HAS_ERROR)
            )
        );

        TEST_P(LoadFishTestFixture, TestLoadFish) {
            const auto& [keys, input0, input1, expectedError] = GetParam();

            json j;
            j.merge_patch(createFishJson(input0));
            j.merge_patch(createFishJson(input1));
            wrapKeys(j, keys);

            std::unordered_map<std::string, std::unordered_map<std::string, fish_t>> expectedFish;
            std::set<std::string> expectedBlueFish;

            expectedFish.insert({ input0.fish.type, { } });
            expectedFish.insert({ input1.fish.type, { } });
            const auto fish0 = std::make_pair(input0.fish.name, input0.fish);
            const auto fish1 = std::make_pair(input1.fish.name, input1.fish);
            expectedFish.at(input0.fish.type).insert(fish0);
            expectedFish.at(input1.fish.type).insert(fish1);
            if (input0.fish.type == "Blue Fish") expectedBlueFish.insert(input0.fish.name);
            if (input1.fish.type == "Blue Fish") expectedBlueFish.insert(input1.fish.name);

            std::unordered_map<std::string, std::unordered_map<std::string, fish_t>> fish;
            std::set<std::string> blueFish;
            EXPECT_EQ(isError(jsonLoadUtils::loadFish(fish, blueFish, j)), expectedError);

            if (!expectedError)
            {
                EXPECT_EQ(fish, expectedFish);
                EXPECT_EQ(blueFish, expectedBlueFish);
            }
        }
    }
}