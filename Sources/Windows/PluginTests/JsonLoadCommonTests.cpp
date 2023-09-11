//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"
#include "JsonLoadCommon.h"
#include "../FFXIVOceanFishingJsonLoadUtils.h"


namespace LoadJsonTests
{
    namespace JsonLoadCommonTests
    {
        class WrapKeysTestFixture :
            public ::testing::TestWithParam<
            ::testing::tuple<std::vector<std::string>, jstruct_t, jstruct_t>> {};

        INSTANTIATE_TEST_CASE_P(
            WrapKeysTests,
            WrapKeysTestFixture,
            ::testing::Values(
                std::make_tuple(std::vector<std::string>{ "key0"}, jstruct_t(R"({})"), jstruct_t(R"({"key0":{}})")),
                std::make_tuple(std::vector<std::string>{ "key0", "key1" }, jstruct_t(R"({"tail" : 0})"), jstruct_t(R"({"key0":{"key1":{"tail": 0}}})")),
                std::make_tuple(std::vector<std::string>{ }, jstruct_t(R"({})"), jstruct_t(R"({})")),
                std::make_tuple(std::vector<std::string>{ }, jstruct_t(R"({"tail" : 0})"), jstruct_t(R"({"tail" : 0})"))
            )
        );

        TEST_P(WrapKeysTestFixture, WrapKeysTests) {
            const auto& [keys, jTail, expectedResult] = GetParam();

            json j = jTail.j;
            wrapKeys(j, keys);

            EXPECT_EQ(j, expectedResult.j);
        }
    }
}