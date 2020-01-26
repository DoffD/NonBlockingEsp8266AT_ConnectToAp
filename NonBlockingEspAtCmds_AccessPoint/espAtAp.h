#ifndef ESP_AT_AP_H
#define ESP_AT_AP_H

#include "definitions.h"
#include "espAt.h"

#define ESP_AT_AP_SIZE_SSID 30
#define ESP_AT_AP_SIZE_PASSWORD 30

#ifdef DBG_ALL
// #define DBG_ESP_AT_AP_SM
#endif

enum class EspAtApResult:uint8_t{
  none,
  invalidApCredentials,
  connected,
  failedReset,
  failedStationMode,
  failedConnectToAp
};
static EspAtApResult espAt_ap_result{};

static char esp_ap_ssid[ESP_AT_AP_SIZE_SSID+1]{};
static char esp_ap_password[ESP_AT_AP_SIZE_PASSWORD+1]{};

void espAt_ap_connectToAp(char * ssid, char * pw){
  uint8_t i{};
  while(ssid[i] && i<ESP_AT_AP_SIZE_SSID){
    esp_ap_ssid[i] = ssid[i];
    i++;
  }
  uint8_t s{};
  while(pw[s] && s<ESP_AT_AP_SIZE_PASSWORD){
    esp_ap_password[s] = pw[s];
    s++;
  }
  // Clear ssid if ssid string ptr or pw string ptr is null (don't proceed).
  if(i == 0 || s == 0){
    memset(esp_ap_ssid, 0, sizeof(esp_ap_ssid));
    memset(esp_ap_password, 0, sizeof(esp_ap_password));
    espAt_ap_result = EspAtApResult::invalidApCredentials;
    #ifdef DBG_ESP_AT_AP_SM
    DBG_PORT.println(F("espApSt: invalid ap credentials"));
    #endif
  }
}

// This function returns the latest result.
// If result != none, it is reset to none.
// Hence, user must buffer the return value since the internal variable is cleared after being read.
EspAtApResult espAt_ap_getResult(){
  EspAtApResult result{};
  if(espAt_ap_result != EspAtApResult::none){
    result = espAt_ap_result;
    espAt_ap_result = EspAtApResult::none;
  }
  return result;
}

