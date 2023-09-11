//
// pch.h
//

#pragma once

#include "gtest/gtest.h"

#include "../../Vendor/json/src/json.hpp"

using json = nlohmann::json;

const bool HAS_ERROR = true;

struct jstruct_t
{
    json j;
    jstruct_t(std::string jsonString) { j = json::parse(jsonString); }
};
