//==============================================================================
/**
@file       FFXIVOceanFishingTrackerPlugin.cpp
@brief      FFXIV Ocean Fishing Tracker plugin
@copyright  (c) 2023, Momoko Tomoko
**/
//==============================================================================

#include "FFXIVOceanFishingTrackerPlugin.h"

#include "Windows/TimeUtils.hpp"
#include "Windows/ImageUtils.h"
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

/**
	@brief Starts the callback timers for this plugin
**/
void FFXIVOceanFishingTrackerPlugin::startTimers()
{
	stopTimers();

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
			{
				std::unique_lock<std::mutex> lock(this->mVisibleContextsMutex);
				for (auto& [context, metadata] : mContextServerMap)
				{
					// First find what voyages we are actually looking for.
					// So convert what is requested to be tracked into voyage IDs
					std::unordered_set<uint32_t> voyageIds =
						mFFXIVOceanFishingHelper->getVoyageIdByTracker(
							metadata.routeName,
							metadata.tracker,
							metadata.targetName
						);

					// now call the helper to compute the relative time until the next window
					uint32_t relativeSecondsTillNextVoyage = 0;
					uint32_t relativeWindowTime = 0;
					time_t startTime = time(0);
					status = mFFXIVOceanFishingHelper->getSecondsUntilNextVoyage(
						relativeSecondsTillNextVoyage,
						relativeWindowTime,
						startTime,
						voyageIds,
						metadata.routeName,
						metadata.skips
					);
					// store the absolute times
					metadata.voyageTime = startTime + relativeSecondsTillNextVoyage;
					metadata.windowTime = startTime + relativeWindowTime;

					std::string imageName;
					std::string buttonLabel;
					mFFXIVOceanFishingHelper->getImageNameAndLabel(
						imageName,
						buttonLabel,
						metadata.routeName,
						metadata.tracker,
						metadata.targetName,
						startTime,
						metadata.priority,
						metadata.skips
					);

					// update the image if needUpdate flag was set, or if name/label changed
					if (metadata.needUpdate ||
						metadata.imageName != imageName ||
						metadata.buttonLabel != buttonLabel)
					{
						metadata.imageName = imageName;
						metadata.buttonLabel = buttonLabel;
						updateImage(lock, metadata.imageName, context);
						metadata.needUpdate = false;
					}
				}
			}

			this->UpdateUI();
			return status;
		});

	const std::uint32_t UPDATE_INTERVAL_MS = 500;
	mSecondsTimer->start(UPDATE_INTERVAL_MS, [this]()
		{
			// warning: this is called in the callbacktimer on a loop at certain time intervals

			this->UpdateUI();
		});
}

void FFXIVOceanFishingTrackerPlugin::stopTimers()
{
	if (mTimer)
		mTimer->stop();
	if (mSecondsTimer)
		mSecondsTimer->stop();
}

/**
	@brief creates the title string displayed on a steamdeck button

	@param[in] metadata the metadata to generate the string for
	@param[in] currentTime the current time

	@return the title string
**/
std::string FFXIVOceanFishingTrackerPlugin::createTitleString(
	const contextMetaData_t& metadata,
	const std::time_t& currentTime
)
{
	// TODO add 24h mode
	if (metadata.targetName.empty()) return "";

	std::string titleString  = metadata.buttonLabel + "\n";
	// if we have skips, add a number to the top right
	if (metadata.skips != 0)
		titleString += "               " + std::to_string(metadata.skips);
	titleString += "\n";

	// check to see if we are in a window, and display a timer for that
	int windowTimeLeft = static_cast<int>(difftime(metadata.windowTime, currentTime));
	if (windowTimeLeft <= 15 * 60 && windowTimeLeft > 0)
	{
		titleString += "Window ends:\n";
		titleString += timeutils::convertSecondsToHMSString(windowTimeLeft) + "\n";
	}
	else
		titleString += "\n\n";

	// display either the date or time
	if (metadata.dateOrTime)
	{
		timeutils::date_t date{};
		if (timeutils::convertTimeToDate(date, metadata.voyageTime))
			if (mTimekeepingMode == TIMEKEEPING_MODE::MODE_24H)
				titleString += date.weekday + " " + date.month + " " + std::to_string(date.day) + "\n" + date.time24H;
			else
				titleString += date.weekday + " " + date.month + " " + std::to_string(date.day) + "\n" + date.time12H;
		else
			titleString += "Error";
	}
	else
		titleString += "\n" + timeutils::convertSecondsToHMSString(
			static_cast<int>(difftime(metadata.voyageTime, currentTime)));

	return titleString;
}


