//==============================================================================
/**
@file       TimeUtils.h
@brief      utilities for time
@copyright  (c) 2020, Momoko Tomoko
**/
//==============================================================================

#pragma once

#include <string>
#include <time.h>
#include <sstream>

namespace timeutils
{
	struct date_t
	{
		std::string weekday;
		std::string month;
		uint32_t day;
		std::string time24H;
		std::string time12H;
		uint32_t year;
	};

	/**
		@brief convert a time_t to a date_t struct

		@param[out] date the output date struct
		@param[in] time_t the input time_t

		@return true if successful
	**/
	bool convertTimeToDate(date_t& date, const time_t& t)
	{
		// convert to asctime string
		struct tm localTime;
		localtime_s(&localTime, &t);
		char buffer[256];
		if (!asctime_s(buffer, 256, &localTime))
		{
			// parse asctime string
			std::stringstream ss(buffer);

			if (ss)
				ss >> date.weekday;
			else
				return false;

			if (ss)
				ss >> date.month;
			else
				return false;

			if (ss)
				ss >> date.day;
			else
				return false;

			if (ss)
				ss >> date.time24H;
			else
				return false;

			// parse the 24h time string into 12h time without seconds
			uint32_t firstColonLoc = date.time24H.find(":");
			if (firstColonLoc != std::string::npos)
			{
				uint32_t hour = stoi(date.time24H.substr(0, firstColonLoc));
				std::string amPm = "AM";
				if (hour >= 12)
				{
					amPm = "PM";
					hour -= 12;
				}
				if (hour == 0)
					hour = 12;

				std::string hourString = std::to_string(hour);
				hourString.insert(hourString.begin(), 2 - hourString.length(), '0');

				date.time12H = hourString + date.time24H.substr(firstColonLoc, 3) + amPm;
			}
			else
				return false;

			if (ss)
				ss >> date.year;
			else
				return false;

			return true;
		}
		return false;
	}

	/**
		@brief convert seconds into a XhXmXs string

		@param[in] seconds number of seconds

		@return the parsed string
	**/
	std::string convertSecondsToHMSString(int seconds)
	{
		int S = std::abs(seconds) % 60;
		int M = static_cast<int>(std::floor(std::abs(seconds) / 60)) % 60;
		int H = static_cast<int>(std::floor(seconds / 3600));
		
		std::string output;
		if (H > 0)
			output += std::to_string(H) + "h";
		output += std::to_string(M) + "m" + std::to_string(S) + "s";
		return output;
	}
}