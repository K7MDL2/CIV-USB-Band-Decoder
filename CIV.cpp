///////////////////////////spin the encoder win a frequency!!////////////////////////////
//
//		CIV.cpp
//
//   Process CIV messages sent and received
//

#include "CIV-USB-Band-Decoder.h"
#include "RadioConfig.h"
#include "CIV.h"

extern Metro CAT_Poll;        // Throttle the servicing for CAT comms
extern Metro CAT_Log_Clear;   // Clear the CIV log buffer
extern Metro CAT_Freq_Check;  // Clear the CIV log buffer

extern uint8_t curr_band;  // global tracks our current band setting.
extern uint64_t VFOA;      // 0 value should never be used more than 1st boot before EEPROM since init should read last used from table.
extern uint64_t VFOB;
extern uint64_t radio_VFO;      // temp for freq from radio.  Convert to Xvr freq if acting as IF for current band
extern uint8_t radio_mode;      // mode from radio messages
extern uint8_t radio_filter;    // filter from radio messages
extern uint8_t radio_data;      // filter from radio messages
extern int16_t radio_RIT;    	// RIT offest 9.999KHz to -9.999KHz from radio CI-V
extern int32_t radio_DUP;		// Radio Duplex Offset
extern uint8_t radio_RIT_On_Off;
extern uint8_t radio_XIT_On_Off;
extern uint8_t user_Profile;
extern struct Band_Memory bandmem[];
extern struct Modes_List modeList[];
extern struct Filter_Settings filter[];
extern struct User_Settings user_settings[];
extern struct AGC agc_set[];
extern unsigned int hexToDec(String hexString);

uint64_t freq = 0;

struct cmdList cmd_List[End_of_Cmd_List] = {
    {CIV_C_F_SEND,          {1,0x00}},                      // send operating frequency to all
    {CIV_C_F1_SEND,         {1,0x05}},                      // send operating frequency to one
    {CIV_C_F_READ,          {1,0x03}},                      // read operating frequency
    {CIV_C_F26,        		  {1,0x26}},                      // read selected VFO m data, filt -  26 datafield template; selected VFO; mode, data on/off(0-1), filter (1-3);
    {CIV_C_F26A,        	  {2,0x26,0x00}},                 // read/set selected VFO m data, filt
    {CIV_C_F26B,       		  {2,0x26,0x01}},                 // read/set  un- selected VFO m data, filt
    {CIV_C_F25A_SEND,       {2,0x25,0x00}},                 // set selected VFO frequency
    {CIV_C_F25B_SEND,       {2,0x25,0x01}},                 // set un-selected VFO frequency

    {CIV_C_MOD_READ,        {1,0x04}},               	      // read Modulation Mode in use
    {CIV_C_MOD_SET,         {3,0x06,0x23,0x02}},  		      // set mode to ATV and FIL2, same 2 byte filed for cmds 1, 4, and 6
    {CIV_C_MOD_SEND ,       {1,0x01}},                      // send Modulation Mode to all
    {CIV_C_MOD1_SEND,       {1,0x06}},                      // send Modulation Mode to one
    {CIV_C_MOD_USB_F1_SEND, {3,0x06,0x01,0x01}},            // send USB Filter 1 
    {CIV_C_MOD_USB_SEND,    {2,0x06,0x01}},                 // send USB Filter 1 
    
    {CIV_C_USB_D0_F2_SEND,  {5,0x26,0x00,0x01,0x00,0x02}},  // selected VFO; mod USB; Data OFF; RX_filter F2;
    {CIV_C_USB_D1_F2_SEND,  {5,0x26,0x00,0x01,0x01,0x02}},  // selected VFO; mod USB; Data ON;  RX_filter F2;
    {CIV_C_LSB_D0_F2_SEND,  {5,0x26,0x00,0x00,0x00,0x02}},  // selected VFO; mod USB; Data OFF; RX_filter F2;
    {CIV_C_LSB_D1_F2_SEND,  {5,0x26,0x00,0x00,0x01,0x02}},  // selected VFO; mod USB; Data ON;  RX_filter F2;
    {CIV_C_FM_D1_F1_SEND,   {5,0x26,0x00,0x05,0x01,0x01}},  // selected VFO; mod USB; Data ON;  RX_filter F2;
    
