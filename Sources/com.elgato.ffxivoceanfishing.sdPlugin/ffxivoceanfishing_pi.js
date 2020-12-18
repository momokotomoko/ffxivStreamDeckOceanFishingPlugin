// this is our global websocket, used to communicate from/to Stream Deck software
// and some info about our plugin, as sent by Stream Deck software
var websocket = null,
uuid = null,
actionInfo = {};
         
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
         
    // retrieve saved settings if there are any
    websocket.onmessage = function (evt) {
        // Received message from Stream Deck
        const jsonObj = JSON.parse(evt.data);
        if (jsonObj.event === 'didReceiveSettings') {
            const payload = jsonObj.payload.settings;
         
            document.getElementById('Tracker').value = payload.Name;
            if (payload.DateOrTime)
            {
                document.getElementById('select_date').checked = true;
            }
            else
            {
                document.getElementById('select_countdown').checked = true;
            }
            document.getElementById('set_skips').value = payload.Skips;
            if (payload.url != null)
            {
                document.getElementById('set_url').value = payload.url;
            }
        }
        if (jsonObj.event === 'didReceiveGlobalSettings') {
            const payload = jsonObj.payload.settings;

            if (payload.hasOwnProperty('menu')) {
                for (name in payload.menu) {
                    insertOption(payload.menu[name], name, name);
                }
            }
        }
    };
}

// pass settings to the plugin
function sendSettingsToPlugin() {
    tracker_el = document.getElementById('Tracker');
    group = tracker_el.options[tracker_el.selectedIndex].parentElement.getAttribute('label');

    payload = {
            'Name':tracker_el.value,
            'Tracker':group,
            'DateOrTime':document.getElementById('select_date').checked,
            'Skips':document.getElementById('set_skips').value,
            'url':document.getElementById('set_url').value
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
    saveValues(payload);
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

// inserts an option to specified optgroup ID in the tracker dropdown menu
function insertOption(optgroupID, name, value) {
    menu = document.getElementById("Tracker");
    if (menu.querySelector("optgroup[label='" + optgroupID + "']") == null)
    {
        grp = document.createElement('OPTGROUP');
        grp.label = optgroupID;
        menu.appendChild(grp);
    }
    const el = menu.querySelector("optgroup[label='" + optgroupID + "']");
    opt = document.createElement('OPTION');
    opt.textContent = name;
    opt.value = value;
    el.appendChild(opt);
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