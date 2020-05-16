//==============================================================================
/**
@file       FFXIVOceanFishingTrackerPlugin.cpp
@brief      FFXIV Ocean Fishing Tracker plugin
@copyright  (c) 2020, Momoko Tomoko
**/
//==============================================================================

#include "FFXIVOceanFishingTrackerPlugin.h"

#include "Windows/FFXIVOceanFishingHelper.h"
#include "Windows/TimeUtils.hpp"
#include "Windows/ImageUtils.h"
#include "Windows/CallBackTimer.h"
#include "lodepng.h"
#include "lodepng.cpp"

#include "Common/ESDConnectionManager.h"

//#define LOGGING

FFXIVOceanFishingTrackerPlugin::FFXIVOceanFishingTrackerPlugin()
{
	mFFXIVOceanFishingHelper = new FFXIVOceanFishingHelper();

	// timer that is called on certain minutes of the hour
	mTimer = new CallBackTimer();
	const std::set <int> triggerMinutesOfTheHour = { 0, 15 };
	mTimer->start(triggerMinutesOfTheHour, [this]()
		{
			// warning: this is called in the callbacktimer on a loop at certain time intervals

			#ifdef LOGGING
			mConnectionManager->LogMessage("Callback function triggered");
			#endif
			// For each context, load what the context is trying to track.
			// Then compute the seconds until the next window
			bool status = true;
			this->mVisibleContextsMutex.lock();
			for (auto& context : mContextServerMap)
			{
				// First find what routes we are actually looking for.
				// So convert what is requested to be tracked into route IDs
				std::set<unsigned int> routeIds;
				if (context.second.tracker == "Blue Fish")
				{
					routeIds = mFFXIVOceanFishingHelper->getRoutesWithBlueFish(context.second.name);
				}
				else if (context.second.tracker == "Routes")
				{
					routeIds = mFFXIVOceanFishingHelper->getRouteIdFromName(context.second.name);
				}
				else if (context.second.tracker == "Other")
				{
					routeIds = {};
					time_t startTime = time(0);
					context.second.name = mFFXIVOceanFishingHelper->getNextRouteName(startTime, context.second.skips);
					mConnectionManager->LogMessage(context.second.name);
					updateImage(context.second.name, context.first);
				}

				// now call the helper to compute the relative time until the next window
				int relativeSecondsTillNextRoute = 0;
				int relativeWindowTime = 0;
				time_t startTime = time(0);
				status = mFFXIVOceanFishingHelper->getSecondsUntilNextRoute(relativeSecondsTillNextRoute, relativeWindowTime, startTime, routeIds, context.second.skips);

				// store the absolute times
				context.second.routeTime = startTime + relativeSecondsTillNextRoute;
				context.second.windowTime = startTime + relativeWindowTime;
			}
			this->mVisibleContextsMutex.unlock();

			this->UpdateUI();
			return status;
		});

	//timer that is called every half a second to update UI
	mSecondsTimer = new CallBackTimer();
	mSecondsTimer->start(500, [this]()
		{
			// warning: this is called in the callbacktimer on a loop at certain time intervals

			this->UpdateUI();
		});

	// the "Next Route" name has no attached image, the image gets assigned in the callback
	// Insert a null image here so there's no error thrown for a missing image file.
	mImageNameToBase64Map.insert({ "Next Route", "" });
}

FFXIVOceanFishingTrackerPlugin::~FFXIVOceanFishingTrackerPlugin()
{
	if(mTimer != nullptr)
	{
		mTimer->stop();
		
		delete mTimer;
		mTimer = nullptr;
	}
	
	if (mSecondsTimer != nullptr)
	{
		mSecondsTimer->stop();

		delete mSecondsTimer;
		mSecondsTimer = nullptr;
	}

	if(mFFXIVOceanFishingHelper != nullptr)
	{
		delete mFFXIVOceanFishingHelper;
		mFFXIVOceanFishingHelper = nullptr;
	}
}

/**
	@brief Updates all visible contexts for this app. 
**/
void FFXIVOceanFishingTrackerPlugin::UpdateUI()
{
	if(mConnectionManager != nullptr)
	{
		bool isSuccessful = true;
		mVisibleContextsMutex.lock();
		// go through all our visible contexts and set the title to show what we are tracking and the window times
		time_t now = time(0);
		for (const auto & context : mContextServerMap)
		{
			if (context.second.name.length() > 0)
			{
				std::string titleString = context.second.name + "\n";
				// if we have skips, add a number to the top right
				if (context.second.skips != 0)
				{
					titleString += "               " + std::to_string(context.second.skips) + "\n";
				}
				else
					titleString += "\n";

				// check to see if we are in a window, and display a timer for that
				int windowTimeLeft = static_cast<int>(difftime(context.second.windowTime, now));
				if (windowTimeLeft <= 15*60 && windowTimeLeft > 0)
				{
					titleString += "Window ends:\n";
					titleString += timeutils::convertSecondsToHMSString(windowTimeLeft) + "\n";
				}
				else
					titleString += "\n\n";

				// display either the date or time
				if (context.second.dateOrTime)
				{
					timeutils::date_t date{};
					if (timeutils::convertTimeToDate(date, context.second.routeTime))
						titleString += date.weekday + " " + date.month + " " + std::to_string(date.day) + "\n" + date.time12H;
					else
						titleString += "Error";
				}
				else
				{
					titleString += timeutils::convertSecondsToHMSString(static_cast<int>(difftime(context.second.routeTime, now)));
				}
				
				// send the title to StreamDeck
				mConnectionManager->SetTitle(titleString, context.first, kESDSDKTarget_HardwareAndSoftware);
			}
		}
		mVisibleContextsMutex.unlock();
	}

}


