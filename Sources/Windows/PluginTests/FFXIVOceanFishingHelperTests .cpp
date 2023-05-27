//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"

#include <fstream>
#include <tuple>
#include <iostream>
#include "../FFXIVOceanFishingHelper.h"
#include "../TimeUtils.hpp"

namespace {
	using json = nlohmann::json;

	class FFXIVOceanFishingHelperBase
	{
	protected:
		const std::vector<std::string> dataFiles =
		{
			"../../com.elgato.ffxivoceanfishing.sdPlugin/oceanFishingDatabase - Indigo Route.json",
			"../../com.elgato.ffxivoceanfishing.sdPlugin/oceanFishingDatabase - Ruby Route.json"
		};

		std::unique_ptr< FFXIVOceanFishingHelper> mFFXIVOceanFishingHelper;
	};

	class FFXIVOceanFishingHelperTests :
		public FFXIVOceanFishingHelperBase,
		public ::testing::Test
	{
	protected:
		void SetUp()
		{
			mFFXIVOceanFishingHelper.reset(new FFXIVOceanFishingHelper(dataFiles));
		}
	};

	TEST_F(FFXIVOceanFishingHelperTests, DatabaseInitialization) {
		std::cout << mFFXIVOceanFishingHelper->getErrorMessage() << std::endl;
		ASSERT_TRUE(mFFXIVOceanFishingHelper->isInit());

		json jRouteNames = mFFXIVOceanFishingHelper->getRouteNames();
		std::cout << jRouteNames.dump() << std::endl;

		ASSERT_EQ(jRouteNames, json::parse(R"(
[
    "Indigo Route",
    "Ruby Route"
]
			)")
		);
	}

	TEST_F(FFXIVOceanFishingHelperTests, GetTargetsJsonIndigo) {
		json jTargets = mFFXIVOceanFishingHelper->getTargetsJson("Indigo Route");
		std::cout << jTargets.dump(4) << std::endl;

		// green fish test
		EXPECT_EQ(jTargets["Aetheric Seadragon"].get<std::string>(), "Green Fish");

		// blue fish test
		EXPECT_EQ(jTargets["Hafgufa"].get<std::string>(), "Blue Fish");

		// achievement
		EXPECT_EQ(jTargets["Balloon"].get<std::string>(), "Achievement");

		// routes
		EXPECT_EQ(jTargets["Bloodbrine"].get<std::string>(), "Routes");
		EXPECT_EQ(jTargets["Bloodbrine by Day"].get<std::string>(), "Routes");

		// pattern
		EXPECT_EQ(jTargets["Soth-X-Stone"].get<std::string>(), "Blue Fish Pattern");
	}

	TEST_F(FFXIVOceanFishingHelperTests, GetTargetsJsonRuby) {
		json jTargets = mFFXIVOceanFishingHelper->getTargetsJson("Ruby Route");
		std::cout << jTargets.dump(4) << std::endl;

		// green fish test
		EXPECT_EQ(jTargets["Fishy Shark"].get<std::string>(), "Green Fish");

		// blue fish test
		EXPECT_EQ(jTargets["Hells' Claw"].get<std::string>(), "Blue Fish");

		// achievement
		EXPECT_EQ(jTargets["Shrimp"].get<std::string>(), "Achievement");

		// routes
		EXPECT_EQ(jTargets["One River"].get<std::string>(), "Routes");
		EXPECT_EQ(jTargets["One River by Day"].get<std::string>(), "Routes");

		// pattern
		EXPECT_EQ(jTargets["X-Glass-Jewel"].get<std::string>(), "Blue Fish Pattern");
	}

	TEST_F(FFXIVOceanFishingHelperTests, GetTrackerTypesJson) {
		json jTrackerTypes = mFFXIVOceanFishingHelper->getTrackerTypesJson("Indigo Route");
		std::cout << jTrackerTypes.dump(4) << std::endl;

		std::set <std::string> trackers = {};
		for (const auto& trackerName : jTrackerTypes)
			trackers.insert(trackerName.get<std::string>());

		std::set <std::string> defaultTrackers = {
			"Achievement",
			"Blue Fish",
			"Blue Fish Pattern",
			"Green Fish",
			"Other",
			"Routes"
		};

		EXPECT_EQ(trackers, defaultTrackers);
	}

	class FFXIVOceanFishingHelperNoRouteFixture :
		public FFXIVOceanFishingHelperBase,
		public ::testing::TestWithParam<
		::testing::tuple<
			uint32_t,
			uint32_t,
			time_t,
			std::unordered_set<uint32_t>,
			std::string,
			uint32_t>>
	{
	protected:
		void SetUp()
		{
			mFFXIVOceanFishingHelper.reset(new FFXIVOceanFishingHelper(dataFiles));
		}
	};

	std::unordered_set<uint32_t> zeroId = { 0 };
	std::unordered_set<uint32_t> emptyId = { };

	INSTANTIATE_TEST_CASE_P(
		NoRouteTests,
		FFXIVOceanFishingHelperNoRouteFixture,
		::testing::Combine(
			::testing::Values(UINT_MAX), // seconds till next route
			::testing::Values(0), // window time
			::testing::Values(0, 1), // start time
			::testing::Values( // routes
				zeroId, emptyId
			),
			::testing::Values("Indigo Route", "Ruby Route", "Invalid Name"), // routeName
			::testing::Values(0, 1, 2) // skips
		)
	);

