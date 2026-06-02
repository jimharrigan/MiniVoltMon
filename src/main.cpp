#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>

#include "adc.h"
#include "config.h"
#include "netclock.h"
#include "reporter.h"

// Wall-clock instant this wake began, used to bound how long we stay awake.
static uint32_t bootMs = 0;

// Rolling average over VOLTAGE_AVG_SAMPLES, same window as esp-voltage-monitor.
// Here it just smooths the one reading we take per wake before deep sleep.
static float voltsHistory[VOLTAGE_AVG_SAMPLES];
static uint8_t voltsHistoryCount = 0;
static uint8_t voltsHistoryHead = 0;

static float pushSample(float v) {
    voltsHistory[voltsHistoryHead] = v;
    voltsHistoryHead = (voltsHistoryHead + 1) % VOLTAGE_AVG_SAMPLES;
    if (voltsHistoryCount < VOLTAGE_AVG_SAMPLES) voltsHistoryCount++;
    float sum = 0;
    for (int k = 0; k < voltsHistoryCount; k++) sum += voltsHistory[k];
    return sum / voltsHistoryCount;
}

// Power down and wake after DEEP_SLEEP_MINUTES to run the next cycle from scratch.
static void deepSleep() {
    Serial.printf("[sleep] deep sleep for %d min\n", DEEP_SLEEP_MINUTES);
    Serial.flush();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    esp_sleep_enable_timer_wakeup((uint64_t)DEEP_SLEEP_MINUTES * 60ULL * 1000000ULL);
    esp_deep_sleep_start();   // does not return; the chip resets on wake
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n[boot] MiniVoltMon");
    bootMs = millis();

    adc::begin();
    reporter::begin();

    bool wantPortal = netclock::configRequested();
    Serial.println(wantPortal ? "[wifi] config portal..." : "[wifi] connect...");
    netclock::begin(wantPortal);
    Serial.printf("[wifi] %s\n", netclock::wifiConnected() ? "ok" : "no");

    // Take one averaged reading and queue it for this wake's single report.
    float avg = 0;
    for (int i = 0; i < VOLTAGE_AVG_SAMPLES; i++) avg = pushSample(adc::readVolts());
    reporter::queueMetric(avg);
    Serial.printf("[adc] reading %.3f V\n", avg);
}

void loop() {
    netclock::maintain();
    reporter::tick();

    // Once the reading has been reported, sleep until the next cycle. Also sleep
    // if we've been awake too long (WiFi down or repeated send failures) so the
    // board doesn't burn battery stuck awake — it retries on the next wake.
    if (!reporter::pending()) {
        deepSleep();
    } else if ((millis() - bootMs) >= MAX_AWAKE_MS) {
        Serial.println("[sleep] awake too long; sleeping without a successful report");
        deepSleep();
    }
}
