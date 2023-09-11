//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"

#include "../FFXIVOceanFishingProcessor.h"

namespace FFXIVOceanFishingProcessorTests
{
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
    "voyages": {
            "C by Sunset": {
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
            "B by Night": {
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
            "C by Night": {
                "shortform": "r3",
                "id": 3,
                "stops": [
                    {
                        "name": "StopA",
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
            "D by Night": {
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
                        "locations":
                            {
                                "name": "StopB",
                                "time": "night"
                            }
                    },
					"Fish from any time B": {
                        "locations": [
                            {
                                "name": "StopB"
                            }
                        ]
                    }
                },
                "Green Fish": {
                    "Fish from multiple locations": {
                        "locations": [
                            {
                                "name": "StopA",
                                "time": ["night", "sunset"]
                            },
                            {
                                "name": "StopD",
                                "time": "night"
                            }
                        ]
                    },
                    "Fish with no location": {
                    }
                }
        },
        "achievements": {
            "AchieveAB": {
                "voyageIds": [1, 2]
            },
            "AchieveB": {
                "voyageIds": [2]
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

    class GetVoyageIdByTrackerTestFixture :
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
        GetVoyageIdByTrackerTest,
        GetVoyageIdByTrackerTestFixture,
        ::testing::Values(
            std::make_tuple("Achievement", "AchieveAB", std::unordered_set<uint32_t>({ 1, 2 })),
            std::make_tuple("Blue Fish", "Fish from B night", std::unordered_set<uint32_t>({ 2 })),
            std::make_tuple("Blue Fish", "Fish from A night or sunset", std::unordered_set<uint32_t>({ 1, 2 })),
            std::make_tuple("Blue Fish", "Fish from any time B", std::unordered_set<uint32_t>({ 1, 2 })),
            std::make_tuple("Green Fish", "Fish from multiple locations", std::unordered_set<uint32_t>({ 1, 2, 4 })),
            std::make_tuple("Other", "Next Voyage", std::unordered_set<uint32_t>({ 1, 2, 3, 4 })),
            std::make_tuple("Blue Fish", "Does not exist", std::unordered_set<uint32_t>({ })),
            std::make_tuple("Achievement", "Does not exist", std::unordered_set<uint32_t>({ })),
            std::make_tuple("Invalid Tracker", "AchieveAB", std::unordered_set<uint32_t>({ }))
        )
    );

    TEST_P(GetVoyageIdByTrackerTestFixture, GetVoyageIdByTrackerTest) {
        std::string tracker = std::get<0>(GetParam());
        std::string name = std::get<1>(GetParam());
        std::unordered_set<uint32_t> ids = mFFXIVOceanFishingProcessor->getVoyageIdByTracker(tracker, name);
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
            std::make_tuple("Fish from multiple locations", "Fish from multiple locations", "Green Fish", "Fish from multiple locations", 0, PRIORITY::BLUE_FISH, 1),
            std::make_tuple("Fish from multiple locations", "Fish from multiple locations", "Green Fish", "Fish from multiple locations", 0, PRIORITY::ACHIEVEMENTS, 0),
            std::make_tuple("AchieveAB", "AchieveAB", "Achievement", "AchieveAB", 0, PRIORITY::ACHIEVEMENTS, 0),
            std::make_tuple("f1-Fish from any time B-X", "f1-Fish from any time B-X", "Blue Fish Pattern", "f1-Fish from any time B-X", 0, PRIORITY::ACHIEVEMENTS, 0),
            std::make_tuple("", "", "Voyages", "A", 0, PRIORITY::ACHIEVEMENTS, 0), // location A has no last stop, so it is not a voyage
            std::make_tuple("AchieveAB-AchieveB", "AchieveAB-AchieveB", "Voyages", "B", 0, PRIORITY::ACHIEVEMENTS, 1),
            std::make_tuple("X-f1-f2", "X-f1-f2", "Voyages", "B", 0, PRIORITY::BLUE_FISH, 1),
            std::make_tuple("", "", "Voyages", "C", 0, PRIORITY::ACHIEVEMENTS, 0),
            std::make_tuple("", "", "Voyages", "C", 0, PRIORITY::ACHIEVEMENTS, 1),
            std::make_tuple("AchieveAB", "AchieveAB", "Voyages", "C", 0, PRIORITY::ACHIEVEMENTS, 2),
            std::make_tuple("", "", "Voyages", "C", 0, PRIORITY::BLUE_FISH, 0),
            std::make_tuple("", "", "Voyages", "C", 0, PRIORITY::BLUE_FISH, 1),
            std::make_tuple("f1-Fish from any time B-X", "f1-Fish from any time B-X", "Voyages", "C", 0, PRIORITY::BLUE_FISH, 2),
            std::make_tuple("", "", "Voyages", "D", 0, PRIORITY::BLUE_FISH, 2),
            std::make_tuple("", "", "Voyages", "D", 0, PRIORITY::ACHIEVEMENTS, 2),
            std::make_tuple("", "", "Other", "Next Voyage", 0, PRIORITY::BLUE_FISH, 0),
            std::make_tuple("", "", "Other", "Next Voyage", 0, PRIORITY::ACHIEVEMENTS, 0),
            std::make_tuple("f1-Fish from any time B-X", "f1-Fish from any time B-X", "Other", "Next Voyage", 0, PRIORITY::BLUE_FISH, 2),
            std::make_tuple("AchieveAB", "AchieveAB", "Other", "Next Voyage", 0, PRIORITY::ACHIEVEMENTS, 2),
            std::make_tuple("f1-Fish from any time B-X", "f1-Fish from any time B-X", "Other", "Next Voyage", 1, PRIORITY::BLUE_FISH, 2),
            std::make_tuple("AchieveAB", "AchieveAB", "Other", "Next Voyage", 1, PRIORITY::ACHIEVEMENTS, 2),
            std::make_tuple("f1-Fish from any time B-X", "f1-Fish from any time B-X", "Other", "Next Voyage", 7200 * 3, PRIORITY::BLUE_FISH, 0),
            std::make_tuple("AchieveAB", "AchieveAB", "Other", "Next Voyage", 7200 * 3, PRIORITY::ACHIEVEMENTS, 0),
            std::make_tuple("AchieveAB-AchieveB", "AchieveAB-AchieveB", "Voyages", "B by Night", 0, PRIORITY::ACHIEVEMENTS, 0),
            std::make_tuple("X-f1-f2", "X-f1-f2", "Voyages", "B by Night", 0, PRIORITY::BLUE_FISH, 0),
            std::make_tuple("AchieveAB-AchieveB", "AchieveAB-AchieveB", "Voyages", "B by Night", 0, PRIORITY::ACHIEVEMENTS, 1),
            std::make_tuple("X-f1-f2", "X-f1-f2", "Voyages", "B by Night", 0, PRIORITY::BLUE_FISH, 1)
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