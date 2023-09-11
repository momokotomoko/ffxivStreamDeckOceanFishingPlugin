//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"
#include "JsonLoadCommon.h"
#include "../FFXIVOceanFishingJsonLoadUtils.h"


namespace LoadJsonTests
{
    namespace LoadStops
    {

        struct loadStopsTestInputs_t
        {
            json j;
            std::optional<std::pair<std::string, std::string>> expectedStops;
        };

        class LoadStopsTestFixture :
            public ::testing::TestWithParam<
            ::testing::tuple<std::vector<std::string>, loadStopsTestInputs_t, loadStopsTestInputs_t, bool>> {};

        auto loadStopsNoErrorValues = ::testing::Values(
            loadStopsTestInputs_t{ json::parse(R"({"StopA": {"shortform": "A"}})"), std::make_pair<std::string, std::string>("StopA", "A") },
            loadStopsTestInputs_t{ json::parse(R"({"StopB": {"shortform": 1}})"), std::make_pair<std::string, std::string>("StopB", "StopB") },
            loadStopsTestInputs_t{ json::parse(R"({"StopC": {}})"), std::make_pair<std::string, std::string>("StopC", "StopC") },
            loadStopsTestInputs_t{ json::parse(R"({})"), std::nullopt }
        );

        INSTANTIATE_TEST_CASE_P(
            LoadStopsNoError,
            LoadStopsTestFixture,
            ::testing::Combine(
                ::testing::Values(std::vector<std::string>{ "stops" }),
                loadStopsNoErrorValues,
                loadStopsNoErrorValues,
                ::testing::Values(NO_ERROR)
            )
        );

        INSTANTIATE_TEST_CASE_P(
            LoadStopsHasError,
            LoadStopsTestFixture,
            ::testing::Combine(
                ::testing::Values(std::vector<std::string>{ "badstopskey" }),
                ::testing::Values(loadStopsTestInputs_t{ json::parse(R"({})"), std::nullopt }),
                ::testing::Values(loadStopsTestInputs_t{ json::parse(R"({})"), std::nullopt }),
                ::testing::Values(HAS_ERROR)
            )
        );

        TEST_P(LoadStopsTestFixture, TestLoadStops) {
            const auto& [keys, input0, input1, expectedError] = GetParam();

            json j;
            j.update(input0.j);
            j.update(input1.j);
            wrapKeys(j, keys);

            std::unordered_map<std::string, std::string> expectedStops;
            if (input0.expectedStops) expectedStops.insert(input0.expectedStops.value());
            if (input1.expectedStops) expectedStops.insert(input1.expectedStops.value());

            std::unordered_map<std::string, std::string> stops;
            EXPECT_EQ(isError(jsonLoadUtils::loadStops(stops, j)), expectedError);
            EXPECT_EQ(stops, expectedStops);
        }
    }
}