    {CIV_C_ATTN_READ,   	  {1,0x11}},                  	  // Attn read state
    {CIV_C_ATTN_OFF,   		  {2,0x11,0x00}},                 // Attn OFF
    {CIV_C_ATTN_ON,    		  {2,0x11,0x10}},                 // Attn 10dB (144, 432, 1200 bands only)
    {CIV_C_SPLIT_READ,      {1,0x0F}},                      // read Split OFF
    {CIV_C_SPLIT_OFF_SEND,  {2,0x0F,0x00}},                 // set split OFF
    {CIV_C_SPLIT_ON_SEND,   {2,0x0F,0x01}},                 // Set split ON
    {CIV_C_RFGAIN,          {2,0x14,0x02}},                 // send/read RF Gain
    {CIV_C_AFGAIN,          {2,0x14,0x01}},                 // send/read AF Gain
    {CIV_C_RFPOWER,         {2,0x14,0x0A}},                 // send/read selected bands RF power
    {CIV_C_S_MTR_LVL,       {2,0x15,0x02}},                 // send/read S-meter level (00 00 to 02 55)  00 00 = S0, 01 20 = S9, 02 41 = S9+60dB
    {CIV_C_PREAMP_READ,     {2,0x16,0x02}},             	  // read preamp state
    {CIV_C_PREAMP_OFF,      {3,0x16,0x02,0x00}},            // send/read preamp 3rd byte is on or of for sending - 00 = OFF, 01 = ON
    {CIV_C_PREAMP_ON,       {3,0x16,0x02,0x00}},            // send/read preamp 3rd byte is on or of for sending - 00 = OFF, 01 = ON
    {CIV_C_PREAMP_ON2,      {3,0x16,0x02,0x02}},            // send/read preamp 3rd byte is on or of for sending - 00 = OFF, 01 = ON - not on 905
    {CIV_C_AGC_READ,        {2,0x16,0x12}},                 // send/read AGC  01 = FAST, 02 = MID, 03 = SLOW
    {CIV_C_AGC_FAST,        {3,0x16,0x12,0x01}},            // send/read AGC  01 = FAST, 02 = MID, 03 = SLOW
    {CIV_C_AGC_MID,         {3,0x16,0x12,0x02}},            // send/read AGC  01 = FAST, 02 = MID, 03 = SLOW
    {CIV_C_AGC_SLOW,        {3,0x16,0x12,0x03}},            // send/read AGC  01 = FAST, 02 = MID, 03 = SLOW
    {CIV_C_CW_MSGS,         {1,0x17}},                      // Send CW messages see page 17 of prog manual for char table
    {CIV_C_BSTACK,          {2,0x1A,0x01}},                 // send/read BandStack contents - see page 19 of prog manual.  
                                                                    // data byte 1 0xyy = Freq band code
                                                                    // dat abyte 2 0xzz = register code 01, 02 or 03
                                                                    // to read 432 band stack register 1 use 0x1A,0x01,0x02,0x01
    {CIV_C_MY_POSIT_READ,   {2,0x23,0x00}},          	    // read my GPS Position
    {CIV_C_MY_POSIT_DATA,   {1,0x23}},          	    	  // read my GPS Position
    {CIV_C_RF_POW,          {2,0x14,0x0A}},            		// send / read max RF power setting (0..255 == 0 .. 100%)
    {CIV_C_TRX_ON_OFF,      {1,0x18}},                 		// switch radio ON/OFF
    {CIV_C_TRX_ID,          {2,0x19,0x00}},            		// ID query
    {CIV_C_TX,              {2,0x1C,0x00}},            		// query of TX-State 00=OFF, 01=ON
    // the following three commands don't fit for IC7100 !!!
    {CIV_C_DATE,            {4,0x1A,0x05,0x00,0x94}},  		// + 0x20 0x20 0x04 0x27 for 27.4.2020
    {CIV_C_TIME,            {4,0x1A,0x05,0x00,0x95}},  		// + 0x19 0x57 for 19:57
    //{CIV_C_UTC,             {4,0x1A,0x05,0x00,0x96}},  		// + 0x01,0x00,0x00 = +1h delta of UTC to MEZ
    {CIV_C_UTC_READ_905,    {4,0x1A,0x05,0x01,0x81}},     //  Get UTC Offset
    //{CIV_C_UTC_SEND,        {4,0x1A,0x05,0x00,0x96}},  		// + 0x01,0x00,0x00 = +1h delta of UTC to MEZ
    {CIV_C_UTC_READ_705,    {4,0x1A,0x05,0x01,0x70}},  		// + 0x01,0x00,0x00 = +1h delta of UTC to MEZ
    {CIV_C_DUPLEX_READ,		  {1,0x0C}},          	    	  // read Duplex Offset  - has 3 bytes frequency offset data
    {CIV_C_DUPLEX_SEND,		  {1,0x0D}},	          	    	// send Duplex Offset
    {CIV_C_RIT_XIT,			    {2,0x21,0x00}},          	    // read or send RIT/XIT Offset  - has 3 bytes frequency offset data  XIT and RIT share this Offset value
    {CIV_C_RIT_ON_OFF,		  {2,0x21,0x01}},	          	  // send or send RIT ON or Off status 00 = , 01 = t
    {CIV_C_XIT_ON_OFF,		  {2,0x21,0x02}},	          	  // send or send XIT Offset
    {CIV_C_RADIO_OFF,		    {2,0x18,0x00}},	          	  // Turn Off the radio
    {CIV_C_RADIO_ON,		    {2,0x18,0x01}}	          	  // Turn on the radio
};

