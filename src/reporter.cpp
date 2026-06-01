#include "reporter.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>

#include "config.h"

namespace reporter {

// URL key sent for the metric. Appears as its own line on the shared web page.
static const char* const METRIC_KEY = "miniVoltMon";

static Preferences prefs;
static String g_url;

// Pending send. Latest value wins if the metric changes again before its
// previous send succeeds. g_settleMs is the earliest time it may be sent; every
// new change pushes it out by REPORT_DEBOUNCE_MS, so a value is only sent once
// it has stopped changing.
static bool     g_pending   = false;
static float    g_pendingV  = 0;
static uint32_t g_settleMs  = 0;

// Bounded backoff: on failure the next attempt waits a doubling window; on
// success it resets so the next change fires immediately.
static uint32_t g_nextRetryMs      = 0;
static uint32_t g_currentBackoffMs = RETRY_INITIAL_MS;

void begin() {
    prefs.begin(REPORTER_NVS_NAMESPACE, false);
    g_url = prefs.getString("url", "");
    if (g_url.length() == 0 && strlen(DEFAULT_REPORT_URL) > 0) {
        g_url = DEFAULT_REPORT_URL;       // first-boot seed; portal can override
        prefs.putString("url", g_url);
        Serial.printf("[reporter] seeded url from default\n");
    }
    Serial.printf("[reporter] url='%s'\n", g_url.c_str());
}

const char* url() { return g_url.c_str(); }

void setUrl(const char* newUrl) {
    String s(newUrl ? newUrl : "");
    if (s == g_url) return;
    g_url = s;
    prefs.putString("url", g_url);
    Serial.printf("[reporter] url updated: %s\n", g_url.c_str());
}

void queueMetric(float value) {
    g_pendingV = value;                       // latest value wins
    g_pending  = true;
    g_settleMs = millis() + REPORT_DEBOUNCE_MS;  // re-arm debounce
}

void tick() {
    uint32_t now = millis();

    if (!g_pending) return;
    if ((int32_t)(now - g_settleMs) < 0) return;    // still settling
    if ((int32_t)(now - g_nextRetryMs) < 0) return;  // backing off

    if (WiFi.status() != WL_CONNECTED) {
        g_nextRetryMs = now + g_currentBackoffMs;
        g_currentBackoffMs = min(g_currentBackoffMs * 2, (uint32_t)RETRY_MAX_MS);
        return;
    }

    if (g_url.length() == 0) {
        Serial.println("[reporter] no URL configured; dropping pending send");
        g_pending = false;
        return;
    }

    // The configured URL is expected to end with the key prefix (e.g.
    // ".../set?key="). Append "miniVoltMon&value=V".
    char tail[64];
    snprintf(tail, sizeof(tail), "%s&value=%.3f", METRIC_KEY, g_pendingV);
    String url = g_url + tail;

    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.setConnectTimeout(HTTP_TIMEOUT_MS);
    if (!http.begin(url)) {
        Serial.println("[reporter] HTTP begin() failed");
        g_nextRetryMs = now + g_currentBackoffMs;
        g_currentBackoffMs = min(g_currentBackoffMs * 2, (uint32_t)RETRY_MAX_MS);
        return;
    }
    int code = http.GET();
    http.end();
    Serial.printf("[reporter] GET %s -> %d\n", url.c_str(), code);

    if (code >= 200 && code < 300) {
        g_pending = false;
        g_currentBackoffMs = RETRY_INITIAL_MS;
        g_nextRetryMs = now;
    } else {
        g_nextRetryMs = now + g_currentBackoffMs;
        g_currentBackoffMs = min(g_currentBackoffMs * 2, (uint32_t)RETRY_MAX_MS);
        Serial.printf("[reporter] retry in %u ms\n", g_currentBackoffMs);
    }
}

}  // namespace reporter
