//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"
#include "JsonLoadCommon.h"
#include "../FFXIVOceanFishingJsonLoadUtils.h"


namespace LoadJsonTests
{
    namespace LoadRouteName
    {
        struct loadRouteNameTestInputs_t
        {
            json j;
            std::string routeName;
        };

        class LoadRouteNameTestFixture :
            public ::testing::TestWithParam<::testing::tuple<loadRouteNameTestInputs_t, bool>> {};

        INSTANTIATE_TEST_CASE_P(
            InvalidType,
            LoadRouteNameTestFixture,
            ::testing::Combine(
                ::testing::Values(
                    loadRouteNameTestInputs_t{ json::parse(R"({"name": 1})"), "" },
                    loadRouteNameTestInputs_t{ json::parse(R"({"name": null})"), "" },
                    loadRouteNameTestInputs_t{ json::parse(R"({"name": []})"), "" },
                    loadRouteNameTestInputs_t{ json::parse(R"({"name": {}})"), "" },
                    loadRouteNameTestInputs_t{ json::parse(R"({"name": null})"), "" },
                    loadRouteNameTestInputs_t{ json::parse(R"({})"), "" }
                ),
                ::testing::Values(HAS_ERROR)
            )
        );

        INSTANTIATE_TEST_CASE_P(
            MissingRouteName,
            LoadRouteNameTestFixture,
            ::testing::Values(std::make_tuple(loadRouteNameTestInputs_t{ json::parse(R"({})"), "" }, HAS_ERROR))
        );

        INSTANTIATE_TEST_CASE_P(
            NoError,
            LoadRouteNameTestFixture,
            ::testing::Values(std::make_tuple(loadRouteNameTestInputs_t{ json::parse(R"({"name": "routename"})"), "routename" }, NO_ERROR))
        );

        TEST_P(LoadRouteNameTestFixture, TestLoadRouteName) {
            const auto& [j, expectedRouteName] = std::get<0>(GetParam());

            std::string routeName;
            EXPECT_EQ(isError(jsonLoadUtils::loadRouteName(routeName, j)), std::get<1>(GetParam()));
            EXPECT_EQ(routeName, expectedRouteName);
        }
    }
}