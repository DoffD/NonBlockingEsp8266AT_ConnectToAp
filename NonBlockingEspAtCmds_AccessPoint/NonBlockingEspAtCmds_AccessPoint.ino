#include "definitions.h"
#include "espAt.h"
#include "espAtAp.h"

void setup() {
  // put your setup code here, to run once:
  #ifdef DBG_ALL
  DBG_PORT.begin(DBG_BAUD);
  DBG_PORT.println(F("Non-blocking ESP8266 AT Commands Connection to an Access Point Test!"));
  #endif

  ESP_PORT.begin(ESP_BAUD);

  pinMode(PIN_OUT_LED_HB, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t tRefStConn{};
  static uint8_t st{};
  switch (st)
  {
    case 0: // Initialize connection to AP.
      espAt_ap_connectToAp((char*)ESP_AT_AP_SSID, (char*)ESP_AT_AP_PW);
      tRefStConn = millis();
      st = 1;
      break;
    case 1: // Wait for the result or timeout.
    {
      EspAtApResult result = espAt_ap_getResult();
      switch (result)
      {
        case EspAtApResult::connected:
          st = 2;
          #ifdef DBG_ALL
          DBG_PORT.println(F("Connected to AP!"));
          #endif
          break;
        case EspAtApResult::failedReset:
          #ifndef ESP_AT_AP_VERSION_WILLRETRY_FOREVER
          st = 2;
          #endif
          #ifdef DBG_ALL
          DBG_PORT.println(F("Reset command failed!"));
          #endif
          break;
        case EspAtApResult::failedStationMode:
          #ifndef ESP_AT_AP_VERSION_WILLRETRY_FOREVER
          st = 2;
          #endif
          #ifdef DBG_ALL
          DBG_PORT.println(F("Station mode command failed!"));
          #endif
          break;
        case EspAtApResult::failedConnectToAp:
          #ifndef ESP_AT_AP_VERSION_WILLRETRY_FOREVER
          st = 2;
          #endif
          #ifdef DBG_ALL
          DBG_PORT.println(F("Connect to AP command failed!"));
          #endif
          break;
        case EspAtApResult::invalidApCredentials:
          st = 2;
          #ifdef DBG_ALL
          DBG_PORT.println(F("Invalid AP credentials!"));
          #endif
          break;
        default:
          #ifndef ESP_AT_AP_VERSION_WILLRETRY_FOREVER
          if(millis() - tRefStConn > 30000){ // Allow at least 30 seconds to connect.
            st = 2;
            #ifdef DBG_ALL
            DBG_PORT.println(F("Timeout!"));
            #endif
          }
          #endif
          break;
      }
    }
      break;
    case 2: // Done
      // Do nothing.
      break;
    default:
      break;
  }
  // ESP8266 AT AP routines start ====================
  espAt_ap_loop();
  // ESP8266 AT AP routines end   ====================

  // Blink LED routine start ====================
  static uint32_t hb_tRef{};
  if(millis() - hb_tRef > 500){
    digitalWrite(PIN_OUT_LED_HB, !digitalRead(PIN_OUT_LED_HB));
    hb_tRef = millis();
  }
  // Blink LED routine end   ====================
}
