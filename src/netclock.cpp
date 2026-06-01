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
    // BOOT button on the ESP32-C3 SuperMini is GPIO9, pulled high and driven
    // low while pressed. Hold it at startup to force the config portal open.
    pinMode(PIN_CONFIG_BUTTON, INPUT_PULLUP);
    delayMicroseconds(50);
    return digitalRead(PIN_CONFIG_BUTTON) == LOW;
}

bool begin(bool openPortalIfNeeded) {
    WiFiManager wm;

    WiFiManagerParameter urlParam("url", "Reporting URL", reporter::url(), 256);
    wm.addParameter(&urlParam);

    // Persist URL the moment the form is submitted (covers /wifisave when the
    // user enters URL + WiFi creds together, and /paramsave for URL-only
    // updates). Without this, a reset before STA actually associates would
    // lose an in-memory-only URL value.
    auto persistUrl = [&urlParam]() { reporter::setUrl(urlParam.getValue()); };
    wm.setSaveConfigCallback(persistUrl);
    wm.setSaveParamsCallback(persistUrl);

    if (!openPortalIfNeeded) {
        wm.setEnableConfigPortal(true);   // open portal automatically if saved creds fail
        wm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT_S);
        wifiOk = wm.autoConnect(WIFI_AP_NAME);
    } else {
        // Portal stays up until the user connects via it (or the device is
        // reset). No timeout — non-blocking only so we can poll WiFi state.
        wm.setConfigPortalBlocking(false);
        wm.setConfigPortalTimeout(0);
        wm.startConfigPortal(WIFI_AP_NAME);

        while (WiFi.status() != WL_CONNECTED) {
            wm.process();
            delay(10);
        }
        wm.stopConfigPortal();
        wifiOk = true;
        reporter::setUrl(urlParam.getValue());
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
