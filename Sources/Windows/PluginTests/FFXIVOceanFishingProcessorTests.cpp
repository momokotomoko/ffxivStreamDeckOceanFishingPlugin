//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"

#include "../FFXIVOceanFishingProcessor.h"

namespace
{
    using json = nlohmann::json;

    class FFXIVOceanFishingProcessorBase
    {
    protected:
        json j = json::parse(R"(
{
    "name": "Test Route",
    "stops": {
        "StopA": {
            "shortform": "A"
        },
        "StopB": {
            "shortform": "B"
        },
        "StopC": {
            "shortform": "C"
        },
        "StopD": {
            "shortform": "D"
        }
    },
    "routes": {
            "Route1": {
                "shortform": "r1",
                "id": 1,
                "stops": [
                    {
                        "name": "StopA",
                        "time": "night"
                    },
                    {
                        "name": "StopB",
                        "time": "day"
                    },
                    {
                        "name": "StopC",
                        "time": "sunset"
                    }
                ]
            },
            "Route2": {
                "shortform": "r2",
                "id": 2,
                "stops": [
                    {
                        "name": "StopC",
                        "time": "day"
                    },
                    {
                        "name": "StopA",
                        "time": "sunset"
                    },
                    {
                        "name": "StopB",
                        "time": "night"
                    }
                ]
            },
            "Route3": {
                "shortform": "r3",
                "id": 3,
                "stops": [
                    {
                        "name": "StopC",
                        "time": "day"
                    },
                    {
                        "name": "StopC",
                        "time": "sunset"
                    },
                    {
                        "name": "StopC",
                        "time": "night"
                    }
                ]
            },
            "Route4": {
                "shortform": "r4",
                "id": 4,
                "stops": [
                    {
                        "name": "StopD",
                        "time": "day"
                    },
                    {
                        "name": "StopD",
                        "time": "sunset"
                    },
                    {
                        "name": "StopD",
                        "time": "night"
                    }
                ]
            }
    },
    "targets": {
        "fish":
            {
                "Blue Fish": {
                    "Fish from A night or sunset": {
                        "shortform": "f1",
                        "locations": [
                            {
                                "name": "StopA",
                                "time": ["night", "sunset"]
                            }
                        ]
                    },
                    "Fish from B night": {
                        "shortform": "f2",
                        "locations": [
                            {
                                "name": "StopB",
                                "time": "night"
                            }
                        ]
                    },
					"Fish from any time B": {
                        "locations": [
                            {
                                "name": "StopB"
                            }
                        ]
                    }
                }
        },
        "achievements": {
            "AchieveAB": {
                "routeIds": [1, 2]
            },
            "AchieveB": {
                "routeIds": [2]
            }
        }
    },
    "schedule": {
        "pattern": [
            3, 3, 3, 1, 2, 3, 1, 2, 3, 4],
        "offset": 1
    }
}
	)");

        std::unique_ptr<FFXIVOceanFishingProcessor> mFFXIVOceanFishingProcessor;
    };

    class GetRouteIdByTrackerTestFixture :
        public FFXIVOceanFishingProcessorBase,
        public ::testing::TestWithParam<
        ::testing::tuple<
        std::string,
        std::string,
        std::unordered_set<uint32_t>>>
    {
    protected:
        void SetUp()
        {
            mFFXIVOceanFishingProcessor.reset(new FFXIVOceanFishingProcessor(j));
        }
    };

    INSTANTIATE_TEST_CASE_P(
        GetRouteIdByTrackerTest,
        GetRouteIdByTrackerTestFixture,
        ::testing::Values(
            std::make_tuple("Achievement", "AchieveAB", std::unordered_set<uint32_t>({ 1, 2 })),
            std::make_tuple("Blue Fish", "Fish from B night", std::unordered_set<uint32_t>({ 2 })),
            std::make_tuple("Blue Fish", "Fish from A night or sunset", std::unordered_set<uint32_t>({ 1, 2 })),
            std::make_tuple("Blue Fish", "Fish from any time B", std::unordered_set<uint32_t>({ 1, 2 })),
            std::make_tuple("Other", "Next Route", std::unordered_set<uint32_t>({ 1, 2, 3, 4 })),
            std::make_tuple("Blue Fish", "Does not exist", std::unordered_set<uint32_t>({ })),
            std::make_tuple("Achievement", "Does not exist", std::unordered_set<uint32_t>({ })),
            std::make_tuple("Invalid Tracker", "AchieveAB", std::unordered_set<uint32_t>({ }))
        )
    );

    TEST_P(GetRouteIdByTrackerTestFixture, GetRouteIdByTrackerTest) {
        std::string tracker = std::get<0>(GetParam());
        std::string name = std::get<1>(GetParam());
        std::unordered_set<uint32_t> ids = mFFXIVOceanFishingProcessor->getRouteIdByTracker(tracker, name);
        EXPECT_EQ(ids, std::get<2>(GetParam()));
    }

