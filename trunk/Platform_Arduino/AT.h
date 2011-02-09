/*
    AT.h - library for AT-Communication with GSM-devices

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/



#ifndef __AT_h
#define __AT_h
#include "WProgram.h"

/******************************* IMPORTANT ***********************************
Keep in mind that DEBUG PRINT has only very limited functionality
and can not be 100% guaranteed becuase all debug strings are sent also to the 
GSM module and thus can be interpreted as AT commands.
*****************************************************************************/
//#define DEBUG_PRINT



#define AT_LIB_VERSION 100 // library version X.YY (e.g. 1.00) 100 means 1.00
/*
    Version
    ---------------------------------
    The AT-Library was based on the GSM-Library from www.hwkitchen.com 
    Some functions came from the sserial2mobile (http://code.google.com/p/sserial2mobile/)
    The GSM-Library now links this AT-Library. 
    The sserial2mobile-Library now links this AT-Library.
    
    AT_LIB_VERSION
    010   Beta-Release     Stefan Huber (signalwerk.ch) splitted the 
                           GSM-Library 1.01 (www.hwkitchen.com) into two parts
                           the AT-Library and the GSM-Library. Now the AT-Library 
                           can be used by other devices (more platformindependant)

    100                    release 1.00
    

    ToDo
    ---------------------------------
    - RX/TX-Pin should be realised with Softserial or normal Serial.
*/



// length for the internal communication buffer
#ifndef COMM_BUF_LEN
	#define COMM_BUF_LEN        200
#endif // end of ifndef COMM_BUF_LEN


// Time-Delays
#ifndef START_TINY_COMM_TMOUT
	#define START_TINY_COMM_TMOUT           20
#endif // end of ifndef START_TINY_COMM_TMOUT

#ifndef START_SHORT_COMM_TMOUT
	#define START_SHORT_COMM_TMOUT          500
#endif // end of ifndef START_SHORT_COMM_TMOUT

#ifndef START_LONG_COMM_TMOUT
	#define START_LONG_COMM_TMOUT           1000
#endif // end of ifndef START_LONG_COMM_TMOUT

#ifndef START_XLONG_COMM_TMOUT
	#define START_XLONG_COMM_TMOUT          5000
#endif // end of ifndef START_XLONG_COMM_TMOUT

#ifndef START_XXLONG_COMM_TMOUT
	#define START_XXLONG_COMM_TMOUT         7000
#endif // end of ifndef START_XXLONG_COMM_TMOUT

#ifndef MAX_INTERCHAR_TMOUT
	#define MAX_INTERCHAR_TMOUT             20
#endif // end of ifndef MAX_INTERCHAR_TMOUT

#ifndef MAX_MID_INTERCHAR_TMOUT
	#define MAX_MID_INTERCHAR_TMOUT         100
#endif // end of ifndef MAX_INTERCHAR_TMOUT

#ifndef MAX__LONG_INTERCHAR_TMOUT
	#define MAX__LONG_INTERCHAR_TMOUT       1500
#endif // end of ifndef MAX__LONG_INTERCHAR_TMOUT

#ifndef AT_DELAY
	#define AT_DELAY                        500
#endif // end of ifndef AT_DELAY


// some constants for the IsRxFinished() method
#define RX_NOT_STARTED      0
#define RX_ALREADY_STARTED  1

// SMS type 
// use by method IsSMSPresent()
enum sms_type_enum
{
  SMS_UNREAD,
  SMS_READ,
  SMS_ALL,

  SMS_LAST_ITEM
};

enum comm_line_status_enum 
{
  // CLS like CommunicationLineStatus
  CLS_FREE,   // line is free - not used by the communication and can be used
  CLS_ATCMD,  // line is used by AT commands, includes also time for response
  CLS_DATA,   // for the future - line is used in the CSD or GPRS communication  
  CLS_LAST_ITEM
};

enum rx_state_enum 
{
  RX_NOT_FINISHED = 0,      // not finished yet
  RX_FINISHED,              // finished, some character was received
  RX_FINISHED_STR_RECV,     // finished and expected string received
  RX_FINISHED_STR_NOT_RECV, // finished, but expected string not received
  RX_TMOUT_ERR,             // finished, no character received 
                            // initial communication tmout occurred
  RX_LAST_ITEM
};


