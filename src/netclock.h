#pragma once

#include <time.h>
#include <stdbool.h>

namespace netclock {

bool begin(bool openPortalIfNeeded);  // portal only opens when arg is true; otherwise saved creds only
void maintain();          // call each loop; reconnects WiFi if the link dropped
bool configRequested();   // PIN_CONFIG_BUTTON (BOOT) held low at boot
bool wifiConnected();
bool timeValid();
bool nowUtc(struct tm* out);

}  // namespace netclock
