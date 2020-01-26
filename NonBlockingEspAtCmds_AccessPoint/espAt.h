#ifndef ESP_AT_H
#define ESP_AT_H

#include "definitions.h"

#ifdef DBG_ALL
// #define DBG_ESP_AT_SM
#endif
#define ESP_SIZE_RECEIVEBUFFER 300
#define ESP_SIZE_CMDSTRING 50
#define ESP_SIZE_SEARCHSTRING 20

enum class EspAtResult:uint8_t{
  none,
  timeout,
  searchOk,
  overflow,
  invalidCmd
};
static EspAtResult espAt_result{};
static char espAt_cmd[ESP_SIZE_CMDSTRING+1]{};
static char espAt_searchStr[ESP_SIZE_SEARCHSTRING+1]{};
static uint32_t espAt_timeoutMs{};
static uint8_t espAt_srchStrSz{};

// This function returns the latest result.
// If result != EspAtResult::none, it is reset to EspAtResult::none.
// Hence, user must buffer the return value since the internal variable is cleared after being read.
EspAtResult espAt_getResult(){
  EspAtResult result{};
  if(espAt_result != EspAtResult::none){
    result = espAt_result;
    espAt_result = EspAtResult::none;
  }
  return result;
}

void espAt_commander(char *cmd, uint32_t timeoutMs, char *srchStr){
  uint8_t i{};
  while(cmd[i] && i<ESP_SIZE_CMDSTRING){
    espAt_cmd[i] = cmd[i];
    i++;
  }
  uint8_t s{};
  while(srchStr[s] && s<ESP_SIZE_SEARCHSTRING){
    espAt_searchStr[s] = srchStr[s];
    s++;
  }
  // Clear cmd if cmd string ptr or search string ptr is null or timeout period is 0 (don't proceed).
  if(i == 0 || s == 0 || timeoutMs == 0){
    memset(espAt_cmd, 0, sizeof(espAt_cmd));
    memset(espAt_searchStr, 0, sizeof(espAt_searchStr));
    espAt_result = EspAtResult::invalidCmd;
    #ifdef DBG_ESP_AT_SM
    DBG_PORT.println(F("esp_sm: invalid cmd"));
    #endif
  }
  else{
    espAt_srchStrSz = s;
    espAt_timeoutMs = timeoutMs;
  }
}

void espAt_loop(){
  static uint32_t tRefRcvSt{};
  static char cmd[ESP_SIZE_CMDSTRING+1]{};
  static char srchStr[ESP_SIZE_SEARCHSTRING+1]{};
  static uint8_t srchStrSz{};
  static uint32_t timeoutMs{};
  static uint8_t st{};
  switch (st)
  {
    case 0:
    {
      char testBlock[ESP_SIZE_CMDSTRING]{};
      if(memcmp(espAt_cmd, testBlock, sizeof(testBlock))){
        memset(cmd, 0, sizeof(cmd));
        memset(srchStr, 0, sizeof(srchStr));
        memcpy(cmd, espAt_cmd, sizeof(cmd));
        memcpy(srchStr, espAt_searchStr, sizeof(srchStr));
        timeoutMs = espAt_timeoutMs;
        srchStrSz = espAt_srchStrSz;

        // Clear global variables start =====
        memset(espAt_cmd, 0, sizeof(espAt_cmd));
        memset(espAt_searchStr, 0, sizeof(espAt_searchStr));
        // espAt_timeoutMs = 0;
        // espAt_srchStrSz = 0;
        // Clear global variables end   =====

        #ifdef DBG_ESP_AT_SM
        DBG_PORT.print(F("cmd: ")); DBG_PORT.println(cmd);
        DBG_PORT.print(F("srchStr: ")); DBG_PORT.println(srchStr);
        DBG_PORT.print(F("timeoutMs: ")); DBG_PORT.println(timeoutMs);
        DBG_PORT.print(F("srchStrSz: ")); DBG_PORT.println(srchStrSz);
        #endif
        while(ESP_PORT.available()) ESP_PORT.read();
        ESP_PORT.print(cmd);
        tRefRcvSt = millis();
        st = 1;
      }
    }
      break;
    case 1:
    {
      static uint16_t ctr{};
      static char rcvBfr[ESP_SIZE_RECEIVEBUFFER]{};
      while (ESP_PORT.available()){
        rcvBfr[ctr] = ESP_PORT.read();
        ctr++;
        if(ctr >= srchStrSz && strstr(rcvBfr, srchStr)){
          espAt_result = EspAtResult::searchOk;
          ctr = 0;
          memset(rcvBfr, 0, sizeof(rcvBfr));
          st = 0;
          #ifdef DBG_ESP_AT_SM
          DBG_PORT.println(F("esp_sm: searchOk"));
          #endif
          break;
        }
        else if(millis() - tRefRcvSt > timeoutMs){
          espAt_result = EspAtResult::timeout;
          ctr = 0;
          memset(rcvBfr, 0, sizeof(rcvBfr));
          st = 0;
          #ifdef DBG_ESP_AT_SM
          DBG_PORT.println(F("esp_sm: timeout 1"));
          #endif
          break;
        }
        else if(ctr >= ESP_SIZE_RECEIVEBUFFER){
          espAt_result = EspAtResult::overflow;
          ctr = 0;
          memset(rcvBfr, 0, sizeof(rcvBfr));
          st = 0;
          #ifdef DBG_ESP_AT_SM
          DBG_PORT.println(F("esp_sm: overflow"));
          #endif
          break;
        }
      }
      if(espAt_result == EspAtResult::none && millis() - tRefRcvSt > timeoutMs){
        espAt_result = EspAtResult::timeout;
        ctr = 0;
        memset(rcvBfr, 0, sizeof(rcvBfr));
        st = 0;
        #ifdef DBG_ESP_AT_SM
        DBG_PORT.println(F("esp_sm: timeout 2"));
        #endif
      }
    }
      break;
    default:
      break;
  }
}
#endif