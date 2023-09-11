//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"
#include "JsonLoadCommon.h"
#include "../FFXIVOceanFishingJsonLoadUtils.h"


namespace LoadJsonTests
{
    namespace LoadVoyageSchedule
    {
        struct loadVoyageScheduleTestInputs_t
        {
            json j;
            std::vector<uint32_t> expectedPattern;
            uint32_t expectedOffset;
        };

        class LoadVoyageScheduleTestFixture :
            public ::testing::TestWithParam<::testing::tuple<loadVoyageScheduleTestInputs_t, bool>> {
        };

        INSTANTIATE_TEST_CASE_P(
            InvalidSchedule,
            LoadVoyageScheduleTestFixture,
            ::testing::Combine(
                ::testing::Values(
                    loadVoyageScheduleTestInputs_t{ json::parse(R"({"schedule": {"pattern": [1], "offset": -4}})") },
                    loadVoyageScheduleTestInputs_t{ json::parse(R"({"schedule": {"pattern": [1], "offset": "string offset"}})") },
                    loadVoyageScheduleTestInputs_t{ json::parse(R"({"schedule": {"pattern": [1]}})") },
                    loadVoyageScheduleTestInputs_t{ json::parse(R"({"schedule": {"pattern": {}, "offset": 4}})") },
                    loadVoyageScheduleTestInputs_t{ json::parse(R"({"schedule": {"pattern": [1,"a"], "offset": 4}})") },
                    loadVoyageScheduleTestInputs_t{ json::parse(R"({"schedule": {"pattern": [-1], "offset": 4}})") },
                    loadVoyageScheduleTestInputs_t{ json::parse(R"({"schedule": {"offset": 4}})") }
                ),
                ::testing::Values(HAS_ERROR)
            )
        );

        INSTANTIATE_TEST_CASE_P(
            NoError,
            LoadVoyageScheduleTestFixture,
            ::testing::Combine(
                ::testing::Values(
                    loadVoyageScheduleTestInputs_t{ json::parse(R"({"schedule": {"pattern": [1,2,3,1], "offset": 4}})"), std::vector<uint32_t>{1, 2, 3, 1}, 4 },
                    loadVoyageScheduleTestInputs_t{ json::parse(R"({"schedule": {"pattern": [1], "offset": 0}})"), std::vector<uint32_t>{1}, 0 }
                ),
                ::testing::Values(NO_ERROR)
            )
        );

        TEST_P(LoadVoyageScheduleTestFixture, TestLoadRouteName) {
            const auto& [testInput, expectedError] = GetParam();
            const auto& [j, expectedPattern, expectedOffset] = testInput;

            std::vector<uint32_t> pattern;
            uint32_t offset;
            EXPECT_EQ(isError(jsonLoadUtils::loadVoyageSchedule(pattern, offset, j)), expectedError);

            if (!expectedError)
            {
                EXPECT_EQ(pattern, expectedPattern);
                EXPECT_EQ(offset, expectedOffset);
            }
        }
    }
}