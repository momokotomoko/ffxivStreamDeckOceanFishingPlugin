//==============================================================================
/**
@file       FFXIVOceanFishingTrackerPlugin.cpp
@brief      FFXIV Ocean Fishing Tracker plugin
@copyright  (c) 2023, Momoko Tomoko
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
	mFFXIVOceanFishingHelper = std::make_unique<FFXIVOceanFishingHelper>(
		std::vector <std::string>({
			"oceanFishingDatabase - Indigo Route.json",
			"oceanFishingDatabase - Ruby Route.json"
		})
	);

	// timer that is called on certain minutes of the hour
	mTimer = std::make_unique <CallBackTimer>();
	//timer that is called every half a second to update UI
	mSecondsTimer = std::make_unique <CallBackTimer>();

	startTimers();
}

FFXIVOceanFishingTrackerPlugin::~FFXIVOceanFishingTrackerPlugin()
{
	if(mTimer)
		mTimer->stop();
	
	if (mSecondsTimer)
		mSecondsTimer->stop();
}

/**
	@brief Starts the callback timers for this plugin
**/
void FFXIVOceanFishingTrackerPlugin::startTimers()
{
	mTimer->stop();
	mSecondsTimer->stop();

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
				std::unordered_set<uint32_t> routeIds =
					mFFXIVOceanFishingHelper->getVoyageIdByTracker(
						context.second.routeName,
						context.second.tracker,
						context.second.targetName
					);

				// now call the helper to compute the relative time until the next window
				uint32_t relativeSecondsTillNextRoute = 0;
				uint32_t relativeWindowTime = 0;
				time_t startTime = time(0);
				status = mFFXIVOceanFishingHelper->getSecondsUntilNextVoyage(
					relativeSecondsTillNextRoute,
					relativeWindowTime,
					startTime,
					routeIds,
					context.second.routeName,
					context.second.skips
				);
				// store the absolute times
				context.second.routeTime = startTime + relativeSecondsTillNextRoute;
				context.second.windowTime = startTime + relativeWindowTime;

				std::string imageName;
				std::string buttonLabel;
				mFFXIVOceanFishingHelper->getImageNameAndLabel(
					imageName,
					buttonLabel,
					context.second.routeName,
					context.second.tracker,
					context.second.targetName,
					context.second.priority,
					context.second.skips
				);
				if (context.second.imageName != imageName || context.second.buttonLabel != buttonLabel)
					context.second.needUpdate = true;

				// update the image
				if (context.second.needUpdate)
				{
					context.second.imageName = imageName;
					context.second.buttonLabel = buttonLabel;
					updateImage(context.second.imageName, context.first);
					context.second.needUpdate = false;
				}
			}
			this->mVisibleContextsMutex.unlock();

			this->UpdateUI();
			return status;
		});

	mSecondsTimer->start(500, [this]()
		{
			// warning: this is called in the callbacktimer on a loop at certain time intervals

			this->UpdateUI();
		});
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
			std::string titleString = "";
			if (context.second.targetName.length() > 0)
			{
				titleString = context.second.buttonLabel + "\n";
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
					titleString += "\n" + timeutils::convertSecondsToHMSString(static_cast<int>(difftime(context.second.routeTime, now)));
				}
			}
			// send the title to StreamDeck
			mConnectionManager->SetTitle(titleString, context.first, kESDSDKTarget_HardwareAndSoftware);
		}
		mVisibleContextsMutex.unlock();
	}

}