enum at_resp_enum 
{
  AT_RESP_ERR_NO_RESP = -1,   // nothing received
  AT_RESP_ERR_DIF_RESP = 0,   // response_string is different from the response
  AT_RESP_OK = 1,             // response_string was included in the response

  AT_RESP_LAST_ITEM
};

enum registration_ret_val_enum 
{
  REG_NOT_REGISTERED = 0,
  REG_REGISTERED,
  REG_NO_RESPONSE,
  REG_COMM_LINE_BUSY,
    
  REG_LAST_ITEM
};

enum call_ret_val_enum
{
  CALL_NONE = 0,
  CALL_INCOM_VOICE,
  CALL_ACTIVE_VOICE,
  CALL_INCOM_VOICE_AUTH,
  CALL_INCOM_VOICE_NOT_AUTH,
  CALL_INCOM_DATA_AUTH,
  CALL_INCOM_DATA_NOT_AUTH,
  CALL_ACTIVE_DATA,
  CALL_OTHERS,
  CALL_NO_RESPONSE,
  CALL_COMM_LINE_BUSY,

  CALL_LAST_ITEM
};


enum getsms_ret_val_enum
{
  GETSMS_NO_SMS   = 0,
  GETSMS_UNREAD_SMS,
  GETSMS_READ_SMS,
  GETSMS_OTHER_SMS,

  GETSMS_NOT_AUTH_SMS,
  GETSMS_AUTH_SMS,

  GETSMS_LAST_ITEM
};




class AT
{
  public:
    uint16_t comm_buf_len;          // num. of characters in the buffer
    byte comm_buf[COMM_BUF_LEN+1];  // communication buffer +1 for 0x00 termination
    

    // library version
    int LibVer(void);
    // constructor
    AT(void);

    // debug methods
#ifdef DEBUG_LED_ENABLED
    void BlinkDebugLED (byte num_of_blink);
#endif

#ifdef DEBUG_PRINT
    void DebugPrint(const char *string_to_print, byte last_debug_print);
    void DebugPrint(int number_to_print, byte last_debug_print);
#endif

    // serial line initialization
    void InitSerLine(long baud_rate);
    // set comm. line status
    inline void SetCommLineStatus(byte new_status) {comm_line_status = new_status;};
    // get comm. line status
    inline byte GetCommLineStatus(void) {return comm_line_status;};
    
    
    // SMS's methods 
    char InitSMSMemory(void);
    char SendSMS(char *number_str, char *message_str);
    char SendSMS(byte sim_phonebook_position, char *message_str);
    char IsSMSPresent(byte required_status);
    char GetSMS(byte position, char *phone_number, char *SMS_text, byte max_SMS_len);
    char GetAuthorizedSMS(byte position, char *phone_number, char *SMS_text, byte max_SMS_len,
                          byte first_authorized_pos, byte last_authorized_pos);
    char DeleteSMS(byte position);

    // Phonebook's methods
    char GetPhoneNumber(byte position, char *phone_number);
    char WritePhoneNumber(byte position, char *phone_number);
    char ComparePhoneNumber(byte position, char *phone_number);


    // routines regarding communication with the device
    void RxInit(uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
                byte flush_before_read, byte read_when_buffer_full);
    byte IsRxFinished(void);
    byte IsStringReceived(char const *compare_string);
    byte WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout);
    byte WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout, 
                  char const *expected_resp_string);
    char SendATCmdWaitResp(char const *AT_cmd_string,
               uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
               char const *response_string,
               byte no_of_attempts);

  private:
    byte comm_line_status;

    // variables connected with communication buffer
    byte *p_comm_buf;               // pointer to the communication buffer   
    byte rx_state;                  // internal state of rx state machine    
    uint16_t start_reception_tmout; // max tmout for starting reception
    uint16_t interchar_tmout;       // previous time in msec.
    unsigned long prev_time;        // previous time in msec.
    byte  flag_read_when_buffer_full; // flag
    
};





#endif // end of ifndef __AT_h
