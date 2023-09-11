//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"

#include "../FFXIVOceanFishingJsonLoadUtils.h"


namespace LoadJsonTests
{
    namespace UtilityFunctionTests
    {
        struct isBadKeyTestInputs_t
        {
            std::string key;
            json::value_t type;
        };

        class IsBadKeyTestFixture :
            public ::testing::TestWithParam<::testing::tuple<jstruct_t, isBadKeyTestInputs_t, bool>> {
        };

        const isBadKeyTestInputs_t jObject("object", json::value_t::object);
        const isBadKeyTestInputs_t jString("string", json::value_t::string);
        const isBadKeyTestInputs_t jUInt("unsigned number", json::value_t::number_unsigned);
        const isBadKeyTestInputs_t jArray("array", json::value_t::array);
        const isBadKeyTestInputs_t jNoKey("nokey", json::value_t::array);
        const isBadKeyTestInputs_t jNull("null", json::value_t::null);

        INSTANTIATE_TEST_CASE_P(
            IsBadKeyNoError,
            IsBadKeyTestFixture,
            ::testing::Combine(
                ::testing::Values(jstruct_t(R"({"object":{}, "string": "A", "unsigned number": 1, "array": [], "null": null})")),
                ::testing::Values(jObject, jString, jUInt, jArray, jNull),
                ::testing::Values(NO_ERROR)
            )
        );

        INSTANTIATE_TEST_CASE_P(
            IsBadKeyHasError,
            IsBadKeyTestFixture,
            ::testing::Combine(
                ::testing::Values(
                    jstruct_t(R"({"array": 1})"),
                    jstruct_t(R"({"array": "1"})"),
                    jstruct_t(R"({"array": {}})"),
                    jstruct_t(R"({"unsigned number": -1})"),
                    jstruct_t(R"({"unsigned number": "1"})"),
                    jstruct_t(R"({"object": 1})"),
                    jstruct_t(R"({"object": "1"})"),
                    jstruct_t(R"({"object": null})")
                ),
                ::testing::Values(jObject, jString, jUInt, jArray, jNoKey, jNull),
                ::testing::Values(HAS_ERROR)
            )
        );

        TEST_P(IsBadKeyTestFixture, TestIsBadKey) {
            const auto& [jStruct, keyTest, expectedResult] = GetParam();
            const auto& [key, type] = keyTest;
            EXPECT_EQ(jsonLoadUtils::isBadKey(jStruct.j, key, type), expectedResult);
        }

        class ParseSingleOrArrayTestFixture :
            public ::testing::TestWithParam<
            ::testing::tuple<jstruct_t, std::unordered_set<std::string>>> {
        };

        INSTANTIATE_TEST_CASE_P(
            ParseSingleOrArrayTest,
            ParseSingleOrArrayTestFixture,
            ::testing::Values(
                std::make_tuple(jstruct_t(R"(["a", "b"])"), std::unordered_set<std::string>{"a", "b"}),
                std::make_tuple(jstruct_t(R"(["a"])"), std::unordered_set<std::string>{"a"}),
                std::make_tuple(jstruct_t(R"("a")"), std::unordered_set<std::string>{"a"})
            )
        );

        TEST_P(ParseSingleOrArrayTestFixture, TestParseSingleOrArray) {
            const auto& [jStruct, expectedResult] = GetParam();
            EXPECT_EQ(jsonLoadUtils::loadJsonStringArray(jStruct.j), expectedResult);
        }

        class ImplodeStringVectorTestFixture :
            public ::testing::TestWithParam<::testing::tuple<std::vector<std::string>, std::optional<char>, std::string>> {
        };

        INSTANTIATE_TEST_CASE_P(
            ImplodeStringVectorTests,
            ImplodeStringVectorTestFixture,
            ::testing::Values(
                std::make_tuple(std::vector<std::string>{"A", "B", "C"}, '+', "A+B+C"),
                std::make_tuple(std::vector<std::string>{"A", "B", "C"}, std::nullopt, "A-B-C"),
                std::make_tuple(std::vector<std::string>{"", "", ""}, std::nullopt, "--"),
                std::make_tuple(std::vector<std::string>{"", "B", ""}, std::nullopt, "-B-"),
                std::make_tuple(std::vector<std::string>{}, '-', ""),
                std::make_tuple(std::vector<std::string>{}, std::nullopt, "")
            )
        );

        TEST_P(ImplodeStringVectorTestFixture, TestImplodeStringVector) {
            auto & [vectorOfStrings, delim, expectedResult] = GetParam();
            std::string result = delim ?
                jsonLoadUtils::implodeStringVector(vectorOfStrings, delim.value()) :
                jsonLoadUtils::implodeStringVector(vectorOfStrings);
            EXPECT_EQ(result, expectedResult);
        }
    }
}