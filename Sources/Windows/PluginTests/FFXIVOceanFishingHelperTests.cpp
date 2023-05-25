#include "pch.h"

#include "../FFXIVOceanFishingHelper.h"

namespace
{
    using json = nlohmann::json;

    typedef struct
    {
        uint32_t routeId;
        PRIORITY priority;
    } createButtonLabelFromRouteIdTestParams;

    class FFXIVOceanFishingHelperTests : public ::testing::TestWithParam<createButtonLabelFromRouteIdTestParams>
    {
    protected:
        void SetUp()
        {
            mFFXIVOceanFishingHelper.reset(new FFXIVOceanFishingHelper(j));
        }

        json j = json::parse(R"(
{
    "stops": {
        "StopA": {
            "shortform": "A"
        },
        "StopB": {
            "shortform": "B"
        },
        "StopC": {
            "shortform": "C"
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
            3, 3, 3, 1, 2, 3, 1, 2, 3],
        "offset": 1
    }
}
	)");
        std::unique_ptr< FFXIVOceanFishingHelper> mFFXIVOceanFishingHelper;
    };

    TEST_F(FFXIVOceanFishingHelperTests, getRouteIdByTrackerAchievementTest) {
        std::string tracker = "Achievement";
        std::string name = "AchieveAB";
        std::unordered_set<uint32_t> ids = mFFXIVOceanFishingHelper->getRouteIdByTracker(tracker, name);
        ASSERT_EQ(ids, std::unordered_set<uint32_t>({ 1,2 }));
    }

    TEST_F(FFXIVOceanFishingHelperTests, getRouteIdByTrackerTest) {
        std::string tracker = "Blue Fish";
        std::string name = "Fish from A night or sunset";
        std::unordered_set<uint32_t> ids = mFFXIVOceanFishingHelper->getRouteIdByTracker(tracker, name);
        ASSERT_EQ(ids, std::unordered_set<uint32_t>({ 1,2 }));
    }
}