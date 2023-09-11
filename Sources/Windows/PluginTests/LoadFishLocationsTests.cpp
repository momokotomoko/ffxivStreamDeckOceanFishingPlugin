//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"
#include "JsonLoadCommon.h"
#include "../FFXIVOceanFishingJsonLoadUtils.h"

namespace LoadJsonTests
{
    namespace LoadFishLocations
    {
        class LoadFishLocationsTestFixture :
            public ::testing::TestWithParam<
            ::testing::tuple<std::vector<std::string>, locationData_t, locationData_t, bool>> {};

        INSTANTIATE_TEST_CASE_P(
            NoError,
            LoadFishLocationsTestFixture,
            ::testing::Combine(
                ::testing::Values(std::vector<std::string>{ "locations" }),
                ::testing::Values(locationA, locationB, locationC, locationNull),
                ::testing::Values(locationA, locationB, locationC, locationNull),
                ::testing::Values(NO_ERROR)
            )
        );

        INSTANTIATE_TEST_CASE_P(
            InvalidKey,
            LoadFishLocationsTestFixture,
            ::testing::Combine(
                ::testing::Values(std::vector<std::string>{ "badlocationkey" }),
                ::testing::Values(locationNull),
                ::testing::Values(locationNull),
                ::testing::Values(NO_ERROR) // bad key is fine, but should return null location
            )
        );

        INSTANTIATE_TEST_CASE_P(
            InvalidLocation,
            LoadFishLocationsTestFixture,
            ::testing::Combine(
                ::testing::Values(std::vector<std::string>{ "locations" }),
                ::testing::Values(locationInvalidName, locationNoName),
                ::testing::Values(locationA, locationNull),
                ::testing::Values(HAS_ERROR)
            )
        );

        TEST_P(LoadFishLocationsTestFixture, TestLoadFishLocations) {
            const auto& [keys, input0, input1, expectedError] = GetParam();

            json j;
            j.push_back(input0.j);
            j.push_back(input1.j);
            wrapKeys(j, keys);

            std::unordered_map<std::string, locations_t> expectedLocations;
            if (input0.expectedLocation) expectedLocations.insert(input0.expectedLocation.value());
            if (input1.expectedLocation) expectedLocations.insert(input1.expectedLocation.value());

            std::unordered_map<std::string, locations_t> locations;
            EXPECT_EQ(isError(jsonLoadUtils::loadFishLocations(locations, j)), expectedError);

            if (!expectedError)
                EXPECT_EQ(locations, expectedLocations);
        }
    }
}