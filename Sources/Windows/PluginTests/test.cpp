#include "pch.h"

#include <fstream>
#include "../FFXIVOceanFishingHelper.h"

const std::string dataFile = "../../com.elgato.ffxivoceanfishing.sdPlugin/oceanFishingDatabase.json";
TEST(FFXIVOceanFishingHelperTests, TestJsonDatabase) {
	std::ifstream ifs(dataFile);
	if (ifs.fail()) {
		FAIL() << "Cannot open: " << dataFile;
	}
	json j = j.parse(ifs);
	ifs.close();
}

TEST(FFXIVOceanFishingHelperTests, TestInitialization) {
	FFXIVOceanFishingHelper mFFXIVOceanFishingHelper("../../com.elgato.ffxivoceanfishing.sdPlugin/oceanFishingDatabase.json");
}