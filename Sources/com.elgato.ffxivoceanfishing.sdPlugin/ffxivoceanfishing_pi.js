// this is our global websocket, used to communicate from/to Stream Deck software
// and some info about our plugin, as sent by Stream Deck software
var websocket = null,
uuid = null,
actionInfo = {};

// stored menu info
routesJson = {};
menuheadersJson = {};
targetsJson = {};

globalSettingsInitialized = false;
settingsInitialized = false;
isInitialized = false;
         
function connectElgatoStreamDeckSocket(inPort, inUUID, inRegisterEvent, inInfo, inActionInfo) {
    uuid = inUUID;
    // please note: the incoming arguments are of type STRING, so
    // in case of the inActionInfo, we must parse it into JSON first
    actionInfo = JSON.parse(inActionInfo); // cache the info
    websocket = new WebSocket('ws://localhost:' + inPort);
         
    // if connection was established, the websocket sends
    // an 'onopen' event, where we need to register our PI
    websocket.onopen = function () {
        var json = {
            event:  inRegisterEvent,
            uuid:   inUUID
        };
        // register property inspector to Stream Deck
        websocket.send(JSON.stringify(json));

        reloadAllSettings();
    }
         
    // retrieve saved settings if there are any
    websocket.onmessage = function (evt) {
        // Received message from Stream Deck
        const jsonObj = JSON.parse(evt.data);
        if (jsonObj.event === 'didReceiveSettings') {
            const payload = jsonObj.payload.settings;
            const menu = document.getElementById("Tracker")

            if (payload.hasOwnProperty('Route') && payload.Route != null)
            {
                document.getElementById('Routes').value = payload.Route;
                removeAllChildOptions(document.getElementById('Tracker'), '>>Select Tracker<<');
            }

            // create all the headers, this should arrive from plugin in the order we want the headers
            // to be displayed in the dropdown menu
            // ie: Blue Fish
            if (payload.hasOwnProperty('menuheaders') && payload.menuheaders != null) {
                menuheadersJson = payload.menuheaders; // save the menus globally
                // note this loop has to use .length, without it extra elements like 0,1,2,3
                // get put into the menu
                for (var i = 0; i < payload.menuheaders.length; i++) {
                    insertOptGroup(menu, payload.menuheaders[i]);
                }
            }

            // insert the targets under the appropriate headers
            // ie: Elasomosaurus -> Blue Fish
            if (payload.hasOwnProperty('targets') && payload.targets != null) {
                targetsJson = payload.targets; // save the targets globally
                for (targetName in payload.targets) {
                    insertOptionToOptGroup(menu, payload.targets[targetName], targetName, targetName);
                }
            }

            if (payload.hasOwnProperty('Name') && payload.Name != null)
                document.getElementById('Tracker').value = payload.Name;

            if (payload.DateOrTime != null && payload.DateOrTime)
                document.getElementById('select_date').checked = true;
            else
                document.getElementById('select_countdown').checked = true;

			if (payload.Priority != null && !payload.Priority)
				document.getElementById('select_bluefish').checked = true;
            else
                document.getElementById('select_achievement').checked = true;

            if (payload.hasOwnProperty('Skips') && payload.Skips != null)
                document.getElementById('set_skips').value = payload.Skips;

            if( payload.hasOwnProperty('url') && payload.url != null)
                document.getElementById('set_url').value = payload.url;

            settingsInitialized = true;
        }
        if (jsonObj.event === 'didReceiveGlobalSettings') {
            const payload = jsonObj.payload.settings;

            // create route dropdown list
            if (payload.hasOwnProperty('Routes')) {
                routesJson = payload.Routes;
                el = document.getElementById("Routes");
                removeAllChildOptions(el, ">>Select Route<<");
                for (var i = 0; i < payload.Routes.length; i++) {
                    insertOption(el, payload.Routes[i], payload.Routes[i]);
                }
            }

            if (payload.Timekeeping24HMode != null && payload.Timekeeping24HMode)
                document.getElementById('select_24h').checked = true;
            else
                document.getElementById('select_12h').checked = true;

            globalSettingsInitialized = true;
        }
        if (jsonObj.event === 'sendToPropertyInspector') {
            const payload = jsonObj.payload;

            if (payload.action = "InitRoutes") {
                if (payload.hasOwnProperty('Routes'))
                    routesJson = payload.Routes;

                updateGlobalSettings();

                reloadAllSettings();
            }
        }
        if (globalSettingsInitialized && settingsInitialized && !isInitialized) {
            sendSettingsToPlugin();
            isInitialized = true;
        }
    };
}

