//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"

#include <fstream>
#include <tuple>
#include <iostream>
#include "../FFXIVOceanFishingProcessor.h"

namespace FFXIVOceanFishingProcessorInitializationTests
{
	const std::string dataFile = "../../com.elgato.ffxivoceanfishing.sdPlugin/oceanFishingDatabase - Indigo Route.json";
	TEST(FFXIVOceanFishingProcessorInitializationTests, ValidJson) {
		std::ifstream ifs(dataFile);
		if (ifs.fail()) {
			FAIL() << "Cannot open: " << dataFile;
		}
		json j = j.parse(ifs);
		ifs.close();
	}

	TEST(FFXIVOceanFishingProcessorInitializationTests, DatabaseInitialization) {
		FFXIVOceanFishingProcessor mFFXIVOceanFishingProcessor(dataFile);
		std::cout << mFFXIVOceanFishingProcessor.getErrorMessage() << std::endl;
		ASSERT_TRUE(mFFXIVOceanFishingProcessor.isInit());
	}

	TEST(FFXIVOceanFishingProcessorInitializationTests, DatabaseNotFound) {
		FFXIVOceanFishingProcessor mFFXIVOceanFishingProcessor(std::string("./NonExistantFile"));
		std::cout << mFFXIVOceanFishingProcessor.getErrorMessage() << std::endl;
		ASSERT_FALSE(mFFXIVOceanFishingProcessor.isInit());
	}


	class FFXIVOceanFishingProcessorInitDatabaseTestFixture :
		public ::testing::TestWithParam<::testing::tuple<std::string, std::string>> {
	};

	INSTANTIATE_TEST_CASE_P(
		FFXIVOceanFishingProcessorLoadDataBaseTests,
		FFXIVOceanFishingProcessorInitDatabaseTestFixture,
		::testing::Combine(
			::testing::Values("", R"("pattern": [0.5])", R"("pattern": ["b"])"),
			::testing::Values("", R"("offset": 0.5)", R"("offset": "a")")
		));

	TEST_P(FFXIVOceanFishingProcessorInitDatabaseTestFixture, MalformedJson) {
		std::string pattern = std::get<0>(GetParam());
		std::string offset = std::get<1>(GetParam());
		std::string comma = (pattern.length() > 0 && offset.length() > 0) ? "," : "";
		std::string j_string = R"({"schedule": {)" + pattern + comma + offset + "}}";
		json j = json::parse(j_string);
		FFXIVOceanFishingProcessor mFFXIVOceanFishingProcessor(j);
		std::cout << mFFXIVOceanFishingProcessor.getErrorMessage() << std::endl;
		ASSERT_FALSE(mFFXIVOceanFishingProcessor.isInit());
	}
}