void FFXIVOceanFishingTrackerPlugin::KeyDownForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Nothing to do
}

void FFXIVOceanFishingTrackerPlugin::KeyUpForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Nothing to do
}

/**
	@brief Reads a payload into a contextMetaData_t struct

	@param[in] payload the json payload

	@return the contextMetaData_t struct
**/
FFXIVOceanFishingTrackerPlugin::contextMetaData_t FFXIVOceanFishingTrackerPlugin::readJsonIntoMetaData(const json& payload)
{
	#ifdef LOGGING
	mConnectionManager->LogMessage(payload.dump(4));
	#endif
	contextMetaData_t data{};
	if (payload.find("Tracker") != payload.end())
	{
		data.tracker = payload["Tracker"].get<std::string>();
	}
	if (payload.find("Name") != payload.end())
	{
		data.name = payload["Name"].get<std::string>();
	}
	if (payload.find("DateOrTime") != payload.end())
	{
		data.dateOrTime = payload["DateOrTime"].get<bool>();
	}
	if (payload.find("Skips") != payload.end())
	{
		std::string skips = payload["Skips"].get<std::string>();
		if (skips.length() > 0)
			data.skips = std::stoi(skips);
	}
	data.routeTime = 0;
	data.windowTime = 0;
	return data;
}

/**
	@brief Updates the selected context's image icon

	@param[in] name the tracker name. The function will look for the image file in Icons/<name>.png
**/
void FFXIVOceanFishingTrackerPlugin::updateImage(std::string name, const std::string& inContext)
{
	if (mImageNameToBase64Map.find(name) != mImageNameToBase64Map.end()) // if image was cached, just retrieve from cache
	{
		mConnectionManager->SetImage(mImageNameToBase64Map.at(name), inContext, 0);
	}
	else // load image if not cached, have to load it from disk
	{
		std::vector<unsigned char> buffer;
		std::string filename = "Icons/" + name + ".png";
		if (lodepng::load_file(buffer, filename) == 0)
		{
			std::string base64Image;
			imageutils::pngToBase64(base64Image, buffer);
			mImageNameToBase64Map.insert({ name, base64Image }); // store to cache for next time
			mConnectionManager->SetImage(base64Image, inContext, 0);
		}
		else
		{
			mConnectionManager->LogMessage("Error: unable to load image icon for target: " + name);
			mConnectionManager->SetImage("", inContext, 0);
		}
	}
}
/**
	@brief Runs when app shows up on streamdeck profile
**/
void FFXIVOceanFishingTrackerPlugin::WillAppearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// setup the dropdown menu by sending all possible settings
	mInitMutex.lock();
	if (mConnectionManager != nullptr && mIsInit == false)
	{
		json j;
		std::set<std::string> blueFish = mFFXIVOceanFishingHelper->getAllBlueFishNames();
		for (const auto& name : blueFish)
		{
			j["menu"].emplace(name, "Blue Fish");
		}
		std::set<std::string> routes = mFFXIVOceanFishingHelper->getAllRouteNames();
		for (const auto& name : routes)
		{
			j["menu"].emplace(name, "Routes");
		}
		j["menu"].emplace("Next Route", "Other");
		mConnectionManager->SetGlobalSettings(j);
		mIsInit = true;
	}
	mInitMutex.unlock();

	// read payload for any saved settings, update image if needed
	contextMetaData_t data{};
	if (inPayload.find("settings") != inPayload.end())
	{
		data = readJsonIntoMetaData(inPayload["settings"]);
	}
	updateImage(data.name, inContext);

	// Remember the context and the saved server name for this app
	mVisibleContextsMutex.lock();
	mContextServerMap.insert({ inContext, data });
	mVisibleContextsMutex.unlock();

	// update tracked timers
	this->mTimer->wake();
}

/**
	@brief Runs when app is hidden streamdeck profile
**/
void FFXIVOceanFishingTrackerPlugin::WillDisappearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// Remove this particular context so we don't have to process it when updating UI
	mVisibleContextsMutex.lock();
	mContextServerMap.erase(inContext);
	mVisibleContextsMutex.unlock();
}

void FFXIVOceanFishingTrackerPlugin::DeviceDidConnect(const std::string& inDeviceID, const json &inDeviceInfo)
{
	// Nothing to do
}

void FFXIVOceanFishingTrackerPlugin::DeviceDidDisconnect(const std::string& inDeviceID)
{
	// Nothing to do
}

/**
	@brief Runs when app recieves payload from PI
**/
void FFXIVOceanFishingTrackerPlugin::SendToPlugin(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// PI dropdown menu has saved new settings for this context (aka server name changed), load those
	bool nameChanged = false;
	std::string newName;
	mVisibleContextsMutex.lock();
	if (mContextServerMap.find(inContext) != mContextServerMap.end())
	{
		contextMetaData_t data = readJsonIntoMetaData(inPayload);
		if (data.name != mContextServerMap.at(inContext).name)
		{
			// update image since name changed
			nameChanged = true;
			newName = data.name;
		}

		// updated stored settings
		mContextServerMap.at(inContext) = data;
	}
	else
	{
		mConnectionManager->LogMessage("Error: SendToPlugin: could not find stored context: " + inContext + "\nPayload:\n" + inPayload.dump(4));
		mConnectionManager->LogMessage(inContext);
	}
	mVisibleContextsMutex.unlock();

	// update image if we've switched to a different tracker
	if (nameChanged)
	{
		updateImage(newName, inContext);
	}

	// update tracked timers
	this->mTimer->wake();
}