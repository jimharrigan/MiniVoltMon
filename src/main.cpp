#include <Arduino.h>
#include <math.h>

#include "adc.h"
#include "config.h"
#include "netclock.h"
#include "reporter.h"

static uint32_t lastAdcSampleMs = 0;
static float lastReportedV = NAN;   // last average queued for HTTP send

// Rolling average over VOLTAGE_AVG_SAMPLES, same window as esp-voltage-monitor.
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

// Queue a send when the average moved by >= SIG_CHANGE_V since the last send.
static void detectChange(float avg) {
    if (isnan(lastReportedV) || fabsf(avg - lastReportedV) >= SIG_CHANGE_V) {
        reporter::queueMetric(avg);
        lastReportedV = avg;
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n[boot] MiniVoltMon");

    adc::begin();
    reporter::begin();

    bool wantPortal = netclock::configRequested();
    Serial.println(wantPortal ? "[wifi] config portal..." : "[wifi] connect...");
    netclock::begin(wantPortal);
    Serial.printf("[wifi] %s\n", netclock::wifiConnected() ? "ok" : "no");

    // Prime the rolling average and send the first reading.
    float avg = 0;
    for (int i = 0; i < VOLTAGE_AVG_SAMPLES; i++) avg = pushSample(adc::readVolts());
    lastReportedV = avg;
    reporter::queueMetric(avg);
    Serial.printf("[adc] initial %.3f V\n", avg);

    lastAdcSampleMs = millis();
}

void loop() {
    uint32_t now = millis();

    if ((now - lastAdcSampleMs) >= VOLTAGE_SAMPLE_INTERVAL_MS) {
        float avg = pushSample(adc::readVolts());
        lastAdcSampleMs = now;
        detectChange(avg);   // queues an HTTP send on a significant move
    }

    netclock::maintain();
    reporter::tick();
}
