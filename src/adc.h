#pragma once

namespace adc {

bool begin();        // configure the internal ADC
float readVolts();   // one calibrated reading at PIN_VOLTAGE_ADC, in volts

}  // namespace adc
