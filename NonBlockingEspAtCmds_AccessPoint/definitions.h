#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include "Arduino.h"

#define ESP_PORT Serial1 // For espAt.h.
#define ESP_BAUD 115200 // For espAt.h.
// #define ESP_PORT Serial // For espAt.h.

#define DBG_ALL
#ifdef DBG_ALL
#define DBG_PORT Serial
#define DBG_BAUD 9600
#endif

// Max of 30 chars for SSID and pw.
// #define ESP_AT_AP_SSID "Your AP SSID name here"
// #define ESP_AT_AP_PW "Your AP password here"

#ifndef ESP_AT_AP_SSID
#error "Define access point SSID"
#endif
#ifndef ESP_AT_AP_PW
#error "Define access point password"
#endif

#define ESP_AT_AP_VERSION_WILLRETRY_FOREVER // Choose one only.
// #define ESP_AT_AP_VERSION_WILLRETRY_N // Choose one only.

#ifdef ESP_AT_AP_VERSION_WILLRETRY_N
#undef ESP_AT_AP_VERSION_WILLRETRY_FOREVER
const uint8_t ESP_AT_AP_QTY_RETRIES = 3; // The number of retries after a timeout.
#endif

const uint8_t PIN_OUT_LED_HB = 13;

#endif
