#include "netclock.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <time.h>

#include "config.h"
#include "reporter.h"

namespace netclock {

static bool wifiOk = false;
static bool ntpOk = false;
static uint32_t lastReconnectMs = 0;

bool configRequested() {
    // Config pin is GPIO0, held high by the internal pull-up and driven low by
    // a button to GND. Held low at startup, it opens the config portal (which
    // then stays up until the pin is released — see begin()).
    pinMode(PIN_CONFIG_BUTTON, INPUT_PULLUP);
    delayMicroseconds(50);
    return digitalRead(PIN_CONFIG_BUTTON) == LOW;
}

bool begin(bool openPortalIfNeeded) {
    WiFiManager wm;

    WiFiManagerParameter urlParam("url", "Reporting URL", reporter::url(), 256);
    WiFiManagerParameter nameParam("name", "Device name (reporting key)",
                                   reporter::name(), DEVICE_NAME_MAX_LEN);
    wm.addParameter(&urlParam);
    wm.addParameter(&nameParam);

    // Persist URL + name the moment the form is submitted (covers /wifisave when
    // the user enters them together with WiFi creds, and /paramsave for
    // settings-only updates). Without this, a reset before STA actually
    // associates would lose in-memory-only values.
    auto persistParams = [&urlParam, &nameParam]() {
        reporter::setUrl(urlParam.getValue());
        reporter::setName(nameParam.getValue());
    };
    wm.setSaveConfigCallback(persistParams);
    wm.setSaveParamsCallback(persistParams);

    if (!openPortalIfNeeded) {
        wm.setEnableConfigPortal(true);   // open portal automatically if saved creds fail
        wm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT_S);
        wifiOk = wm.autoConnect(WIFI_AP_NAME);
    } else {
        // Portal stays up for as long as the config pin is held low. Releasing
        // it closes the portal and we proceed with whatever creds are now
        // stored. No timeout — non-blocking so we can poll the pin each pass.
        pinMode(PIN_CONFIG_BUTTON, INPUT_PULLUP);
        wm.setConfigPortalBlocking(false);
        wm.setConfigPortalTimeout(0);
        wm.startConfigPortal(WIFI_AP_NAME);

        while (digitalRead(PIN_CONFIG_BUTTON) == LOW) {
            wm.process();
            delay(10);
        }
        wm.stopConfigPortal();

        reporter::setUrl(urlParam.getValue());
        reporter::setName(nameParam.getValue());

        // The user may have released the pin before the link came up (or without
        // touching WiFi at all). If we aren't connected yet, give stored creds a
        // chance to associate before falling through to the no-WiFi path.
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.mode(WIFI_STA);
            WiFi.begin();   // use creds saved in NVS (by the portal or earlier)
            uint32_t deadline = millis() + 10000;
            while (WiFi.status() != WL_CONNECTED && millis() < deadline) delay(50);
        }
        wifiOk = (WiFi.status() == WL_CONNECTED);
    }

    if (!wifiOk) {
        WiFi.mode(WIFI_OFF);
        return false;
    }

    // Backstop: let the driver re-associate on its own if the AP drops; the
    // throttled reconnect in maintain() covers cases the driver doesn't.
    WiFi.setAutoReconnect(true);

    configTime(0, 0, NTP_SERVER);
    uint32_t deadline = millis() + NTP_WAIT_MS;
    struct tm t;
    while (millis() < deadline) {
        if (getLocalTime(&t, 100) && (t.tm_year + 1900) > 2024) {
            ntpOk = true;
            return true;
        }
        delay(50);
    }
    return false;
}

void maintain() {
    if (!wifiOk) return;  // never provisioned — nothing to reconnect to

    if (WiFi.status() != WL_CONNECTED) {
        uint32_t now = millis();
        if (now - lastReconnectMs >= WIFI_RECONNECT_INTERVAL_MS) {
            lastReconnectMs = now;
            Serial.println("[wifi] link down, reconnecting");
            WiFi.reconnect();
        }
        return;
    }

    // Connected. If NTP never synced (or hasn't yet after a reconnect), keep
    // polling non-blocking — SNTP re-syncs in the background on its own.
    if (!ntpOk) {
        struct tm t;
        if (getLocalTime(&t, 0) && (t.tm_year + 1900) > 2024) {
            ntpOk = true;
            Serial.println("[ntp] synced");
        }
    }
}

bool wifiConnected() { return wifiOk && WiFi.status() == WL_CONNECTED; }
bool timeValid()     { return ntpOk; }

bool nowUtc(struct tm* out) {
    if (!ntpOk) return false;
    return getLocalTime(out, 50);
}

}  // namespace netclock
