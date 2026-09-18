#include "app/pump_api.h"
#include "app/response.h"
#include <driver/gpio.h>

// LEDC channel/timer. Channel 0 is otherwise unused in this firmware.
static constexpr uint8_t  kPumpLedcChannel = 0;
// 80 MHz APB clock / 20 kHz = 4000 ticks per period, so 11 bits (2048) is the
// most the timer supports; 10 bits leaves margin and gives ~0.1 % steps.
static constexpr uint8_t  kPumpLedcBits    = 10;
static constexpr uint32_t kPumpLedcMax     = (1u << kPumpLedcBits) - 1;

static float s_duty_pct = 0.0f;

// Electrical polarity is inverted: the PWM is low-pass filtered and injected
// into the MT3608 FB node, so a HIGH pin pulls the boost output DOWN. Pump 0 %
// therefore means the pin is held solidly high (boost at its floor, pump off)
// and pump 100 % means the pin is held low (boost at its maximum).
static uint32_t pumpTicksForDuty(float duty_pct) {
  const uint32_t on_ticks = static_cast<uint32_t>(duty_pct * kPumpLedcMax / 100.0f + 0.5f);
  return kPumpLedcMax - on_ticks;   // ticks the pin spends HIGH
}

void initPump() {
  // Park the pin high immediately so the boost sits at its minimum during the
  // rest of start-up. Also enable the weak pull-up so the line still reads high
  // if the output stage is ever released (e.g. bootloader, reset).
  pinMode(kPumpPin, OUTPUT);
  digitalWrite(kPumpPin, HIGH);
  ledcSetup(kPumpLedcChannel, kPumpPwmHz, kPumpLedcBits);
  ledcAttachPin(kPumpPin, kPumpLedcChannel);
  ledcWrite(kPumpLedcChannel, pumpTicksForDuty(0.0f));   // = full HIGH
  gpio_pullup_en(static_cast<gpio_num_t>(kPumpPin));
  s_duty_pct = 0.0f;
}

float pump_set_duty(float duty_pct) {
  if (!(duty_pct >= 0.0f)) duty_pct = 0.0f;   // also catches NaN
  if (duty_pct > 100.0f)   duty_pct = 100.0f;

  const uint32_t high_ticks = pumpTicksForDuty(duty_pct);
  ledcWrite(kPumpLedcChannel, high_ticks);

  s_duty_pct = (kPumpLedcMax - high_ticks) * 100.0f / kPumpLedcMax;
  return s_duty_pct;
}

float pump_get_duty() { return s_duty_pct; }

void fill_pump_status(JsonObject o) {
  o["duty_pct"]     = s_duty_pct;            // pump drive, 0 = off
  o["pin_high_pct"] = 100.0f - s_duty_pct;   // electrical duty on GPIO21
  o["freq_hz"]      = kPumpPwmHz;
  o["pin"]      = kPumpPin;
}

void cmd_pump(const String &argIn) {
  String arg = argIn;
  arg.trim();

  JsonDocument doc;
  if (arg.length() > 0) {
    // Accept "12", "12.5", "12%"; reject anything that is not a number.
    char *end = nullptr;
    const float pct = strtof(arg.c_str(), &end);
    if (end == arg.c_str() || (*end != '\0' && *end != '%')) {
      doc["error"] = "invalid_duty";
      doc["path"]  = g_requestFull;
      respond(doc);
      return;
    }
    pump_set_duty(pct);
  }
  fill_pump_status(doc.to<JsonObject>());
  respond(doc);
}
