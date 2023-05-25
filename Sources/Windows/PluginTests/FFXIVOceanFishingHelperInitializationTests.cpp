#include "pch.h"

#include <fstream>
#include <tuple>
#include <iostream>
#include "../FFXIVOceanFishingHelper.h"

namespace {
	using json = nlohmann::json;

	const std::string dataFile = "../../com.elgato.ffxivoceanfishing.sdPlugin/oceanFishingDatabase.json";
	TEST(FFXIVOceanFishingHelperInitializationTests, ValidJson) {
		std::ifstream ifs(dataFile);
		if (ifs.fail()) {
			FAIL() << "Cannot open: " << dataFile;
		}
		json j = j.parse(ifs);
		ifs.close();
	}

	TEST(FFXIVOceanFishingHelperInitializationTests, DatabaseInitialization) {
		FFXIVOceanFishingHelper mFFXIVOceanFishingHelper(dataFile);
		std::cout << mFFXIVOceanFishingHelper.getErrorMessage() << std::endl;
		ASSERT_TRUE(mFFXIVOceanFishingHelper.isInit());
	}

	TEST(FFXIVOceanFishingHelperInitializationTests, DatabaseNotFound) {
		FFXIVOceanFishingHelper mFFXIVOceanFishingHelper(std::string("./NonExistantFile"));
		std::cout << mFFXIVOceanFishingHelper.getErrorMessage() << std::endl;
		ASSERT_FALSE(mFFXIVOceanFishingHelper.isInit());
	}


	class FFXIVOceanFishingHelperInitDatabaseTestFixture :
		public ::testing::TestWithParam<::testing::tuple<std::string, std::string>> {
	};

	INSTANTIATE_TEST_CASE_P(
		FFXIVOceanFishingHelperLoadDataBaseTests,
		FFXIVOceanFishingHelperInitDatabaseTestFixture,
		::testing::Combine(
			::testing::Values("", R"("pattern": [0.5])", R"("pattern": ["b"])"),
			::testing::Values("", R"("offset": 0.5)", R"("offset": "a")")
		));

	TEST_P(FFXIVOceanFishingHelperInitDatabaseTestFixture, MalformedJson) {
		std::string pattern = std::get<0>(GetParam());
		std::string offset = std::get<1>(GetParam());
		std::string comma = (pattern.length() > 0 && offset.length() > 0) ? "," : "";
		std::string j_string = R"({"schedule": {)" + pattern + comma + offset + "}}";
		json j = json::parse(j_string);
		FFXIVOceanFishingHelper mFFXIVOceanFishingHelper(j);
		std::cout << mFFXIVOceanFishingHelper.getErrorMessage() << std::endl;
		ASSERT_FALSE(mFFXIVOceanFishingHelper.isInit());
	}
}