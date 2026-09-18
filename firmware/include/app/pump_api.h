#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// ── Piezo pump drive ────────────────────────────────────────────────────────
// GPIO21 drives the current injector of the boost converter with a fixed
// 20 kHz PWM. The filtered PWM level is injected into the MT3608 FB node, so a
// HIGH pin LOWERS the boost output. The duty cycle exposed here (0-100 %) is
// the pump drive: 0 % = pin held high = boost at its floor = pump off;
// 100 % = pin held low = boost at its maximum. Boot state is 0 %.

constexpr uint8_t  kPumpPin      = 21;
constexpr uint32_t kPumpPwmHz    = 20000;

// Configure the LEDC channel and park the output at 0 % (pin high).
// Call this FIRST in setup() so the boost is at its floor as early as possible.
void initPump();

// Set the duty cycle in percent. Values are clamped to 0..100.
// Returns the duty actually applied (after clamping and quantisation).
float pump_set_duty(float duty_pct);

// Current duty cycle in percent.
float pump_get_duty();

// Fill {duty_pct, freq_hz, pin} into `o` (shared by `pump` and `status`).
void fill_pump_status(JsonObject o);

// Command handler. `arg` is the text after "pump" (may be empty). With no
// argument it reports the current state; with one it sets the duty cycle.
void cmd_pump(const String &arg);
