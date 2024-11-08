///////////////////////////spin the encoder win a frequency!!////////////////////////////
//
//	 CIV.h
//
//   Process CIV messages sent and received
//

#ifndef _CIV_H_
#define _CIV_H_

#include <Arduino.h>
#include "CIV-USB-Band-Decoder.h"
#include "RadioConfig.h"
#include <CIVcmds.h>                    // https://github.com/K7MDL2/CIVmasterLib is my fork Icom CIV library https://github.com/WillyIoBrok/CIVmasterLib extended for the 905
#include <CIVmaster.h>                  // CIVcmds.h is automatically included in addition

void getradioInfo(void);
uint8_t check_CIV(uint32_t time_current_baseloop);
uint64_t FrequencyRequest(void);
void RcvCIVmsg(void);
void SendCIVmsg(void);
void civ_905_setup(void);
void pass_CAT_msgs_to_RADIO(void);
void pass_CAT_msg_to_PC(void);
void show_CIV_log(void);
void civ_setup(uint32_t currentTime);
//radioModMode_t getModMode(void);
uint8_t getByteResponse(const uint8_t m_Counter, const uint8_t offset, const uint8_t buffer[]);
uint8_t getRadioMode(void);

#ifdef GPS
  void pass_GPS(void);
#endif

const String retValStr[7] = {
  "CIV_OK",
  "CIV_OK_DAV",
  "CIV_NOK",
  "CIV_HW_FAULT",
  "CIV_BUS_BUSY",
  "CIV_BUS_CONFLICT",
  "CIV_NO_MSG"
};

#endif //_CIV_H_