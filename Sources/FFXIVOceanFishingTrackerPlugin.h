//==============================================================================
/**
@file       FFXIVOceanFishingTrackerPlugin.h
@brief      FFXIV Ocean Fishing Tracker plugin
@copyright  (c) 2023, Momoko Tomoko
**/
//==============================================================================

#include "Common/ESDBasePlugin.h"
#include <mutex>
#include <unordered_map>
#include <string>
#include "Windows/Common.h"

#include "Vendor/json/src/json.hpp"
using json = nlohmann::json;

class FFXIVOceanFishingHelper;
class CallBackTimer;


class FFXIVOceanFishingTrackerPlugin : public ESDBasePlugin
{
public:
	
	FFXIVOceanFishingTrackerPlugin();
	virtual ~FFXIVOceanFishingTrackerPlugin();
	
	void KeyDownForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;
	void KeyUpForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;
	
	void WillAppearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;
	void WillDisappearForAction(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;
	
	void DeviceDidConnect(const std::string& inDeviceID, const json &inDeviceInfo) override;
	void DeviceDidDisconnect(const std::string& inDeviceID) override;
	
	void SendToPlugin(const std::string& inAction, const std::string& inContext, const json &inPayload, const std::string& inDeviceID) override;
private:
	
	void UpdateUI();
	
	std::mutex mVisibleContextsMutex;

	// on first time the app appears we need these to trigger sending global settings
	std::mutex mInitMutex;
	bool mIsInit = false;

	// this struct contains a context's saved settings
	struct contextMetaData_t
	{
		std::string routeName; // name of the route we are using
		std::string targetName; // name of target we are tracking
		std::string tracker; // tracker type, ie: blue fish or route name
		std::string buttonLabel; // the text on the top of the button
		std::string imageName; // the name of hte image to use for this button
		PRIORITY priority; // whether to prioritize showing achievements or blue fish for this button
		bool needUpdate; // true if this button needs an update to image name and label
		time_t routeTime; // time of next route
		time_t windowTime; // if we are in a fishing window, this holds the time remaining
		bool dateOrTime; // true for date, false for time
		unsigned int skips;
		std::string url; // webpage to open on click, each button can have a different webpage
	};
	std::unordered_map<std::string, contextMetaData_t> mContextServerMap;

	contextMetaData_t readJsonIntoMetaData(const json& payload);

	// contains cache of base64 images that have been loaded
	std::unordered_map<std::string, std::string> mImageNameToBase64Map;

	void updateImage(std::string pngfile, const std::string& inContext);
	
	FFXIVOceanFishingHelper *mFFXIVOceanFishingHelper = nullptr;
	CallBackTimer *mTimer = nullptr;
	CallBackTimer* mSecondsTimer = nullptr;

	void startTimers();
};