function reloadAllSettings() {
    json = {
        "event": "getGlobalSettings",
        "context": uuid,
    };
    websocket.send(JSON.stringify(json));
    json = {
        "event": "getSettings",
        "context": uuid,
    };
    websocket.send(JSON.stringify(json));
}

// pass settings to the plugin
function sendSettingsToPlugin() {
    route_el = document.getElementById('Routes');

    tracker_el = document.getElementById('Tracker');
    group = null;
    if (tracker_el.selectedIndex >= 0)
        group = tracker_el.options[tracker_el.selectedIndex].parentElement.getAttribute('label');

    payload = {
        'Route': route_el.value,
        'Name': tracker_el.value,
        'Tracker': group,
        'DateOrTime': document.getElementById('select_date').checked,
        'Priority': document.getElementById('select_achievement').checked,
        'Skips': document.getElementById('set_skips').value,
        'url': document.getElementById('set_url').value
    };

    if (websocket) {
        const json = {
            "action": actionInfo['action'],
            "event": "sendToPlugin",
            "context": uuid,
            "payload": payload
        };
        websocket.send(JSON.stringify(json));
    }

    payload['menuheaders'] = menuheadersJson;
    payload['targets'] = targetsJson;

    saveValues(payload);
}

// updates global settings
function updateGlobalSettings()
{
    globalPayload = {
        'Routes': routesJson,
        'Timekeeping24HMode': document.getElementById('select_24h').checked
    };
    saveGlobalValues(globalPayload);
}
         
// saves a payload
function saveValues(obj) {
    if (websocket) {
        const json = {
            "event": "setSettings",
            "context": uuid,
            "payload": obj
        };
        websocket.send(JSON.stringify(json));
    }
}

// saves a global payload
function saveGlobalValues(payload) {
    if (websocket) {
        const json = {
            "event": "setGlobalSettings",
            "context": uuid,
            "payload": payload
        };
        websocket.send(JSON.stringify(json));
    }
}

// inserts an optgroup
function insertOptGroup(el, optgroupID) {
    if (el.querySelector("optgroup[label='" + optgroupID + "']") == null)
    {
        grp = document.createElement('OPTGROUP');
        grp.label = optgroupID;
        el.appendChild(grp);
    }
}

// inserts an option to specified optgroup ID
function insertOptionToOptGroup(el, optgroupID, name, value) {
	insertOptGroup(el, optgroupID);

    const optgroup = el.querySelector("optgroup[label='" + optgroupID + "']");
    insertOption(optgroup, name, value);
}

// inserts an option to specified element
function insertOption(optgroup, name, value) {
    opt = document.createElement('OPTION');
    opt.textContent = name;
    opt.value = value;
    optgroup.appendChild(opt);
}

// opens a url in the default browser
function openUrl(url) {
    if (websocket) {
        const json = {
            "event": "openUrl",
            "payload": {
                "url": url
            }
        };
        websocket.send(JSON.stringify(json));
    }
}

// remove all child options in a dropdown menu
// and then inserts a default placeholder
function removeAllChildOptions(el, placeholderText) {
    // delete all the child options
    while (el.firstChild) {
        el.removeChild(el.firstChild);
    }

    // insert a placeholder option
    createPlaceholderOption(el, placeholderText)
}

// insert a placeholder option into an element
function createPlaceholderOption(el, text) {
    opt = document.createElement('OPTION');
    opt.disabled = "disabled";
    opt.selected = "selected";
    opt.hidden = "hidden";
    opt.value = "";
    opt.textContent = text;
    el.appendChild(opt);
}