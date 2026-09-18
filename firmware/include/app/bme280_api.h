#pragma once

#include <ArduinoJson.h>

// ── BME280 environment sensor (T, P, RH) ───────────────────────────────────
// U3 on the poroGO board: I2C mode (CSB tied to VDD), SDO tied to GND -> 0x76.
// Unlike the BME68x used on CO2Dot there is no gas sensor, so the "env"
// response carries only T, P and RH.

extern bool bme_available;

bool initBME(void);
void cmd_bme_read();
// Fill an existing JsonObject with the BME status fields (no print).
// Used by cmd_bme_status() and by the combined "status" command.
void fill_bme_status(JsonObject out);
void cmd_bme_status();