int hr_off;  // time offsets to apply to UTC time
int min_off;
int shift_dir;  // + or -

// ********************************************Loop ******************************************
bool freqReceived = false;  // initially, no frequency info has been received from the radio
//bool modeReceived = false;
uint8_t freqPoll = 0;  // number of initial frequency queries in addition to the broadcast info

//-------------------------------------------------------------------------------
// create the civ object
CIV civ;  // create the CIV-Interface object
//-------------------------------------------------------------------------------

CIVresult_t CIVresultL;

void civ_905_setup(void) 
{
  civ.setupp(true, false, "");     // initialize the civ object/module
                                   // and the ICradio objects
  civ.registerAddr(CIV_ADDR);  // tell civ, that this is a valid address to be used
}

//***************************************************************************
//                check_CIV
//
// Polls for messages in queue from radio (CIV_Transceive must be on)
// returns
// 0 = nothing received
// 1 frequency received
// 2 mode received
// xxx others TBD
//
//***************************************************************************
//
uint8_t check_CIV(uint32_t time_current_baseloop) 
{
  	uint8_t msg_type = 0;
	static uint8_t cmd_num = 0;
	uint8_t match = 0;

  	msg_type = 0;
  	CIVresultL = civ.readMsg(CIV_ADDR);

  	freqReceived = false;
	
  	if (CIVresultL.retVal <= CIV_NOK) // valid answer received !
	{  

		if (CIVresultL.retVal == CIV_OK_DAV) 
		{  
			// Data 
			//DPRINTF("check_CIV: CMD Body Length = "); DPRINT(CIVresultL.cmd[0],HEX); DPRINTF(" CMD  = "); DPRINTLN(CIVresultL.cmd[1],HEX);
			
			for (cmd_num = CIV_C_F_SEND; cmd_num < End_of_Cmd_List; cmd_num++)  // loop through the command list structure looking for a pattern match
			{				
				for (int i = 0; i <= CIVresultL.cmd[0]; i++)  // start at the highest and search down. Break out if no match. Make it to the bottom and you have a match
				{
					//PC_Debug_port.printf("check_civ: cmd_num=%d from radio length=%d and cmd=%X, on remote length=%d and cmd=%X\n",cmd_num, CIVresultL.cmd[0], CIVresultL.cmd[1], cmd_List[cmd_num].cmdData[0], cmd_List[cmd_num].cmdData[1]);
					if(cmd_List[cmd_num].cmdData[i] != CIVresultL.cmd[i]) 
					{
						//DPRINTF("check_CIV: Skip this one - Matched 1 element: look at next field, if any left. CMD Body Length = "); DPRINT(CIVresultL.cmd[0],HEX); DPRINTF(" CMD  = "); DPRINTLN(CIVresultL.cmd[1],HEX);
						match = 0;
						break;
					}
					match++;
					//DPRINTF("check_CIV: Possible Match: CMD Body Length = "); DPRINT(CIVresultL.cmd[0],HEX); DPRINTF(" CMD  = "); DPRINTLN(CIVresultL.cmd[1],HEX);
				}
				if (match == CIVresultL.cmd[0]+1) 
				{
					//DPRINTF("check_CIV: Match: CMD Body Length = "); DPRINT(CIVresultL.cmd[0],HEX); DPRINTF(" CMD  = "); DPRINTLN(CIVresultL.cmd[1],HEX);
					break;
				}
			}

			if (cmd_num >= End_of_Cmd_List-1)
			{
				cmd_num--;
				PC_Debug_port.printf("Loop Completed, NO match found -- cmd_num=%d from radio length=%d and cmd=%X, on remote length=%d and cmd=%X\n",cmd_num, CIVresultL.cmd[0], CIVresultL.cmd[1], cmd_List[cmd_num].cmdData[0], cmd_List[cmd_num].cmdData[1]);
				//DPRINTF("check_CIV: No match found: for "); DPRINTLN(cmd_num);
				return 0;
			}
			else
			{
				//DPRINTF("check_CIV: Match Cmd list index: "); DPRINT(cmd_num);  DPRINTF("  CMD: "); DPRINTLN(CIVresultL.cmd[1],HEX);   
			}		
			// Check for Frequency message type
			// NOTE:  when ther radio side changes bands the first message is a mode change followed by the frequency. 
			// An attempt to get the 0x26 extended mode while the frequency is being sent results in a reliabl BUS conflict and mode and freq both fail.
			// Hold off the 0x26 request until bandchange function asks for it.  Can test VFOA old and requested.
			switch (cmd_num) 
			{
				case CIV_C_F_READ:
				case CIV_C_F_SEND:
				case CIV_C_F1_SEND:
				{  // command CIV_C_F_SEND received
					//DPRINTF("check_CIV: CI-V Returned Frequency: "); DPRINTLN(CIVresultL.value);
					radio_VFO = (uint64_t)CIVresultL.value;
					msg_type = 1;
					freqReceived = true;
					break;
				} // Frequency changed 

				case CIV_C_MOD_READ: // Test for MODE change
				case CIV_C_MOD_SEND:
				{  
					// command CIV_C_MODE_READ received
					radio_mode = CIVresultL.value/100;
					DPRINTF("\ncheck_CIV: Mode in BCD: "); DPRINTLN(radio_mode);
					
					// look up the bcd value in our modelist table to see what radio mode it is 
					for (uint8_t i = 0; i< MODES_NUM; i++)
					{
						if (modeList[i].mode_num == hexToDec(radio_mode))  // match bcd value to table mode_num value to get out mode index that we store
						{	
							radio_mode = i;  // now know our decimal index
							break;  
						}
					}// radio_mode now converted to a table index
					
					radio_filter = CIVresultL.value - ((CIVresultL.value/100)*100);
					
					DPRINTF("check_CIV: CI-V Returned Mode: "); DPRINT(modeList[radio_mode].mode_label);  DPRINTF("  Radio Mode = "); DPRINT(radio_mode); DPRINTF("  Filter = "); DPRINTLN(filter[radio_filter].Filter_name);    
					
					msg_type = 2;
					freqReceived = false;
					break;
				}  // Mode changed

				// Test for Bstack message
				case CIV_C_BSTACK:
				//if (CIVresultL.cmd[1] == CIV_C_BSTACK[1]) 
				{
					// returned mnessage is 1A 01 band code, reg code, freq, mode, data on/off
					// receive fields 6 to 52 on p18 of prog guide
					// 6-10 freq or 6-12 for 10GHz+
					// +2 Mode
					// +1 Data mode on/off
					// can ignore the rest - appies to Rptrs and DigV modes
					uint16_t bstack_band  = CIVresultL.datafield[1];  // byte 0 is the datafield length
					uint16_t bstack_reg   = CIVresultL.datafield[2];

					DPRINTF("check_CIV: CI-V Returned Band Stack - Band: "); DPRINT(bstack_band); DPRINTF("  Register: "); DPRINT(bstack_reg);
						
					uint8_t F_len;

					if (CIV_ADDR == CIV_ADDR_905) 
						F_len = 6;	// 6 bytes for IC905
					else
						F_len = 5;	// 6 bytes for IC705 and other models < 10Ghz
					
					uint8_t idx = 0;
					uint8_t DstartIdx = 3;  // start of freq for 6 bytes for IC905, 5 for other models
					uint8_t DstopIdx = DstartIdx + F_len;  // start of mode, filter data on/off will be 1-3 bytes after
					uint8_t rxBuffer[F_len];  // hold 6 bytes of frequency
					uint64_t mul = 1;
					uint64_t bstack_freq = 0;
					uint8_t band = 0;  // temp storage for radio bstack band code to remote bandmem table band index

					for (idx = DstartIdx; idx < DstopIdx; idx++)           // pull out frequency from datafield result
						rxBuffer[idx-DstartIdx] = CIVresultL.datafield[idx];

					// 6 byte data -> first byte is of lowest order - for 905 10G and up bands
					for (idx = 0; idx < F_len; idx++) {
						bstack_freq += (rxBuffer[idx] & 0x0f) * mul; mul *= 10;
						bstack_freq += (rxBuffer[idx] >> 4) * mul; mul *= 10;
					}
					DPRINTF("  Frequency: "); DPRINT(bstack_freq);
					
					radio_mode = CIVresultL.datafield[DstopIdx];  // modulation mode in BCD
					radio_filter = CIVresultL.datafield[DstopIdx+1];  // filter 
					radio_data = CIVresultL.datafield[DstopIdx+2];  // data mode on or off
					DPRINTF("  Mode: "); DPRINT(radio_mode, HEX); DPRINT("  Filter: ");DPRINT(radio_filter, HEX);  DPRINT("  Data: ");DPRINT(radio_data, HEX);   
								
					// convert to our own mode extended mode list to show -D (or not)
					for (uint8_t i = 0; i< MODES_NUM; i++)
					{
						if (modeList[i].mode_num == radio_mode && modeList[i].data == radio_data)
						{
							radio_mode = i;   
							break;
						}
					}
					DPRINTF("  Mode Index: "); DPRINT(radio_mode); DPRINTF("  Mode label: "); DPRINTLN(modeList[radio_mode].mode_label); 
					
					// convert radio bstack band code to remote bandmem table band index
					switch (bstack_band)
					{
						case 1: band = BAND144; break;
						case 2: band = BAND432; break;
						case 3: band = BAND1296; break;
						case 4: band = BAND2400; break;
						case 5: band = BAND5760; break;
						case 6: band = BAND10G; break;
						default: band = BAND144; break;
					}
// ToDo: convert the radio mode to our extended most list which is a combo of mode and data
// This lookup is probably done elsewhere so put it here too.
					switch (bstack_reg)
					{
						case 1: bandmem[band].vfo_A_last   	= bstack_freq; 
								bandmem[band].mode_A 		= radio_mode; // now an index to our extended mode list
								bandmem[band].filter_A 		= radio_filter;
								modeList[bandmem[band].mode_A].Width = radio_filter;
								bandmem[band].data_A 		= radio_data; 		// LSB, USB, AM, FM modes can have DATa mode on or off.  All other radio modes data is NA.
								break;
						case 2: bandmem[band].vfo_A_last_1 	= bstack_freq; 
								bandmem[band].mode_A_1 		= radio_mode;
								bandmem[band].filter_A_1 	= radio_filter;
								modeList[bandmem[band].mode_A].Width = radio_filter;
								bandmem[band].data_A_2 		= radio_data;
								break;
						case 3: bandmem[band].vfo_A_last_2 	= bstack_freq; 
								bandmem[band].mode_A_2 		= radio_mode;
								bandmem[band].filter_A_2	= radio_filter; 
								modeList[bandmem[band].mode_A].Width = radio_filter;
								bandmem[band].data_A_2 		= radio_data;
								break;
					}
					msg_type = 3;
					freqReceived = false;
					break;
				}

				case CIV_C_F26A:
				case CIV_C_F26B:
				case CIV_C_F26:
				//case CIV_C_F26_SEND:
				{
					// [0]=x is length, [1]== 0 is selected VFO
					radio_mode   = CIVresultL.datafield[2];  // mode is in HEX!
					radio_data   = bandmem[curr_band].data_A   = CIVresultL.datafield[3];  // data on/off
					radio_filter = bandmem[curr_band].filter_A = CIVresultL.datafield[4];  // filter setting
					modeList[bandmem[curr_band].mode_A].Width = radio_filter;

  					// convert to our own mode list to show -D (or not)
					for (uint8_t i = 0; i< MODES_NUM; i++)
					{
						if (modeList[i].mode_num == radio_mode && modeList[i].data == radio_data)
						{
							radio_mode = bandmem[curr_band].mode_A = i;  // now stored as our index to the combo in modelist table
							break;
						}
					}
					
					modeList[radio_mode].Width = radio_filter;  // store filter in mode table using the extended mode value (-D or no -D)
					DPRINTF("check_CIV: CI-V Returned Extended Mode: "); DPRINT(modeList[radio_mode].mode_label); DPRINT("  Filter: "); DPRINT(filter[radio_filter].Filter_name); DPRINT("  Data: "); DPRINTLN(radio_data);  
					msg_type = 4;
					freqReceived = false;
					break;
				}  // Mode changed

				// Test for  TX-RX change
				case CIV_C_TX:	
				{
					uint8_t TX;
					static uint8_t TX_last = 0;

					//get_RXTX_from_Radio(); asks the radio every 500ms or so from main loop.  Look for answer back here
					TX = CIVresultL.value;
					if (TX != TX_last)  // stte has changed
					{
						//DPRINTF("check_CIV: CI-V Returned RX-TX status changed: "); DPRINTLN(TX);
						if (TX == 1) // in TX
							user_settings[user_Profile].xmit = 1;
						if (TX == 0) // in RX
							user_settings[user_Profile].xmit = 0;
						TX_last = TX; // capure state to detect change
						msg_type = 5;
					}  // if no state change then will return default msg_type which is 0 and the main loop wll skip out.
					freqReceived = false;
					break;
				}  // RX TX  changed	

				// Test for MY_POSITION response
				// Be sure to give long response time before another command or this long answer will be cut short
				//case CIV_C_MY_POSIT_READ:
				case CIV_C_MY_POSIT_DATA:
				{
					//uint8_t RX = CIVresultL.value;

					//DPRINTF("check_CIV: CI-V Returned MY POSITION and TIME: "); DPRINTLN(retValStr[CIVresultL.value]);
					//               pos      1             2  3  4  5  6    7  8  9 10 11 12   13 14 15 16   17 18   19 20 21   22 23  24  25  26 27 28 term
					// FE.FE.E0.AC.  23.00.  datalen byte  47.46.92.50.01.  01.22.01.98.70.00.  00.15.59.00.  01.05.  00.00.07.  20.24. 07. 20. 23.32.45. FD
					//                                     47.46.925001 lat 122.01.987000 long  155.900m alt  105deg   0.7km/h   2024   07  20  23:32:45 UTC
					// when using datafield, add 1 to prog guide index to account for first byte used as length counter - so 27 is 28 here.
					DPRINTF("** Time from Radio is: ");
					int _hr = bcdByte(CIVresultL.datafield[26]); DPRINT(_hr); DPRINTF(":");
					int _min = bcdByte(CIVresultL.datafield[27]); DPRINT(_min);DPRINTF(":");
					int _sec = bcdByte(CIVresultL.datafield[28]); DPRINT(_sec);DPRINTF(" ");
					
					int _month = bcdByte(CIVresultL.datafield[24]); DPRINT(_month);DPRINTF(".");
					int _day = bcdByte(CIVresultL.datafield[25]); DPRINT(_day); DPRINTF(".");
					int _yr = bcdByte(CIVresultL.datafield[23]); DPRINTLN(_yr); // yr can be 4 or 2 digits  2024 or 24
								
					setTime(_hr,_min,_sec,_day,_month,_yr);  // display UTC time
					
					if (!UTC) 
					{
						setTime(_hr+hr_off,_min+min_off,_sec,_day,_month,_yr);  // correct to local time
						DPRINT(_hr);DPRINTF(":");DPRINT(_min);DPRINTF(":");DPRINT(_sec);DPRINTF(" ");DPRINT(_month);DPRINTF(".");DPRINT(_day);DPRINTF(".");DPRINTLN(_yr); 
					}
					
					msg_type = 6;
					freqReceived = false;
					break;
				}  // MY Position 
				
				// Test for UTC OFFSET 
				// Be sure to give long response time before another command or this long answer will be cut short
				case CIV_C_UTC_READ_905:
        case CIV_C_UTC_READ_705:
				{
					//               pos     sub cmd  len        1    2     3          term
					// FE.FE.E0.AC.  1A.05.  01.81.   3         07.  00.   01.         FD
					// FE.FE.E0.AC.  1A.05.           datalen   hr   min   1=minus 0=+
					//                                     offset time 00 00 to 14 00    Shift dir 00 + and 01 is -
					// when using datafield, add 1 to prog guide index to account for first byte used as length counter - so 3 is 4 here.

					hr_off = bcdByte(CIVresultL.datafield[1]); 
					min_off = bcdByte(CIVresultL.datafield[2]); 
					shift_dir = bcdByte(CIVresultL.datafield[3]);
					
					DPRINTF("check_CIV: CI-V Returned UTC Offset: "); 
					if (shift_dir) 
					{
						hr_off = hr_off * -1;  // invert  - used by UTC set function
						min_off = min_off * -1;  // invert  - used by UTC set function
					}
					DPRINT(hr_off); DPRINTF(":");DPRINTLN(min_off);

					//get current time and correct or set time zone offset
					//setTime(_hr,_min,_sec,_day,_month,_yr);
					
					msg_type = 7;
					freqReceived = false;
					break;
				}  // UTC Offset

				case CIV_C_PREAMP_READ:	
				//case CIV_C_PREAMP_ON:
    			//case CIV_C_PREAMP_OFF:
				{
					uint8_t _val = CIVresultL.value;
					
					DPRINTF("check_CIV: CI-V Returned PreAmp status: "); DPRINTLN(_val);

					if (_val > 0)
					{
						// Reading from the radio we just want to update database and screen and not repeat back to radio.
						bandmem[curr_band].attenuator = ATTN_OFF;   // Only 1 on at a time
						bandmem[curr_band].preamp = PREAMP_ON;
					}
					if (_val == 0)
					{
						bandmem[curr_band].preamp = PREAMP_OFF;
					}
					msg_type = 8;
					freqReceived = false;
					displayPreamp();
					displayAttn();
					break;
				}  // Preamp changed

        case CIV_C_SPLIT_READ: {
            uint8_t _val = CIVresultL.value;

            //DPRINTF("CIV_Action:  CI-V Returned Preamp status: "); DPRINTLN(_val);
            if (_val > 0)
            {
                //bands[band].atten = ATTN_OFF;
                bandmem[curr_band].split = 1;              
            }
            if (_val == 0)
            {
                bandmem[curr_band].split = 0;
            }
            //displayAttn();
            //displayPreamp();
            break;
        }

				case CIV_C_ATTN_READ:	
				case CIV_C_ATTN_ON:
    		case CIV_C_ATTN_OFF:
				{
					uint8_t _val = CIVresultL.value;

					DPRINTF("check_CIV: CI-V Returned Attn status: "); DPRINTLN(_val);
					if (_val > 0)
					{
						bandmem[curr_band].preamp 		= PREAMP_OFF;
						bandmem[curr_band].attenuator  	= ATTN_ON;
					}
					if (_val == 0)
					{
						bandmem[curr_band].attenuator   = ATTN_OFF;
					}
					
					msg_type = 9;
					freqReceived = false;
					displayAttn();
					displayPreamp();
					break;
				}  // Attn changed
				
				case CIV_C_AGC_READ:
				//case CIV_C_AGC_FAST:
				//case CIV_C_AGC_MID:
				//case CIV_C_AGC_SLOW:
				{
					uint8_t _val = CIVresultL.value;

					if (_val == 1) {bandmem[curr_band].agc_mode = AGC_FAST; AGC(3);} // 0 sets to database state. 2 is toggle state. -1 and 1 are down and up
					if (_val == 2) {bandmem[curr_band].agc_mode = AGC_MID;  AGC(3);} // 0 sets to database state. 2 is toggle state. -1 and 1 are down and up
					if (_val == 3) {bandmem[curr_band].agc_mode = AGC_SLOW; AGC(3);} // 0 sets to database state. 2 is toggle state. -1 and 1 are down and up
					DPRINTF("check_CIV: CI-V Returned AGC state: "); DPRINTLN(agc_set[bandmem[curr_band].agc_mode].agc_name);
					msg_type = 10;
					freqReceived = false;
					displayAgc();
					break;
				}  // AGC changed

				case CIV_C_DUPLEX_READ:
				case CIV_C_DUPLEX_SEND:
				{
					//              Dup            1k/100Hz   100K/10Khz    10M/1MHz    term
					// FE.FE.E0.AC. 0C.               01.        81.          00.        FD
					//					 datalen=3   
					// when using datafield, add 1 to prog guide index to account for first byte used as length counter - so 3 is 4 here.

					//uint8_t DUP_10MHZ 	= bcdByte(CIVresultL.datafield[3] & 0xF0); 
					//uint8_t DUP_1MHZ 	= bcdByte(CIVresultL.datafield[3] & 0x0F); 
					
					//uint8_t DUP_100KHZ 	= bcdByte(CIVresultL.datafield[2] & 0xF0); 
					//uint8_t DUP_10KHZ 	= bcdByte(CIVresultL.datafield[2] & 0x0F); 

					//uint8_t DUP_1KHZ 	= bcdByte(CIVresultL.datafield[1] & 0xF0);
					//uint8_t DUP_100HZ 	= bcdByte(CIVresultL.datafield[1] & 0x0F);
					
				
					//int8_t DUP_MINUS 	= CIVresultL.datafield[3];    // 00 = plus, 01 - minus
					radio_DUP 	   = bcdByte(CIVresultL.datafield[3])*1000;
					radio_DUP  	  += bcdByte(CIVresultL.datafield[2])*10; 
					radio_DUP  	  += bcdByte(CIVresultL.datafield[1]); 
					radio_DUP 	  *= 1000;  //convert KHz to Hz
					//radio_DUP = DUP_MINUS ?  radio_DUP*-1: radio_DUP;
					DPRINTF("check_CIV: Radio Returned Duplex Offset: "); DPRINT(radio_DUP); DPRINTLNF("Hz");

					msg_type = 11;
					freqReceived = false;
					break;
				}  // Duplex Offset

				case CIV_C_RIT_XIT:
				{
					// RIT -1.97KHz
					//              RIT            1k/100Hz   10/1hz   +/1    term
					// FE.FE.E0.AC. 21.00              01.      97.     01.     FD
					//					 datalen[0]=3   
										//                                     
					// when using datafield, add 1 to prog guide index to account for first byte used as length counter - so 3 is 4 here.

					int8_t RIT_MINUS = CIVresultL.datafield[3];    // 00 = plus, 01 - minus
					radio_RIT 		 = bcdByte(CIVresultL.datafield[2])* 100; // * -RIT_MINUS; 
					radio_RIT  	    += bcdByte(CIVresultL.datafield[1]); 
					radio_RIT = RIT_MINUS ?  radio_RIT*-1: radio_RIT;
					DPRINTF("check_CIV: RIT/XIT Offset: "); DPRINT(radio_RIT); DPRINTLNF("Hz");

					msg_type = 12;
					freqReceived = false;
					break;
				}  // RIT/XIT Offset

				case CIV_C_RIT_ON_OFF:
				{	// when using datafield, add 1 to prog guide index to account for first byte used as length counter - so 3 is 4 here.
					
					radio_RIT_On_Off  	    = bcdByte(CIVresultL.datafield[1]); 
					DPRINTF("check_CIV: RIT On/Off: "); DPRINTLN(radio_RIT_On_Off);
					msg_type = 13;
					freqReceived = false;
					break;
				}  // RIT On/Off

				case CIV_C_XIT_ON_OFF:
				{	// when using datafield, add 1 to prog guide index to account for first byte used as length counter - so 3 is 4 here.
					
					radio_XIT_On_Off  	    = bcdByte(CIVresultL.datafield[1]); 
					DPRINTF("check_CIV: XIT On/Off: "); DPRINTLN(radio_XIT_On_Off);
					msg_type = 14;
					freqReceived = false;
					break;
				}  // XIT On/Off

			}  // end switch
			return msg_type;
    	}  // Data available

		// ----------------------------------  do a query for frequency, if necessary
		// poll every 500 * 10ms = 5sec until a valid frequency has been received
		//if ((freqReceived == false) && (CAT_Freq_Check.check() == 1)) 
		if (0)  // not sure we need this, possibly corrupting other sequences
		{
			delay(20);
			CIVresultL = civ.writeMsg(CIV_ADDR, cmd_List[CIV_C_F_READ].cmdData, CIV_D_NIX, CIV_wChk);
			if (CIVresultL.retVal<=CIV_NOK)
			{
				DPRINTF("check_CIV: Poll for RADIO Frequency Status: "); DPRINT(CIVresultL.retVal);
				DPRINTF("  Poll for RADIO Frequency Return Value: "); DPRINTLN((uint8_t)CIVresultL.value);
				msg_type = 1;
			}
		}
	}// valid answer received
  	return msg_type;
}  // if BASELOOP_TICK

