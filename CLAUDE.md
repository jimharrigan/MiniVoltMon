# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**MiniVoltMon** — ESP32-C3 SuperMini firmware that monitors the voltage on one
analog input pin using the chip's **internal ADC** and reports it over WiFi to
the same web page as the sibling `../esp-voltage-monitor` project. It reuses
that project's WiFi provisioning (`netclock`) and HTTP reporting design, the
same rolling-average window, and the same significant-change threshold.

It is headless: no display, SD card, or sensors — just sample, average, and
report on change.

## Build / flash / monitor

PlatformIO project, single env `esp32-c3-supermini`:

```
pio run                                # build
pio run -t upload                      # flash via /dev/ttyACM0 (native USB CDC)
pio device monitor                     # 115200 baud
```

There is no host-side test harness — verification happens on hardware over the
serial monitor. (`pio device monitor` needs a real TTY; to read the port from a
non-interactive shell, use a short pyserial script instead.)

## Architecture

`src/main.cpp` is the orchestrator; each subsystem is a small C++ namespace with
a matching `foo.h` / `foo.cpp`, mirroring esp-voltage-monitor's conventions.

- `adc` — internal ADC. `begin()` sets 12-bit / 11 dB; `readVolts()` returns one
  factory-calibrated reading at `PIN_VOLTAGE_ADC`, scaled by `ADC_DIVIDER_RATIO`
  and offset by `ADC_OFFSET_V`.
- `netclock` (WiFiManager + NTP) — copied from esp-voltage-monitor. `begin(openPortalIfNeeded)`
  provisions WiFi and the **Reporting URL** and **Device name** portal parameters.
  With no stored credentials the portal (AP `MiniVoltMon-Setup`, `192.168.4.1`)
  opens automatically; pulling GPIO0 low at startup forces it open even when
  credentials exist, and keeps it open for as long as GPIO0 is held. NTP is
  carried over from the reference but unused here.
- `reporter` — single-metric HTTP reporter, same mechanism as esp-voltage-monitor:
  an NVS-persisted base URL plus an NVS-persisted device name (both set in the
  portal), to which the query tail `<name>&value=V` is appended, with a
  settle/debounce window and bounded exponential backoff shared across attempts.
  The device name is the reporting key (default `miniVoltMon`); empty input is
  ignored so the key can never become blank.

`main.cpp` samples every `VOLTAGE_SAMPLE_INTERVAL_MS` into a rolling average of
`VOLTAGE_AVG_SAMPLES`, and queues an HTTP send whenever the average moves by at
least `SIG_CHANGE_V` since the last send. All tunables live in `src/config.h`.

## Hardware notes (ESP32-C3 SuperMini)

- Analog input defaults to **GPIO3** (ADC1_CH3). Use an ADC1 pin (GPIO0..4):
  ADC2 is unusable while WiFi is active. GPIO3 is not a strapping pin.
- Config trigger is **GPIO0 (active-low)**, a button to GND. The portal stays
  open for as long as GPIO0 is held low; release it and the device connects with
  whatever creds are stored (or rely on the automatic portal when no creds are
  stored). GPIO0 is *not* a strapping pin on the ESP32-C3, so unlike the BOOT
  button (GPIO9) it can be held across a hardware reset without entering ROM
  download mode.
- The internal ADC reads 0..~3.3 V at the pin. For higher inputs add an external
  resistor divider and set `ADC_DIVIDER_RATIO = (R_top + R_bot) / R_bot`.

## Relationship to esp-voltage-monitor

The WiFi/provisioning (`netclock`) and reporting (`reporter`) code are ported
from `../esp-voltage-monitor` with the multi-channel ADS1115 path replaced by a
single internal-ADC channel. Sample averaging (`VOLTAGE_AVG_SAMPLES`,
`VOLTAGE_SAMPLE_INTERVAL_MS`) and the `SIG_CHANGE_V` threshold are kept identical.
The reported metric key defaults to `miniVoltMon` (vs. that project's
`voltMon_chN`) but is portal-configurable per device via the **Device name**
field (`DEFAULT_DEVICE_NAME` seeds it on first boot).