/**
	@brief Updates all visible contexts for this app. 
**/
void FFXIVOceanFishingTrackerPlugin::UpdateUI()
{
	if (mConnectionManager == nullptr) return;

	std::unique_lock<std::mutex> lock(mVisibleContextsMutex);
	// go through all our visible contexts and set the title to show what we are tracking and the window times
	time_t now = time(0);
	for (const auto& [context, metadata] : mContextServerMap)
		mConnectionManager->SetTitle(createTitleString(metadata, now), context, kESDSDKTarget_HardwareAndSoftware);
}


void FFXIVOceanFishingTrackerPlugin::KeyDownForAction(const std::string& /*inAction*/, const std::string& inContext, const json &/*inPayload*/, const std::string& /*inDeviceID*/)
{
	std::string url;
	{
		std::unique_lock<std::mutex> lock(mVisibleContextsMutex);
		url = mContextServerMap.at(inContext).url;
	}

	if (url.find("https://") != 0 && url.find("http://") != 0)
		url = "https://" + url;
	mConnectionManager->OpenUrl(url);
}

void FFXIVOceanFishingTrackerPlugin::KeyUpForAction(const std::string& /*inAction*/, const std::string& /*inContext*/, const json &/*inPayload*/, const std::string& /*inDeviceID*/)
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

	data.priority = PRIORITY::BLUE_FISH;
	if (payload.contains("Priority"))
		if (payload["Priority"].get<bool>())
			data.priority = PRIORITY::ACHIEVEMENTS;

	if (payload.contains("Skips"))
	{
		std::string skips = payload["Skips"].get<std::string>();
		if (skips.length() > 0)
			data.skips = std::stoi(skips);
	}

	if (payload.contains("url"))
		data.url = payload["url"].get<std::string>();

	data.voyageTime = 0;
	data.windowTime = 0;
	data.needUpdate = false;

	// TODO: This is just for backwards compatibility, remove later
	if (data.targetName == "Next Route")
		data.targetName = "Next Voyage";
	return data;
}

std::optional<std::string> FFXIVOceanFishingTrackerPlugin::loadBase64Image(const std::unique_lock<std::mutex>& lock, const std::string& imageName)
{
	if (!lock.owns_lock()) return std::nullopt;

	// if image was cached, just retrieve from cache
	if (mImageNameToBase64Map.contains(imageName)) return mImageNameToBase64Map.at(imageName);

	// image was not cached, try to load the image from file
	std::vector<uint8_t> buffer;
	const std::string filename = "Icons/" + imageName + ".png";
	if (lodepng::load_file(buffer, filename) != 0) return std::nullopt; // load failed

	std::string base64Image;
	imageutils::pngToBase64(base64Image, buffer);
	mImageNameToBase64Map.emplace( imageName, base64Image ); // store to cache for next time
	return base64Image;
}


/**
	@brief Updates the selected context's image icon

	@param[in] name the tracker name. The function will look for the image file in Icons/<name>.png
**/
void FFXIVOceanFishingTrackerPlugin::updateImage(const std::unique_lock<std::mutex>& lock, std::string& name, const std::string& inContext)
{
	const std::string defaultImageName = "default";
	if (name.empty())
		name = defaultImageName;

	const std::optional<std::string> imgData = loadBase64Image(lock, name);

	if (!imgData)
		mConnectionManager->LogMessage("Error: unable to load image icon for target: " + name);

	mConnectionManager->SetImage(imgData.value_or(""), inContext, 0);
}