#ifdef GPS
void pass_GPS(void) 
{
  civ.readGPS();  // read USB serial ch 'B' for GPS NMEA data strings
                  // ToDo: extract grid and time info and display
}
#endif

void pass_CAT_msgs_to_RADIO(void) 
{
  civ.pass_CAT_msg_to_RADIO();
}

void pass_CAT_msg_to_PC(void)
{
  civ.pass_CAT_msg_to_PC();   // civ.readmsg() always does this.
}

// If you want to see the hex message contest turn on logging in the library file.  This will display the log.
void show_CIV_log(void) 
{
  //if (CAT_Poll.check() == 1)
    civ.logDisplay();  // show messages accumulated until cleared.

  // can clear the log periodically here based on timer
  //if (CAT_Log_Clear.check() == 1)  // Clear the CIV log buffer, jsu show last 2 seconds
    civ.logClear();
}

uint8_t getByteResponse(const uint8_t m_Counter, const uint8_t offset, const uint8_t buffer[])
{
  if (m_Counter < offset + 3)
        return 0;
	uint8_t ret = bcdByte(buffer[offset]) * 100;
    ret += bcdByte(buffer[offset+1]);
    return ret;
}


// below lines are in case you want to stop/start log scrolling
// use "l",if "#define log_CIV" in file civ.h is active
//if (keyCmd==KEY_LOG_PRESSED) civ.logDisplay();