    class GetImageNameAndLabelTestFixture :
        public FFXIVOceanFishingProcessorBase,
        public ::testing::TestWithParam<
        ::testing::tuple<
        std::string,
        std::string,
        std::string,
        std::string,
        time_t,
        PRIORITY,
        uint32_t>>
    {
    protected:
        void SetUp()
        {
            mFFXIVOceanFishingProcessor.reset(new FFXIVOceanFishingProcessor(j));
        }
    };

    INSTANTIATE_TEST_CASE_P(
        GetImageNameAndLabelTest,
        GetImageNameAndLabelTestFixture,
        ::testing::Values(
            std::make_tuple("", "", "Invalid Tracker Name", "Fish from any time B", 0, PRIORITY::ACHIEVEMENTS, 0),
            std::make_tuple("", "", "Blue Fish", "Invalid Fish Name", 0, PRIORITY::ACHIEVEMENTS, 0),
            std::make_tuple("Fish from A night or sunset", "Fish from A night or sunset", "Blue Fish", "Fish from A night or sunset", 0, PRIORITY::BLUE_FISH, 0),
            std::make_tuple("Fish from B night", "Fish from B night", "Blue Fish", "Fish from B night", 0, PRIORITY::BLUE_FISH, 1),
            std::make_tuple("Fish from B night", "Fish from B night", "Blue Fish", "Fish from B night", 0, PRIORITY::ACHIEVEMENTS, 0),
            std::make_tuple("AchieveAB", "AchieveAB", "Achievement", "AchieveAB", 0, PRIORITY::ACHIEVEMENTS, 0),
            std::make_tuple("f1-Fish from any time B-X", "f1-Fish from any time B-X", "Blue Fish Pattern", "f1-Fish from any time B-X", 0, PRIORITY::ACHIEVEMENTS, 0),
            std::make_tuple("", "", "Routes", "A", 0, PRIORITY::ACHIEVEMENTS, 0), // location A has no last stop, so it is not a route
            std::make_tuple("AchieveAB-AchieveB", "AchieveAB-AchieveB", "Routes", "B", 0, PRIORITY::ACHIEVEMENTS, 1),
            std::make_tuple("X-f1-f2", "X-f1-f2", "Routes", "B", 0, PRIORITY::BLUE_FISH, 1),
            std::make_tuple("", "", "Routes", "C", 0, PRIORITY::ACHIEVEMENTS, 0),
            std::make_tuple("", "", "Routes", "C", 0, PRIORITY::ACHIEVEMENTS, 1),
            std::make_tuple("AchieveAB", "AchieveAB", "Routes", "C", 0, PRIORITY::ACHIEVEMENTS, 2),
            std::make_tuple("", "", "Routes", "C", 0, PRIORITY::BLUE_FISH, 0),
            std::make_tuple("", "", "Routes", "C", 0, PRIORITY::BLUE_FISH, 1),
            std::make_tuple("f1-Fish from any time B-X", "f1-Fish from any time B-X", "Routes", "C", 0, PRIORITY::BLUE_FISH, 2),
            std::make_tuple("", "", "Routes", "D", 0, PRIORITY::BLUE_FISH, 2),
            std::make_tuple("", "", "Routes", "D", 0, PRIORITY::ACHIEVEMENTS, 2),
            std::make_tuple("", "", "Other", "Next Route", 0, PRIORITY::BLUE_FISH, 0),
            std::make_tuple("", "", "Other", "Next Route", 0, PRIORITY::ACHIEVEMENTS, 0),
            std::make_tuple("f1-Fish from any time B-X", "f1-Fish from any time B-X", "Other", "Next Route", 0, PRIORITY::BLUE_FISH, 2),
            std::make_tuple("AchieveAB", "AchieveAB", "Other", "Next Route", 0, PRIORITY::ACHIEVEMENTS, 2),
            std::make_tuple("f1-Fish from any time B-X", "f1-Fish from any time B-X", "Other", "Next Route", 1, PRIORITY::BLUE_FISH, 2),
            std::make_tuple("AchieveAB", "AchieveAB", "Other", "Next Route", 1, PRIORITY::ACHIEVEMENTS, 2),
            std::make_tuple("f1-Fish from any time B-X", "f1-Fish from any time B-X", "Other", "Next Route", 7200 * 3, PRIORITY::BLUE_FISH, 0),
            std::make_tuple("AchieveAB", "AchieveAB", "Other", "Next Route", 7200 * 3, PRIORITY::ACHIEVEMENTS, 0)
        )
    );

    
    TEST_P(GetImageNameAndLabelTestFixture, GetImageNameAndLabelTest) {

        std::string imageName;
        std::string buttonLabel;
        mFFXIVOceanFishingProcessor->getImageNameAndLabel(
            imageName,
            buttonLabel,
            std::get<2>(GetParam()),
            std::get<3>(GetParam()),
            std::get<4>(GetParam()),
            std::get<5>(GetParam()),
            std::get<6>(GetParam()));
        EXPECT_EQ(imageName, std::get<0>(GetParam()));
        EXPECT_EQ(buttonLabel, std::get<1>(GetParam()));
    }
}