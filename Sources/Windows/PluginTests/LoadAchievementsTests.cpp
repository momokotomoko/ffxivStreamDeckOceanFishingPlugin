//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"
#include "JsonLoadCommon.h"
#include "../FFXIVOceanFishingJsonLoadUtils.h"


namespace LoadJsonTests
{
    namespace LoadAchievements
    {
        class LoadAchievementsTestFixture :
            public ::testing::TestWithParam<
            ::testing::tuple<std::vector<std::string>, achievementData_t, achievementData_t, bool>> {};

        INSTANTIATE_TEST_CASE_P(
            NoError,
            LoadAchievementsTestFixture,
            ::testing::Combine(
                ::testing::Values(std::vector<std::string>{ "targets", "achievements" }),
                ::testing::Values(achieveA, achieveB, achieveC, achieveNull),
                ::testing::Values(achieveA, achieveB, achieveC, achieveNull),
                ::testing::Values(NO_ERROR)
            )
        );

        INSTANTIATE_TEST_CASE_P(
            InvalidKeys,
            LoadAchievementsTestFixture,
            ::testing::Combine(
                ::testing::Values(
                    std::vector<std::string>{ "badtargetkey", "achievements" },
                    std::vector<std::string>{ "targets", "badachievementkey" }),
                ::testing::Values(achieveA, achieveNull),
                ::testing::Values(achieveA, achieveNull),
                ::testing::Values(HAS_ERROR)
            )
        );

        INSTANTIATE_TEST_CASE_P(
            InvalidVoyageId,
            LoadAchievementsTestFixture,
            ::testing::Combine(
                ::testing::Values(std::vector<std::string>{ "targets", "achievements" }),
                ::testing::Values(achieveInvalid),
                ::testing::Values(achieveA, achieveB, achieveNull),
                ::testing::Values(HAS_ERROR)
            )
        );

        TEST_P(LoadAchievementsTestFixture, TestLoadAchievement) {
            const auto& [keys, input0, input1, expectedError] = GetParam();

            json j;
            j.update(input0.j);
            j.update(input1.j);
            wrapKeys(j, keys);

            std::unordered_map<std::string, std::unordered_set<uint32_t>> expectedAchievements;
            if (input0.expectedAchievement) expectedAchievements.insert(input0.expectedAchievement.value());
            if (input1.expectedAchievement) expectedAchievements.insert(input1.expectedAchievement.value());

            std::unordered_map<std::string, std::unordered_set<uint32_t>> achievements;
            EXPECT_EQ(isError(jsonLoadUtils::loadAchievements(achievements, j)), expectedError);

            if (!expectedError)
                EXPECT_EQ(achievements, expectedAchievements);
        }
    }
}