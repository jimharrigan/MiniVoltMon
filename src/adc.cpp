#include "adc.h"

#include <Arduino.h>

#include "config.h"

namespace adc {

bool begin() {
    analogReadResolution(12);          // 0..4095
    analogSetAttenuation(ADC_11db);    // full ~0..3.3 V range; mV reads are factory-calibrated
    return true;
}

float readVolts() {
    // Single calibrated read; the rolling average in main.cpp (VOLTAGE_AVG_SAMPLES)
    // smooths noise, mirroring the reference project's averaging.
    float mv = analogReadMilliVolts(PIN_VOLTAGE_ADC);
    return mv * ADC_DIVIDER_RATIO / 1000.0f - ADC_OFFSET_V;
}

}  // namespace adc