void FFXIVOceanFishingTrackerPlugin::KeyDownForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	mVisibleContextsMutex.lock();
	std::string url = mContextServerMap.at(inContext).url;
	mVisibleContextsMutex.unlock();

	if (url.find("https://") != 0 && url.find("http://") != 0)
		url = "https://" + url;
	mConnectionManager->OpenUrl(url);
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

	if (payload.contains("Route"))
		data.routeName = payload["Route"].get<std::string>();

	if (payload.contains("Tracker") &&
		payload["Tracker"].is_string())
		data.tracker = payload["Tracker"].get<std::string>();

	if (payload.contains("Name"))
	{
		data.targetName = payload["Name"].get<std::string>();
		data.buttonLabel = data.targetName;
		data.imageName = data.imageName;
	}

	if (payload.contains("DateOrTime"))
		data.dateOrTime = payload["DateOrTime"].get<bool>();

	data.priority = BLUE_FISH;
	if (payload.contains("Priority"))
		if (payload["Priority"].get<bool>())
			data.priority = ACHIEVEMENTS;

	if (payload.contains("Skips"))
	{
		std::string skips = payload["Skips"].get<std::string>();
		if (skips.length() > 0)
			data.skips = std::stoi(skips);
	}

	if (payload.contains("url"))
		data.url = payload["url"].get<std::string>();

	data.routeTime = 0;
	data.windowTime = 0;
	data.needUpdate = false;
	return data;
}

/**
	@brief Updates the selected context's image icon

	@param[in] name the tracker name. The function will look for the image file in Icons/<name>.png
**/
void FFXIVOceanFishingTrackerPlugin::updateImage(std::string name, const std::string& inContext)
{
	if (name.empty())
		name = "default";

	// if image was cached, just retrieve from cache
	if (mImageNameToBase64Map.contains(name))
	{
		mConnectionManager->SetImage(mImageNameToBase64Map.at(name), inContext, 0);
		return;
	}

	// image was not cached, try to load the image from file
	std::vector<unsigned char> buffer;
	std::string filename = "Icons/" + name + ".png";
	if (lodepng::load_file(buffer, filename) == 0)
	{
		std::string base64Image;
		imageutils::pngToBase64(base64Image, buffer);
		mImageNameToBase64Map.insert({ name, base64Image }); // store to cache for next time
		mConnectionManager->SetImage(base64Image, inContext, 0);
		return;
	}

	// image failed to load
	mConnectionManager->LogMessage("Error: unable to load image icon for target: " + name);
	mConnectionManager->SetImage("", inContext, 0);
}

/**
	@brief Runs when app shows up on streamdeck profile
**/
void FFXIVOceanFishingTrackerPlugin::WillAppearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID)
{
	// read payload for any saved settings, update image if needed
	contextMetaData_t data{};
	if (inPayload.contains("settings"))
		data = readJsonIntoMetaData(inPayload["settings"]);
	data.needUpdate = true;

	// setup the routes menu
	mInitMutex.lock();
	if (mConnectionManager != nullptr && mIsInit == false)
	{
		json j;
		j["routes"] = mFFXIVOceanFishingHelper->getRouteNames();
		mConnectionManager->SetGlobalSettings(j);
		mIsInit = true;
	}
	mInitMutex.unlock();

	// if this is the first plugin to be displayed, boot up the timers
	if (mContextServerMap.empty())
		startTimers();

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

	// if we have no active plugin displayed, kill the timers to save cpu cycles
	if (mContextServerMap.empty())
	{
		mTimer->stop();
		mSecondsTimer->stop();
	}
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
	// PI dropdown menu has saved new settings for this context, load those
	contextMetaData_t data;
	mVisibleContextsMutex.lock();
	if (mContextServerMap.contains(inContext))
	{
		data = readJsonIntoMetaData(inPayload);
		if (data.routeName != mContextServerMap.at(inContext).routeName ||
			data.targetName != mContextServerMap.at(inContext).targetName ||
			data.tracker != mContextServerMap.at(inContext).tracker)
		{
			// update image since name changed
			data.needUpdate = true;
		}

		if (data.routeName != mContextServerMap.at(inContext).routeName)
		{
			json j;
			j["menuheaders"] = mFFXIVOceanFishingHelper->getTrackerTypesJson(data.routeName);
			j["targets"] = mFFXIVOceanFishingHelper->getTargetsJson(data.routeName);
			mConnectionManager->SetSettings(j, inContext);
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

	// update tracked timers
	this->mTimer->wake();
}