/**
	@brief Runs when app shows up on streamdeck profile
**/
void FFXIVOceanFishingTrackerPlugin::WillAppearForAction(const std::string& /*inAction*/, const std::string& inContext, const json& inPayload, const std::string& /*inDeviceID*/)
{
	if (!mIsGlobalSettingsReceived)
	{
		mConnectionManager->GetGlobalSettings();
		mIsGlobalSettingsReceived = true;
	}

	// read payload for any saved settings, update image if needed
	contextMetaData_t data{};
	if (inPayload.contains("settings"))
		data = readJsonIntoMetaData(inPayload["settings"]);
	data.needUpdate = true;

	std::unique_lock<std::mutex> lock(mVisibleContextsMutex);

	// if this is the first plugin to be displayed, boot up the timers
	if (mContextServerMap.empty())
		startTimers();

	// Remember the context and the saved server name for this app
	mContextServerMap.emplace(inContext, data);

	// update tracked timers
	this->mTimer->wake();
}

/**
	@brief Runs when app is hidden streamdeck profile
**/
void FFXIVOceanFishingTrackerPlugin::WillDisappearForAction(const std::string& /*inAction*/, const std::string& inContext, const json &/*inPayload*/, const std::string& /*inDeviceID*/)
{
	// Remove this particular context so we don't have to process it when updating UI
	std::unique_lock<std::mutex> lock(mVisibleContextsMutex);
	mContextServerMap.erase(inContext);

	// if we have no active plugin displayed, kill the timers to save cpu cycles
	if (mContextServerMap.empty())
		stopTimers();
}

/**
	@brief Runs when app recieves payload from PI
**/
void FFXIVOceanFishingTrackerPlugin::SendToPlugin(const std::string& /*inAction*/, const std::string& inContext, const json& inPayload, const std::string& /*inDeviceID*/)
{
	// setup the routes menu
	{
		std::unique_lock<std::mutex> lock(mInitMutex);
		if (mIsInit == false)
		{
			json j;
			j["Routes"] = mFFXIVOceanFishingHelper->getRouteNames();
			
			mConnectionManager->SendToPropertyInspector("InitRoutes", inContext, j);
			mIsInit = true;
		}
	}

	// PI dropdown menu has saved new settings for this context, load those
	std::unique_lock<std::mutex> lock(mVisibleContextsMutex);
	if (!mContextServerMap.contains(inContext))
	{
		mConnectionManager->LogMessage("Error: SendToPlugin: could not find stored context: " + inContext + "\nPayload:\n" + inPayload.dump(4));
		mConnectionManager->LogMessage(inContext);
		return;
	}

	contextMetaData_t data = readJsonIntoMetaData(inPayload);
	if (data.routeName != mContextServerMap.at(inContext).routeName ||
		data.targetName != mContextServerMap.at(inContext).targetName ||
		data.tracker != mContextServerMap.at(inContext).tracker)
	{
		// update image since name changed
		data.needUpdate = true;
	}

	if (data.routeName != mContextServerMap.at(inContext).routeName)
	{
		json j = inPayload;
		j["menuheaders"] = mFFXIVOceanFishingHelper->getTrackerTypesJson(data.routeName);
		j["targets"] = mFFXIVOceanFishingHelper->getTargetsJson(data.routeName);
		mConnectionManager->SetSettings(j, inContext);
	}

	// updated stored settings
	mContextServerMap.at(inContext) = data;

	// update tracked timers
	this->mTimer->wake();
}

/**
	@brief Runs when app receives global settings payload from PI
**/
void FFXIVOceanFishingTrackerPlugin::DidReceiveGlobalSettings(const json& inPayload)
{
	json j = inPayload["settings"];

	TIMEKEEPING_MODE timeMode = mTimekeepingMode;
	if (j.contains("Timekeeping24HMode"))
		timeMode = j["Timekeeping24HMode"].get<bool>() ? TIMEKEEPING_MODE::MODE_24H : TIMEKEEPING_MODE::MODE_12H;

	std::unique_lock<std::mutex> lock(mVisibleContextsMutex);
	if (timeMode == mTimekeepingMode) return;

	mTimekeepingMode = timeMode;
	// re-wake to update the button title text with new time mode
	this->mTimer->wake();
}

void FFXIVOceanFishingTrackerPlugin::DeviceDidConnect(const std::string& /*inDeviceID*/, const json& /*inDeviceInfo*/)
{
	// Nothing to do
}

void FFXIVOceanFishingTrackerPlugin::DeviceDidDisconnect(const std::string& /*inDeviceID*/)
{
	// Nothing to do
}