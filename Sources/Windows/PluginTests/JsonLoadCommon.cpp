//copyright  (c) 2023, Momoko Tomoko

#include "pch.h"
#include "JsonLoadCommon.h"

namespace LoadJsonTests
{
    /**
        @brief wraps a json with nested keys

        @param[in/out] jTail the json to wrap
        @param[in] nestedKeys the vector of keys to nest the json with
    **/
    void wrapKeys(json& jTail, const std::vector <std::string>& nestedKeys)
    {
        json j;
        json* it = &j;
        for (const auto& key : nestedKeys)
            it = &((*it)[key]);
        *it = jTail;
        jTail = j;
    }

    /**
        @brief returns if there was an optional error message, also prints it

        @param[in] errMsg optional error message

        @return true if there was an error message
    **/
    bool isError(const std::optional<std::string>& errMsg)
    {
        if (!errMsg) return false;
        std::cout << errMsg.value() << std::endl;
        return true;
    }
}