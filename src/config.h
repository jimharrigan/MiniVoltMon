#pragma once

// --- Pins (ESP32-C3 SuperMini) ---------------------------------------------
// Analog input is read by the chip's internal ADC. Use an ADC1 pin (GPIO0..4):
// ADC2 is unusable while WiFi is active. GPIO3 = ADC1_CH3, and unlike GPIO2/8/9
// it is not a boot strapping pin, so it is the safe default.
#define PIN_VOLTAGE_ADC    3

// Hold the on-board BOOT button (GPIO9, active-low) at startup to force the
// WiFi config portal open even when credentials are already stored. With no
// stored credentials the portal opens automatically regardless of the button.
#define PIN_CONFIG_BUTTON  9

// --- ADC scaling -----------------------------------------------------------
// The internal ADC reads the voltage at PIN_VOLTAGE_ADC directly (0..~3.3 V
// with 11 dB attenuation). If the input goes through a resistor divider, set
// ADC_DIVIDER_RATIO = (R_top + R_bot) / R_bot. Default 1.0 = measure the pin
// voltage as-is. ADC_OFFSET_V nulls residual error at 0 V.
#define ADC_DIVIDER_RATIO  1.0f
#define ADC_OFFSET_V       0.0f

// --- Sampling / averaging (same as esp-voltage-monitor) --------------------
#define VOLTAGE_AVG_SAMPLES         10    // rolling average window
#define VOLTAGE_SAMPLE_INTERVAL_MS  100   // ~10 Hz sampling
#define SIG_CHANGE_V                0.050f  // report when the average moves this much

// --- Deep sleep ------------------------------------------------------------
// Duty cycle: each wake connects WiFi, takes one reading, reports it, then the
// board deep-sleeps for this long before repeating. The chip resets on wake, so
// every cycle is a fresh boot through setup().
#define DEEP_SLEEP_MINUTES          10
// Safety cap on time spent awake per cycle. If WiFi never connects or the send
// keeps failing, sleep anyway after this long so a stuck cycle can't sit awake
// draining the battery — the reading is simply retried next wake.
#define MAX_AWAKE_MS                60000

// --- WiFi / provisioning (same code as esp-voltage-monitor) ----------------
#define WIFI_AP_NAME                "MiniVoltMon-Setup"
#define WIFI_PORTAL_TIMEOUT_S       30
#define WIFI_RECONNECT_INTERVAL_MS  10000   // throttle reconnect attempts when the link drops
#define NTP_SERVER                  "pool.ntp.org"
#define NTP_WAIT_MS                 5000

// --- HTTP reporter (same mechanism as esp-voltage-monitor) -----------------
// The latest average is sent as a GET to the configured URL with the query
// tail "miniVoltMon&value=V" appended. URL is set via the WiFi portal and
// persisted in NVS.
#define REPORTER_NVS_NAMESPACE      "minivoltmon"
// Seed value written to NVS on first boot when no URL is stored yet (matches the
// esp-voltage-monitor endpoint). The portal can still override it later. Set to
// "" to disable seeding.
#define DEFAULT_REPORT_URL          "http://jimharrigan.com/kvp/set?key="
// Device name = the reporting key appended to the URL (".../set?key=NAME&value=V").
// Set via the WiFi portal and persisted in NVS; this is the first-boot seed.
#define DEFAULT_DEVICE_NAME         "miniVoltMon"
// Max length of the device name (portal field width and on-stack buffer sizing).
#define DEVICE_NAME_MAX_LEN         32
#define HTTP_TIMEOUT_MS             2000
#define RETRY_INITIAL_MS            2000
#define RETRY_MAX_MS                60000
// Debounce: only send once the value has been stable this long. Each new
// significant change re-arms the timer, so a continuous change sends a single
// value once it settles rather than flooding the server.
#define REPORT_DEBOUNCE_MS          1000