	TEST_P(FFXIVOceanFishingHelperNoRouteFixture, NoNextRoute) {
		uint32_t relativeSecondsTillNextRoute;
		uint32_t relativeWindowTime;
		std::string routeName;

		ASSERT_FALSE(mFFXIVOceanFishingHelper->getSecondsUntilNextVoyage(
			relativeSecondsTillNextRoute,
			relativeWindowTime,
			std::get<2>(GetParam()),
			std::get<3>(GetParam()),
			std::get<4>(GetParam()),
			std::get<5>(GetParam())
		));

		EXPECT_EQ(std::get<0>(GetParam()), relativeSecondsTillNextRoute);
		EXPECT_EQ(std::get<1>(GetParam()), relativeWindowTime);
	}

	class FFXIVOceanFishingHelperNextRouteFixture :
		public FFXIVOceanFishingHelperBase,
		public ::testing::TestWithParam<
		::testing::tuple<
		uint32_t,
		uint32_t,
		time_t,
		std::tuple<std::string, std::unordered_set<uint32_t>>,
		uint32_t>>
	{
	protected:
		void SetUp()
		{
			mFFXIVOceanFishingHelper.reset(new FFXIVOceanFishingHelper(dataFiles));
		}
	};

	std::tuple<std::string, std::unordered_set<uint32_t>> indigo1 = { "Indigo Route", { 9 } };
	std::tuple<std::string, std::unordered_set<uint32_t>> ruby1 = { "Ruby Route", { 3 } };

	INSTANTIATE_TEST_CASE_P(
		GetFirstRouteSeconds,
		FFXIVOceanFishingHelperNextRouteFixture,
		::testing::Combine(
			::testing::Values(7200), // seconds till next route
			::testing::Values(0), // window time
			::testing::Values(0, 1, 7199), // start time
			::testing::Values( // routes
				indigo1,
				ruby1
			),
			::testing::Values(0) // skips
		)
	);

	TEST_P(FFXIVOceanFishingHelperNextRouteFixture, getSecondsUntilNextVoyage) {
		uint32_t relativeSecondsTillNextRoute = 0;
		uint32_t relativeWindowTime = 0;

		ASSERT_TRUE(mFFXIVOceanFishingHelper->getSecondsUntilNextVoyage(
			relativeSecondsTillNextRoute,
			relativeWindowTime,
			std::get<2>(GetParam()),
			std::get<1>(std::get<3>(GetParam())),
			std::get<0>(std::get<3>(GetParam())),
			std::get<4>(GetParam())
		));

		std::cout << "Next Route Time: " << timeutils::convertSecondsToHMSString(relativeSecondsTillNextRoute) << std::endl;
		std::cout << "Window Time: " << timeutils::convertSecondsToHMSString(relativeWindowTime) << std::endl;

		EXPECT_EQ(std::get<0>(GetParam())-std::get<2>(GetParam()), relativeSecondsTillNextRoute);
		EXPECT_EQ(std::get<1>(GetParam()), relativeWindowTime);
	}

	class FFXIVOceanFishingHelperSkipRouteFixture :
		public FFXIVOceanFishingHelperBase,
		public ::testing::TestWithParam<
		::testing::tuple<
		time_t,
		std::tuple<std::string, std::unordered_set<uint32_t>>,
		uint32_t>>
	{
	protected:
		void SetUp()
		{
			mFFXIVOceanFishingHelper.reset(new FFXIVOceanFishingHelper(dataFiles));
		}
	};

	INSTANTIATE_TEST_CASE_P(
		FFXIVOceanFishingHelperRouteProcessingTests,
		FFXIVOceanFishingHelperSkipRouteFixture,
		::testing::Combine(
			::testing::Values(0, 60000), // start time
			::testing::Values( // routes
				indigo1,
				ruby1
			),
			::testing::Values(1,2,3) // skips
		)
	);

	TEST_P(FFXIVOceanFishingHelperSkipRouteFixture, GetSecondsUntilNextVoyage) {
		// first get the expected time until the window + skips
		uint32_t relativeSecondsTillNextRoute0 = 0;
		uint32_t relativeWindowTime0 = 0;
		ASSERT_TRUE(mFFXIVOceanFishingHelper->getSecondsUntilNextVoyage(
			relativeSecondsTillNextRoute0,
			relativeWindowTime0,
			std::get<0>(GetParam()),
			std::get<1>(std::get<1>(GetParam())),
			std::get<0>(std::get<1>(GetParam())),
			std::get<2>(GetParam())
		));

		// go to the time predicted above -1, the predicted time should be 1 second
		uint32_t relativeSecondsTillNextRoute1 = 0;
		uint32_t relativeWindowTime1 = 0;
		std::string routeName1;
		ASSERT_TRUE(mFFXIVOceanFishingHelper->getSecondsUntilNextVoyage(
			relativeSecondsTillNextRoute1,
			relativeWindowTime1,
			relativeSecondsTillNextRoute0 + std::get<0>(GetParam()) - 1,
			std::get<1>(std::get<1>(GetParam())),
			std::get<0>(std::get<1>(GetParam())),
			0
		));
		EXPECT_EQ(1, relativeSecondsTillNextRoute1);
	}
}