void espAt_ap_loop(){
  // ESP8266 AP routines start ====================
  static char ssid[ESP_AT_AP_SIZE_SSID+1]{};
  static char password[ESP_AT_AP_SIZE_PASSWORD+1]{};

  enum class EspApSt:uint8_t{
    init,
    waiting,
    reset,
    stationMode,
    connectToAp,
    waitForResult
  };
  static EspApSt espApSt{}, espApStPrev{};
  switch (espApSt)
  {
    case EspApSt::init:
      espApSt = EspApSt::waiting;
      #ifdef DBG_ESP_AT_AP_SM
      DBG_PORT.println(F("espApSt: init > waiting"));
      #endif
      break;
    case EspApSt::waiting:
    {
      char testBlock[ESP_AT_AP_SIZE_SSID]{};
      if(memcmp(esp_ap_ssid, testBlock, sizeof(testBlock))){
        memset(ssid, 0, sizeof(ssid));
        memset(password, 0, sizeof(password));
        memcpy(ssid, esp_ap_ssid, sizeof(ssid));
        memcpy(password, esp_ap_password, sizeof(password));

        // Clear global variables start =====
        memset(esp_ap_ssid, 0, sizeof(esp_ap_ssid));
        memset(esp_ap_password, 0, sizeof(esp_ap_password));
        // Clear global variables end   =====
        espApSt = EspApSt::reset;
        #ifdef DBG_ESP_AT_AP_SM
        DBG_PORT.println(F("espApSt: waiting > reset"));
        #endif
      }

    }
      break;
    case EspApSt::reset:
      espAt_commander((char*)"AT+RST\r\n", 3000, (char*)"eady");
      // espAt_commander((char*)"AT+RST\r\n", 3000, (char*)"xx");
      espApStPrev = EspApSt::reset;
      espApSt = EspApSt::waitForResult;
      #ifdef DBG_ESP_AT_AP_SM
      DBG_PORT.println(F("espApSt: reset > waitForResult"));
      #endif
      break;
    case EspApSt::stationMode:
      espAt_commander((char*)"AT+CWMODE=1\r\n", 2000, (char*)"OK");
      // espAt_commander((char*)"AT+CWMODE=1\r\n", 2000, (char*)"XX");
      espApStPrev = EspApSt::stationMode;
      espApSt = EspApSt::waitForResult;
      #ifdef DBG_ESP_AT_AP_SM
      DBG_PORT.println(F("espApSt: stationMode > waitForResult"));
      #endif
      break;
    case EspApSt::connectToAp:
    {
      char bfr[100+1]{};
      sprintf(bfr,"AT+CWJAP=\"%s\",\"%s\"\r\n",ssid, password);
      espAt_commander(bfr, 20000, (char*)ssid);
      // espAt_commander(bfr, 5000, (char*)"xx");
      espApStPrev = EspApSt::connectToAp;
      espApSt = EspApSt::waitForResult;
      #ifdef DBG_ESP_AT_AP_SM
      DBG_PORT.println(F("espApSt: connectToAp > waitForResult"));
      #endif
    }
      break;
    case EspApSt::waitForResult:
    {
      // NB: Buffer the return value of espAt_getResult() since the internal variable is cleared after being read.
      EspAtResult espAtResult = espAt_getResult();
      switch (espAtResult)
      {
        case EspAtResult::searchOk:
        {
          if(espApStPrev == EspApSt::reset){
            espApSt = EspApSt::stationMode;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: reply rcvd, ESP reset!"));
            DBG_PORT.println(F("  : waitForResult > stationMode"));
            #endif
          }
          else if(espApStPrev == EspApSt::stationMode){
            espApSt = EspApSt::connectToAp;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: reply rcvd, ESP in station mode."));
            DBG_PORT.println(F("  : waitForResult > connectToAp"));
            #endif
          }
          else if(espApStPrev == EspApSt::connectToAp){
            espAt_ap_result = EspAtApResult::connected;
            espApSt = EspApSt::waiting;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: reply rcvd, connected to AP!"));
            DBG_PORT.println(F("  : waitForResult > waiting"));
            #endif
          }
        }
          break;
        case EspAtResult::timeout:
        {
          if(espApStPrev == EspApSt::reset){
            #ifdef ESP_AT_AP_VERSION_WILLRETRY_FOREVER
            espApSt = EspApSt::reset;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: timeout!"));
            DBG_PORT.println(F("  : waitForResult > reset"));
            #endif
            #elif defined ESP_AT_AP_VERSION_WILLRETRY_N
            static uint8_t retryCnt{};
            if(retryCnt < ESP_AT_AP_QTY_RETRIES){
              retryCnt++;
              espApSt = EspApSt::reset;
              #ifdef DBG_ESP_AT_AP_SM
              DBG_PORT.println(F("espApSt: timeout!"));
              DBG_PORT.println(F("  : waitForResult > reset"));
              #endif
            }
            else{
              espAt_ap_result = EspAtApResult::failedReset;
              uint8_t attempts = retryCnt;
              retryCnt = 0;
              espApSt = EspApSt::waiting;
              #ifdef DBG_ESP_AT_AP_SM
              DBG_PORT.println(F("espApSt: timeout!"));
              DBG_PORT.print(F("  : retries: ")); DBG_PORT.println(attempts);
              DBG_PORT.println(F("  : waitForResult > waiting"));
              #endif
            }
            #else
            espAt_ap_result = EspAtApResult::failedReset;
            espApSt = EspApSt::waiting;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: timeout!"));
            DBG_PORT.println(F("  : waitForResult > waiting"));
            #endif
            #endif
          }
          else if(espApStPrev == EspApSt::stationMode){
            #ifdef ESP_AT_AP_VERSION_WILLRETRY_FOREVER
            espApSt = EspApSt::stationMode;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: timeout!"));
            DBG_PORT.println(F("  : waitForResult > stationMode"));
            #endif
            #elif defined ESP_AT_AP_VERSION_WILLRETRY_N
            static uint8_t retryCnt{};
            if(retryCnt < ESP_AT_AP_QTY_RETRIES){
              retryCnt++;
              espApSt = EspApSt::stationMode;
              #ifdef DBG_ESP_AT_AP_SM
              DBG_PORT.println(F("espApSt: timeout!"));
              DBG_PORT.println(F("  : waitForResult > stationMode"));
              #endif
            }
            else{
              espAt_ap_result = EspAtApResult::failedStationMode;
              uint8_t attempts = retryCnt;
              retryCnt = 0;
              espApSt = EspApSt::waiting;
              #ifdef DBG_ESP_AT_AP_SM
              DBG_PORT.println(F("espApSt: timeout!"));
              DBG_PORT.print(F("  : retries: ")); DBG_PORT.println(attempts);
              DBG_PORT.println(F("  : waitForResult > waiting"));
              #endif
            }
            #else
            espAt_ap_result = EspAtApResult::failedStationMode;
            espApSt = EspApSt::waiting;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: timeout!"));
            DBG_PORT.println(F("  : waitForResult > waiting"));
            #endif
            #endif
          }
          else if(espApStPrev == EspApSt::connectToAp){
            #ifdef ESP_AT_AP_VERSION_WILLRETRY_FOREVER
            espApSt = EspApSt::connectToAp;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: timeout!"));
            DBG_PORT.println(F("  : waitForResult > connectToAp"));
            #endif
            #elif defined ESP_AT_AP_VERSION_WILLRETRY_N
            static uint8_t retryCnt{};
            if(retryCnt < ESP_AT_AP_QTY_RETRIES){
              retryCnt++;
              espApSt = EspApSt::connectToAp;
              #ifdef DBG_ESP_AT_AP_SM
              DBG_PORT.println(F("espApSt: timeout!"));
              DBG_PORT.println(F("  : waitForResult > connectToAp"));
              #endif
            }
            else{
              espAt_ap_result = EspAtApResult::failedConnectToAp;
              uint8_t attempts = retryCnt;
              retryCnt = 0;
              espApSt = EspApSt::waiting;
              #ifdef DBG_ESP_AT_AP_SM
              DBG_PORT.println(F("espApSt: timeout!"));
              DBG_PORT.print(F("  : retries: ")); DBG_PORT.println(attempts);
              DBG_PORT.println(F("  : waitForResult > waiting"));
              #endif
            }
            #else
            espAt_ap_result = EspAtApResult::failedConnectToAp;
            espApSt = EspApSt::waiting;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: timeout!"));
            DBG_PORT.println(F("  : waitForResult > waiting"));
            #endif
            #endif
          }
        }
          break;
        case EspAtResult::overflow:
        {
          if(espApStPrev == EspApSt::reset){
            #ifdef ESP_AT_AP_VERSION_WILLRETRY_FOREVER
            espApSt = EspApSt::reset;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: overflow!"));
            DBG_PORT.println(F("  : waitForResult > reset"));
            #endif
            #elif defined ESP_AT_AP_VERSION_WILLRETRY_N
            static uint8_t retryCnt{};
            if(retryCnt < ESP_AT_AP_QTY_RETRIES){
              retryCnt++;
              espApSt = EspApSt::reset;
              #ifdef DBG_ESP_AT_AP_SM
              DBG_PORT.println(F("espApSt: overflow!"));
              DBG_PORT.println(F("  : waitForResult > reset"));
              #endif
            }
            else{
              espAt_ap_result = EspAtApResult::failedReset;
              uint8_t attempts = retryCnt;
              retryCnt = 0;
              espApSt = EspApSt::waiting;
              #ifdef DBG_ESP_AT_AP_SM
              DBG_PORT.println(F("espApSt: overflow!"));
              DBG_PORT.print(F("  : retries: ")); DBG_PORT.println(attempts);
              DBG_PORT.println(F("  : waitForResult > waiting"));
              #endif
            }
            #else
            espAt_ap_result = EspAtApResult::failedReset;
            espApSt = EspApSt::waiting;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: overflow!"));
            DBG_PORT.println(F("  : waitForResult > waiting"));
            #endif
            #endif
          }
          else if(espApStPrev == EspApSt::stationMode){
            #ifdef ESP_AT_AP_VERSION_WILLRETRY_FOREVER
            espApSt = EspApSt::stationMode;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: overflow!"));
            DBG_PORT.println(F("  : waitForResult > stationMode"));
            #endif
            #elif defined ESP_AT_AP_VERSION_WILLRETRY_N
            static uint8_t retryCnt{};
            if(retryCnt < ESP_AT_AP_QTY_RETRIES){
              retryCnt++;
              espApSt = EspApSt::stationMode;
              #ifdef DBG_ESP_AT_AP_SM
              DBG_PORT.println(F("espApSt: overflow!"));
              DBG_PORT.println(F("  : waitForResult > stationMode"));
              #endif
            }
            else{
              espAt_ap_result = EspAtApResult::failedStationMode;
              uint8_t attempts = retryCnt;
              retryCnt = 0;
              espApSt = EspApSt::waiting;
              #ifdef DBG_ESP_AT_AP_SM
              DBG_PORT.println(F("espApSt: overflow!"));
              DBG_PORT.print(F("  : retries: ")); DBG_PORT.println(attempts);
              DBG_PORT.println(F("  : waitForResult > waiting"));
              #endif
            }
            #else
            espAt_ap_result = EspAtApResult::failedStationMode;
            espApSt = EspApSt::waiting;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: overflow!"));
            DBG_PORT.println(F("  : waitForResult > waiting"));
            #endif
            #endif
          }
          else if(espApStPrev == EspApSt::connectToAp){
            #ifdef ESP_AT_AP_VERSION_WILLRETRY_FOREVER
            espApSt = EspApSt::connectToAp;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: overflow!"));
            DBG_PORT.println(F("  : waitForResult > connectToAp"));
            #endif
            #elif defined ESP_AT_AP_VERSION_WILLRETRY_N
            static uint8_t retryCnt{};
            if(retryCnt < ESP_AT_AP_QTY_RETRIES){
              retryCnt++;
              espApSt = EspApSt::connectToAp;
              #ifdef DBG_ESP_AT_AP_SM
              DBG_PORT.println(F("espApSt: overflow!"));
              DBG_PORT.println(F("  : waitForResult > connectToAp"));
              #endif
            }
            else{
              espAt_ap_result = EspAtApResult::failedConnectToAp;
              uint8_t attempts = retryCnt;
              retryCnt = 0;
              espApSt = EspApSt::waiting;
              #ifdef DBG_ESP_AT_AP_SM
              DBG_PORT.println(F("espApSt: overflow!"));
              DBG_PORT.print(F("  : retries: ")); DBG_PORT.println(attempts);
              DBG_PORT.println(F("  : waitForResult > waiting"));
              #endif
            }
            #else
            espAt_ap_result = EspAtApResult::failedConnectToAp;
            espApSt = EspApSt::waiting;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: overflow!"));
            DBG_PORT.println(F("  : waitForResult > waiting"));
            #endif
            #endif
          }
        }
          break;
        case EspAtResult::invalidCmd:
          if(espApStPrev == EspApSt::reset){
            espAt_ap_result = EspAtApResult::failedReset;
            espApSt = EspApSt::waiting;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: invalid ssid!"));
            DBG_PORT.println(F("  : waitForResult > waiting"));
            #endif
          }
          else if(espApStPrev == EspApSt::stationMode){
            espAt_ap_result = EspAtApResult::failedStationMode;
            espApSt = EspApSt::waiting;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: invalid ssid!"));
            DBG_PORT.println(F("  : waitForResult > waiting"));
            #endif
          }
          else if(espApStPrev == EspApSt::connectToAp){
            espAt_ap_result = EspAtApResult::failedConnectToAp;
            espApSt = EspApSt::waiting;
            #ifdef DBG_ESP_AT_AP_SM
            DBG_PORT.println(F("espApSt: invalid ssid!"));
            DBG_PORT.println(F("  : waitForResult > waiting"));
            #endif
          }
          break;
        default:
          break;
      }
    }
      break;
    default:
      break;
  }
  // ESP8266 AP routines end   ====================
  // ESP8266 AT routines start ====================
  espAt_loop();
  // ESP8266 AT routines end   ====================
}
#endif