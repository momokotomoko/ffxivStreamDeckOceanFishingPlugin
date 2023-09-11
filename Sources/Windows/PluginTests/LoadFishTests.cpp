//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"
#include "JsonLoadCommon.h"
#include "../FFXIVOceanFishingJsonLoadUtils.h"


namespace LoadJsonTests
{
    namespace LoadFish
    {
        json createFishJson(const fishData_t& data)
        {
            json jFish = nlohmann::json::value_t::object;
            if (data.shortForm) jFish["shortform"] = data.shortForm.value();
            if (data.locations)
            {
                jFish["locations"] = json::value_t::array;
                for (const auto& [locationJson, _] : data.locations.value())
                    jFish["locations"].push_back(locationJson);
            }

            json j;
            j[data.fishType][data.name] = jFish;
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
                ::testing::Values(fishA, fishB, fishC, fishD, fishNoLocation),
                ::testing::Values(fishA, fishB, fishC, fishD, fishNoLocation),
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
                ::testing::Values(fishA, fishC, fishNoLocation),
                ::testing::Values(fishA, fishC, fishNoLocation),
                ::testing::Values(HAS_ERROR)
            )
        );

        INSTANTIATE_TEST_CASE_P(
            InvalidFish,
            LoadFishTestFixture,
            ::testing::Combine(
                ::testing::Values(std::vector<std::string>{ "targets", "fish" }),
                ::testing::Values(fishLocationMissingName, fishInvalidLocation),
                ::testing::Values(fishNoLocation),
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
            std::map<std::string, fish_t> expectedBlueFish;
            auto fish0 = createFish(input0);
            auto fish1 = createFish(input1);
            expectedFish.insert({ input0.fishType, { } });
            expectedFish.insert({ input1.fishType, { } });
            expectedFish.at(input0.fishType).insert(fish0);
            expectedFish.at(input1.fishType).insert(fish1);
            if (input0.fishType == "Blue Fish") expectedBlueFish.insert(fish0);
            if (input1.fishType == "Blue Fish") expectedBlueFish.insert(fish1);

            std::unordered_map<std::string, std::unordered_map<std::string, fish_t>> fish;
            std::map<std::string, fish_t> blueFish;
            EXPECT_EQ(isError(jsonLoadUtils::loadFish(fish, blueFish, j)), expectedError);

            if (!expectedError)
            {
                EXPECT_EQ(fish, expectedFish);
                EXPECT_EQ(blueFish, expectedBlueFish);
            }
        }
    }
}