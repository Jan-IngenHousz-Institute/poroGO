#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include "app/bme280_api.h"
#include "app/commands.h"
#include "app/response.h"

static constexpr uint8_t BME280_ADDR = 0x76;  // SDO -> GND on the board
static Adafruit_BME280 bme;                   // I2C

// Fills out with the BME reading {T,P,RH}. Floats use serialized() so the
// emitted precision is fixed (2 decimals, matching the historical output).
static void fill_bme_values(JsonObject out, float t_c, float p_pa, float rh) {
  out["T"]  = serialized(String(t_c, 2));          // °C
  out["P"]  = serialized(String(p_pa / 100.0f, 2)); // hPa
  out["RH"] = serialized(String(rh, 2));           // %RH
}

bool initBME(void) {
  if (!bme.begin(BME280_ADDR, &Wire)) {
#if DEBUG
    Serial.print(F("BME280 init failed, sensor ID 0x"));
    Serial.println(bme.sensorID(), HEX);
#endif
    return false;
  }
  // Forced mode: one conversion per takeForcedMeasurement(), sensor sleeps in
  // between (lowest self-heating). Oversampling mirrors the old BME68x setup.
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X2,   // temperature
                  Adafruit_BME280::SAMPLING_X4,   // pressure
                  Adafruit_BME280::SAMPLING_X2,   // humidity
                  Adafruit_BME280::FILTER_OFF);
  return true;
}

void cmd_bme_read() {
  JsonDocument doc;
  if (!bme_available) {
    doc["error"] = "not_available";
    respond(doc);
    return;
  }
  // Blocks until the forced conversion completes (a few ms at this oversampling).
  if (!bme.takeForcedMeasurement()) {
    doc["error"] = "read_failed";
    respond(doc);
    return;
  }
  const float t  = bme.readTemperature();
  const float p  = bme.readPressure();
  const float rh = bme.readHumidity();
  if (isnan(t) || isnan(p) || isnan(rh)) {
    doc["error"] = "read_failed";
    respond(doc);
    return;
  }
  fill_bme_values(doc.to<JsonObject>(), t, p, rh);
  respond(doc);
}

void fill_bme_status(JsonObject out) {
  out["available"] = bme_available;
  out["sensor"]    = "BME280";
  out["addr"]      = "0x76";
}

void cmd_bme_status() {
  JsonDocument doc;
  fill_bme_status(doc.to<JsonObject>());
  respond(doc);
}
