#pragma once

namespace reporter {

// Single-metric HTTP reporter. Shares the esp-voltage-monitor mechanism:
// an NVS-persisted base URL (set via the WiFi portal) to which a query tail
// "<name>&value=V" is appended, with debounce and bounded backoff. The metric
// key <name> (the device name) is also portal-configurable and NVS-persisted.

void begin();                       // load URL and device name from NVS
const char* url();                  // current URL ("" if unset)
void setUrl(const char* newUrl);    // persist URL if changed
const char* name();                 // current device name / metric key
void setName(const char* newName);  // persist device name if changed
void queueMetric(float value);      // mark the latest reading for HTTP send
void tick();                        // drive HTTP send with bounded backoff
bool pending();                     // true while a queued send has not yet succeeded

}  // namespace reporter
