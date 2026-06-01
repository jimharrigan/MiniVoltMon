#pragma once

namespace reporter {

// Single-metric HTTP reporter. Shares the esp-voltage-monitor mechanism:
// an NVS-persisted base URL (set via the WiFi portal) to which a query tail
// "miniVoltMon&value=V" is appended, with debounce and bounded backoff.

void begin();                       // load URL from NVS
const char* url();                  // current URL ("" if unset)
void setUrl(const char* newUrl);    // persist URL if changed
void queueMetric(float value);      // mark the latest reading for HTTP send
void tick();                        // drive HTTP send with bounded backoff

}  // namespace reporter
