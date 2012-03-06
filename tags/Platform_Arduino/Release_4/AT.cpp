/*
    AT.cpp - library for AT-Communication with GSM-devices

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


#include "Arduino.h"
#include "AT.h"

extern "C" {
  #include <string.h>
}



#ifdef DEBUG_LED_ENABLED
  int DEBUG_LED = 13;                // LED connected to digital pin 13

void  AT::BlinkDebugLED (byte num_of_blink)
  {
    byte i;

    pinMode(DEBUG_LED, OUTPUT);      // sets the digital pin as output
    for (i = 0; i < num_of_blink; i++) {
      digitalWrite(DEBUG_LED, HIGH);   // sets the LED on
      delay(50);
      digitalWrite(DEBUG_LED, LOW);   // sets the LED off
      delay(500);
    }
  }
#endif

#ifdef DEBUG_PRINT
/**********************************************************
Two methods print out debug information to the standard output
- it means to the serial line.
First method prints string.
Second method prints integer numbers.

Note:
=====
The serial line is connected to the GSM module and is 
used for sending AT commands. There is used "trick" that GSM
module accepts not valid AT command strings because it doesn't
understand them and still waits for some valid AT command.
So after all debug strings are sent we send just AT<CR> as
a valid AT command and GSM module responds by OK. So previous 
debug strings are overwritten and GSM module is not influenced
by these debug texts 


string_to_print:  pointer to the string to be print out
last_debug_print: 0 - this is not last debug info, we will
                      continue with sending... so don't send
                      AT<CR>(see explanation above)
                  1 - we are finished with sending debug info 
                      for this time and finished AT<CR> 
                      will be sent(see explanation above)

**********************************************************/
void AT::DebugPrint(const char *string_to_print, byte last_debug_print)
{
  if (last_debug_print) {
    Serial.println(string_to_print);
    SendATCmdWaitResp("AT", START_SHORT_COMM_TMOUT, MAX_INTERCHAR_TMOUT, "OK", 1);
  }
  else Serial.print(string_to_print);
}

void AT::DebugPrint(int number_to_print, byte last_debug_print)
{
  Serial.println(number_to_print);
  if (last_debug_print) {
    SendATCmdWaitResp("AT", START_SHORT_COMM_TMOUT, MAX_INTERCHAR_TMOUT, "OK", 1);
  }
}
#endif


/**********************************************************
Method returns AT library version

return val: 010 means library version 0.10
            101 means library version 1.01
**********************************************************/
int AT::LibVer(void)
{
  return (AT_LIB_VERSION);
}

AT::AT(void)
{
}

/**********************************************************
  Initialization of GSM module serial line
**********************************************************/
void AT::InitSerLine(long baud_rate)
{
  // open the serial line for the communication
  Serial.begin(baud_rate);
  // communication line is not used yet = free
  SetCommLineStatus(CLS_FREE);
  // pointer is initialized to the first item of comm. buffer
  p_comm_buf = &comm_buf[0];
}

/**********************************************************
  Initializes receiving process

  start_comm_tmout    - maximum waiting time for receiving the first response
                        character (in msec.)
  max_interchar_tmout - maximum tmout between incoming characters 
                        in msec.
  flush_before_read   - 1: circular rx buffer is flushed before reading 
                        (this is a standard usage)
                        0: circular rx buffer is not flushed before reading
                        (this is used when the data(GPRS) connection is activated)
      
  read_when_buffer_full - 1: reading continues until specified max_interchar_tmout
                             is reached(standard usage) 
                          0: reading is stopped either buffer is full or specified max_interchar_tmout
                             is reached(used when data(GPRS) connection is activated)
      
  if there is no other incoming character longer then specified
  tmout(in msec) receiving process is considered as finished
**********************************************************/
void AT::RxInit(uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
                 byte flush_before_read, byte read_when_buffer_full)
{
  rx_state = RX_NOT_STARTED;
  start_reception_tmout = start_comm_tmout;
  interchar_tmout = max_interchar_tmout;
  prev_time = millis();
  comm_buf[0] = 0x00; // end of string
  p_comm_buf = &comm_buf[0];
  comm_buf_len = 0;
  if (flush_before_read) {
    Serial.flush(); // erase rx circular buffer
  }
  flag_read_when_buffer_full = read_when_buffer_full; 
}

/**********************************************************
Method checks if receiving process is finished or not.
Rx process is finished if defined inter-character tmout is reached

returns:
        RX_NOT_FINISHED = 0,// not finished yet
        RX_FINISHED,        // finished - inter-character tmout occurred
        RX_TMOUT_ERR,       // initial communication tmout occurred
**********************************************************/
byte AT::IsRxFinished(void)
{
  byte num_of_bytes;
  byte ret_val = RX_NOT_FINISHED;  // default not finished

  // Rx state machine
  // ----------------

  if (rx_state == RX_NOT_STARTED) {
    // Reception is not started yet - check tmout
    if (!Serial.available()) {
      // still no character received => check timeout
      if ((unsigned long)(millis() - prev_time) >= start_reception_tmout) {
        // timeout elapsed => GSM module didn't start with response
        // so communication is takes as finished
        comm_buf[comm_buf_len] = 0x00;
        ret_val = RX_TMOUT_ERR;
      }
    }
    else {
      // at least one character received => so init inter-character 
      // counting process again and go to the next state
      prev_time = millis(); // init tmout for inter-character space
      rx_state = RX_ALREADY_STARTED;
    }
  }

  if (rx_state == RX_ALREADY_STARTED) {
    // Reception already started
    // check new received bytes
    // only in case we have place in the buffer
    num_of_bytes = Serial.available();
    // if there are some received bytes postpone the timeout
    if (num_of_bytes) prev_time = millis();
      
    // read all received bytes      
    while (num_of_bytes) {
      num_of_bytes--;
      if (comm_buf_len < COMM_BUF_LEN) {
        // we have still place in the GSM internal comm. buffer =>
        // move available bytes from circular buffer 
        // to the rx buffer
        *p_comm_buf = Serial.read();
        p_comm_buf++;
        comm_buf_len++;
        comm_buf[comm_buf_len] = 0x00;  // and finish currently received characters
                                        // so after each character we have
                                        // valid string finished by the 0x00
      }
      else if (flag_read_when_buffer_full) {
        // comm buffer is full, other incoming characters
        // will be discarded 
        // but despite of we have no place for other characters 
        // we still must to wait until  
        // inter-character tmout is reached
        
        // so just readout character from circular RS232 buffer 
        // to find out when communication id finished(no more characters
        // are received in inter-char timeout)
        Serial.read();
      }
      else {
        // buffer is full and we are in the data state => finish 
        // receiving and dont check timeout further
        ret_val = RX_FINISHED;
        break;  
      }
    }

    // finally check the inter-character timeout 
    if ((unsigned long)(millis() - prev_time) >= interchar_tmout) {
      // timeout between received character was reached
      // reception is finished
      // ---------------------------------------------
      comm_buf[comm_buf_len] = 0x00;  // for sure finish string again
                                      // but it is not necessary
      ret_val = RX_FINISHED;
    }
  }
  return (ret_val);
}

/**********************************************************
Method checks received bytes

compare_string - pointer to the string which should be find

return: 0 - string was NOT received
        1 - string was received
**********************************************************/
byte AT::IsStringReceived(char const *compare_string)
{
  char *ch;
  byte ret_val = 0;

  if(comm_buf_len) {
    ch = strstr((char *)comm_buf, compare_string);
    if (ch != NULL) {
      ret_val = 1;
    }
  }

  return (ret_val);
}

/**********************************************************
Method waits for response

      start_comm_tmout    - maximum waiting time for receiving the first response
                            character (in msec.)
      max_interchar_tmout - maximum tmout between incoming characters 
                            in msec.  
return: 
      RX_FINISHED         finished, some character was received

      RX_TMOUT_ERR        finished, no character received 
                          initial communication tmout occurred
**********************************************************/
byte AT::WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout)
{
  byte status;

  RxInit(start_comm_tmout, max_interchar_tmout, 1, 1);
  // wait until response is not finished
  do {
    status = IsRxFinished();
  } while (status == RX_NOT_FINISHED);
  return (status);
}


/**********************************************************
Method waits for response with specific response string
    
      start_comm_tmout    - maximum waiting time for receiving the first response
                            character (in msec.)
      max_interchar_tmout - maximum tmout between incoming characters 
                            in msec.  
      expected_resp_string - expected string
return: 
      RX_FINISHED_STR_RECV,     finished and expected string received
      RX_FINISHED_STR_NOT_RECV  finished, but expected string not received
      RX_TMOUT_ERR              finished, no character received 
                                initial communication tmout occurred
**********************************************************/
byte AT::WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout, 
                   char const *expected_resp_string)
{
  byte status;
  byte ret_val;

  RxInit(start_comm_tmout, max_interchar_tmout, 1, 1);
  // wait until response is not finished
  do {
    status = IsRxFinished();
  } while (status == RX_NOT_FINISHED);

  if (status == RX_FINISHED) {
    // something was received but what was received?
    // ---------------------------------------------
    if(IsStringReceived(expected_resp_string)) {
      // expected string was received
      // ----------------------------
      ret_val = RX_FINISHED_STR_RECV;      
    }
    else ret_val = RX_FINISHED_STR_NOT_RECV;
  }
  else {
    // nothing was received
    // --------------------
    ret_val = RX_TMOUT_ERR;
  }
  return (ret_val);
}


/**********************************************************
Method sends AT command and waits for response

return: 
      AT_RESP_ERR_NO_RESP = -1,   // no response received
      AT_RESP_ERR_DIF_RESP = 0,   // response_string is different from the response
      AT_RESP_OK = 1,             // response_string was included in the response
**********************************************************/
char AT::SendATCmdWaitResp(char const *AT_cmd_string,
                uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
                char const *response_string,
                byte no_of_attempts)
{
  byte status;
  char ret_val = AT_RESP_ERR_NO_RESP;
  byte i;

  for (i = 0; i < no_of_attempts; i++) {
    // delay 500 msec. before sending next repeated AT command 
    // so if we have no_of_attempts=1 tmout will not occurred
    if (i > 0) delay(AT_DELAY); 

    Serial.println(AT_cmd_string);
    status = WaitResp(start_comm_tmout, max_interchar_tmout); 
    if (status == RX_FINISHED) {
      // something was received but what was received?
      // ---------------------------------------------
      if(IsStringReceived(response_string)) {
        ret_val = AT_RESP_OK;      
        break;  // response is OK => finish
      }
      else ret_val = AT_RESP_ERR_DIF_RESP;
    }
    else {
      // nothing was received
      // --------------------
      ret_val = AT_RESP_ERR_NO_RESP;
    }
    
  }


  return (ret_val);
}

/**********************************************************
Method sends SMS

number_str:   pointer to the phone number string
message_str:  pointer to the SMS text string


return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0 - SMS was not sent
        1 - SMS was sent


an example of usage:
        GSM gsm;
        gsm.SendSMS("00XXXYYYYYYYYY", "SMS text");
**********************************************************/
char AT::SendSMS(char *number_str, char *message_str) 
{
  char ret_val = -1;
  byte i;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // still not send
  // try to send SMS 3 times in case there is some problem
  for (i = 0; i < 3; i++) {
    // send  AT+CMGS="number_str"
    Serial.print("AT+CMGS=\"");
    Serial.print(number_str);  
    Serial.print("\"\r");

    // 1000 msec. for initial comm tmout
    // 20 msec. for inter character timeout
    if (RX_FINISHED_STR_RECV == WaitResp(START_LONG_COMM_TMOUT, MAX_INTERCHAR_TMOUT, ">")) {
      // send SMS text
      Serial.print(message_str); 

#ifdef DEBUG_SMS_ENABLED
      // SMS will not be sent = we will not pay => good for debugging
      Serial.write(0x1b);
      if (RX_FINISHED_STR_RECV == WaitResp(START_XXLONG_COMM_TMOUT, MAX_INTERCHAR_TMOUT, "OK")) {
#else 
      Serial.write(0x1a);
      if (RX_FINISHED_STR_RECV == WaitResp(START_XXLONG_COMM_TMOUT, MAX_INTERCHAR_TMOUT, "+CMGS")) {
#endif
        // SMS was send correctly 
        ret_val = 1;
        break;
      }
      else continue;
    }
    else {
      // try again
      continue;
    }
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method sends SMS to the specified SIM phonebook position

sim_phonebook_position:   SIM phonebook position <1..20>
message_str:              pointer to the SMS text string


return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - specified position must be > 0

        OK ret val:
        -----------
        0 - SMS was not sent
        1 - SMS was sent


an example of usage:
        GSM gsm;
        gsm.SendSMS(1, "SMS text");
**********************************************************/
char AT::SendSMS(byte sim_phonebook_position, char *message_str) 
{
  char ret_val = -1;
  char sim_phone_number[20];

  ret_val = 0; // SMS is not send yet
  if (sim_phonebook_position == 0) return (-3);
  if (1 == GetPhoneNumber(sim_phonebook_position, sim_phone_number)) {
    // there is a valid number at the spec. SIM position
    // => send SMS
    // -------------------------------------------------
    ret_val = SendSMS(sim_phone_number, message_str);
  }
  return (ret_val);

}

/**********************************************************
Method initializes memory for the incoming SMS in the Telit
module - SMSs will be stored in the SIM card

!!This function is used internally after first registration
so it is not necessary to used it in the user sketch

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free

        OK ret val:
        -----------
        0 - SMS memory was not initialized
        1 - SMS memory was initialized

**********************************************************/
char AT::InitSMSMemory(void) 
{
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // not initialized yet
  
  // Disable messages about new SMS from the GSM module 
  SendATCmdWaitResp("AT+CNMI=2,0", START_LONG_COMM_TMOUT, MAX_INTERCHAR_TMOUT, "OK", 2);

  // send AT command to init memory for SMS in the SIM card
  // response:
  // +CPMS: <usedr>,<totalr>,<usedw>,<totalw>,<useds>,<totals>
  if (AT_RESP_OK == SendATCmdWaitResp("AT+CPMS=\"SM\",\"SM\",\"SM\"", START_LONG_COMM_TMOUT, START_LONG_COMM_TMOUT, "+CPMS:", 10)) {
    ret_val = 1;
  }
  else ret_val = 0;

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method finds out if there is present at least one SMS with
specified status

Note:
if there is new SMS before IsSMSPresent() is executed
this SMS has a status UNREAD and then
after calling IsSMSPresent() method status of SMS
is automatically changed to READ

required_status:  SMS_UNREAD  - new SMS - not read yet
                  SMS_READ    - already read SMS                  
                  SMS_ALL     - all stored SMS

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout

        OK ret val:
        -----------
        0 - there is no SMS with specified status
        1..20 - position where SMS is stored 
                (suitable for the function GetGSM())


an example of use:
        GSM gsm;
        char position;  
        char phone_number[20]; // array for the phone number string
        char sms_text[100];

        position = gsm.IsSMSPresent(SMS_UNREAD);
        if (position) {
          // read new SMS
          gsm.GetSMS(position, phone_number, sms_text, 100);
          // now we have phone number string in phone_num
          // and SMS text in sms_text
        }
**********************************************************/
char AT::IsSMSPresent(byte required_status) 
{
  char ret_val = -1;
  char *p_char;
  byte status;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // still not present

  switch (required_status) {
    case SMS_UNREAD:
      Serial.print("AT+CMGL=\"REC UNREAD\"\r");
      break;
    case SMS_READ:
      Serial.print("AT+CMGL=\"REC READ\"\r");
      break;
    case SMS_ALL:
      Serial.print("AT+CMGL=\"ALL\"\r");
      break;
  }

  // 5 sec. for initial comm tmout
  // and max. 1500 msec. for inter character timeout
  RxInit(START_XLONG_COMM_TMOUT, MAX__LONG_INTERCHAR_TMOUT, 1, 1); 
  // wait response is finished
  do {
    if (IsStringReceived("OK")) { 
      // perfect - we have some response, but what:

      // there is either NO SMS:
      // <CR><LF>OK<CR><LF>

      // or there is at least 1 SMS
      // +CMGL: <index>,<stat>,<oa/da>,,[,<tooa/toda>,<length>]
      // <CR><LF> <data> <CR><LF>OK<CR><LF>
      status = RX_FINISHED;
      break; // so finish receiving immediately and let's go to 
             // to check response 
    }
    status = IsRxFinished();
  } while (status == RX_NOT_FINISHED);

  


  switch (status) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED:
      // something was received but what was received?
      // ---------------------------------------------
      if(IsStringReceived("+CMGL:")) { 
        // there is some SMS with status => get its position
        // response is:
        // +CMGL: <index>,<stat>,<oa/da>,,[,<tooa/toda>,<length>]
        // <CR><LF> <data> <CR><LF>OK<CR><LF>
        p_char = strchr((char *)comm_buf,':');
        if (p_char != NULL) {
          ret_val = atoi(p_char+1);
        }
      }
      else {
        // other response like OK or ERROR
        ret_val = 0;
      }

      // here we have WaitResp() just for generation tmout 20msec. in case OK was detected
      // not due to receiving
      WaitResp(START_TINY_COMM_TMOUT, MAX_INTERCHAR_TMOUT); 
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}


/**********************************************************
Method reads SMS from specified memory(SIM) position

position:     SMS position <1..20>
phone_number: a pointer where the phone number string of received SMS will be placed
              so the space for the phone number string must be reserved - see example
SMS_text  :   a pointer where SMS text will be placed
max_SMS_len:  maximum length of SMS text excluding also string terminating 0x00 character
              
return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - specified position must be > 0

        OK ret val:
        -----------
        GETSMS_NO_SMS       - no SMS was not found at the specified position
        GETSMS_UNREAD_SMS   - new SMS was found at the specified position
        GETSMS_READ_SMS     - already read SMS was found at the specified position
        GETSMS_OTHER_SMS    - other type of SMS was found 


an example of usage:
        GSM gsm;
        char position;
        char phone_num[20]; // array for the phone number string
        char sms_text[100]; // array for the SMS text string

        position = gsm.IsSMSPresent(SMS_UNREAD);
        if (position) {
          // there is new SMS => read it
          gsm.GetGSM(position, phone_num, sms_text, 100);
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG SMS phone number: ", 0);
            gsm.DebugPrint(phone_num, 0);
            gsm.DebugPrint("\r\n          SMS text: ", 0);
            gsm.DebugPrint(sms_text, 1);
          #endif
        }        
**********************************************************/
char AT::GetSMS(byte position, char *phone_number, char *SMS_text, byte max_SMS_len) 
{
  char ret_val = -1;
  char *p_char; 
  char *p_char1;
  byte len;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  phone_number[0] = 0;  // end of string for now
  ret_val = GETSMS_NO_SMS; // still no SMS
  
  //send "AT+CMGR=X" - where X = position
  Serial.print("AT+CMGR=");
  Serial.print((int)position);  
  Serial.print("\r");

  // 5000 msec. for initial comm tmout
  // 100 msec. for inter character tmout
  switch (WaitResp(START_XLONG_COMM_TMOUT, MAX_MID_INTERCHAR_TMOUT, "+CMGR")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // OK was received => there is NO SMS stored in this position
      if(IsStringReceived("OK")) {
        // there is only response <CR><LF>OK<CR><LF> 
        // => there is NO SMS
        ret_val = GETSMS_NO_SMS;
      }
      else if(IsStringReceived("ERROR")) {
        // error should not be here but for sure
        ret_val = GETSMS_NO_SMS;
      }
      break;

    case RX_FINISHED_STR_RECV:
      // find out what was received exactly

      //response for new SMS:
      //<CR><LF>+CMGR: "REC UNREAD","+XXXXXXXXXXXX",,"02/03/18,09:54:28+40"<CR><LF>
		  //There is SMS text<CR><LF>OK<CR><LF>
      if(IsStringReceived("\"REC UNREAD\"")) { 
        // get phone number of received SMS: parse phone number string 
        // +XXXXXXXXXXXX
        // -------------------------------------------------------
        ret_val = GETSMS_UNREAD_SMS;
      }
      //response for already read SMS = old SMS:
      //<CR><LF>+CMGR: "REC READ","+XXXXXXXXXXXX",,"02/03/18,09:54:28+40"<CR><LF>
		  //There is SMS text<CR><LF>
      else if(IsStringReceived("\"REC READ\"")) {
        // get phone number of received SMS
        // --------------------------------
        ret_val = GETSMS_READ_SMS;
      }
      else {
        // other type like stored for sending.. 
        ret_val = GETSMS_OTHER_SMS;
      }

      // extract phone number string
      // ---------------------------
      p_char = strchr((char *)(comm_buf),',');
      p_char1 = p_char+2; // we are on the first phone number character
      p_char = strchr((char *)(p_char1),'"');
      if (p_char != NULL) {
        *p_char = 0; // end of string
        strcpy(phone_number, (char *)(p_char1));
      }


      // get SMS text and copy this text to the SMS_text buffer
      // ------------------------------------------------------
      p_char = strchr(p_char+1, 0x0a);  // find <LF>
      if (p_char != NULL) {
        // next character after <LF> is the first SMS character
        p_char++; // now we are on the first SMS character 

        // find <CR> as the end of SMS string
        p_char1 = strchr((char *)(p_char), 0x0d);  
        if (p_char1 != NULL) {
          // finish the SMS text string 
          // because string must be finished for right behaviour 
          // of next strcpy() function
          *p_char1 = 0; 
        }
        // in case there is not finish sequence <CR><LF> because the SMS is
        // too long (more then 130 characters) sms text is finished by the 0x00
        // directly in the WaitResp() routine

        // find out length of the SMS (excluding 0x00 termination character)
        len = strlen(p_char);

        if (len < max_SMS_len) {
          // buffer SMS_text has enough place for copying all SMS text
          // so copy whole SMS text
          // from the beginning of the text(=p_char position) 
          // to the end of the string(= p_char1 position)
          strcpy(SMS_text, (char *)(p_char));
        }
        else {
          // buffer SMS_text doesn't have enough place for copying all SMS text
          // so cut SMS text to the (max_SMS_len-1)
          // (max_SMS_len-1) because we need 1 position for the 0x00 as finish 
          // string character
          memcpy(SMS_text, (char *)(p_char), (max_SMS_len-1));
          SMS_text[max_SMS_len] = 0; // finish string
        }
      }
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method reads SMS from specified memory(SIM) position and
makes authorization - it means SMS phone number is compared
with specified SIM phonebook position(s) and in case numbers
match GETSMS_AUTH_SMS is returned, otherwise GETSMS_NOT_AUTH_SMS
is returned

position:     SMS position to be read <1..20>
phone_number: a pointer where the tel. number string of received SMS will be placed
              so the space for the phone number string must be reserved - see example
SMS_text  :   a pointer where SMS text will be placed
max_SMS_len:  maximum length of SMS text excluding terminating 0x00 character

first_authorized_pos: initial SIM phonebook position where the authorization process
                      starts
last_authorized_pos:  last SIM phonebook position where the authorization process
                      finishes

                      Note(important):
                      ================
                      In case first_authorized_pos=0 and also last_authorized_pos=0
                      the received SMS phone number is NOT authorized at all, so every
                      SMS is considered as authorized (GETSMS_AUTH_SMS is returned)
              
return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        GETSMS_NO_SMS           - no SMS was found at the specified position
        GETSMS_NOT_AUTH_SMS     - NOT authorized SMS found at the specified position
        GETSMS_AUTH_SMS         - authorized SMS found at the specified position


an example of usage:
        GSM gsm;
        char phone_num[20]; // array for the phone number string
        char sms_text[100]; // array for the SMS text string

        // authorize SMS with SIM phonebook positions 1..3
        if (GETSMS_AUTH_SMS == gsm.GetAuthorizedSMS(1, phone_num, sms_text, 100, 1, 3)) {
          // new authorized SMS was detected at the SMS position 1
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG SMS phone number: ", 0);
            gsm.DebugPrint(phone_num, 0);
            gsm.DebugPrint("\r\n          SMS text: ", 0);
            gsm.DebugPrint(sms_text, 1);
          #endif
        }

        // don't authorize SMS with SIM phonebook at all
        if (GETSMS_AUTH_SMS == gsm.GetAuthorizedSMS(1, phone_num, sms_text, 100, 0, 0)) {
          // new SMS was detected at the SMS position 1
          // because authorization was not required
          // SMS is considered authorized
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG SMS phone number: ", 0);
            gsm.DebugPrint(phone_num, 0);
            gsm.DebugPrint("\r\n          SMS text: ", 0);
            gsm.DebugPrint(sms_text, 1);
          #endif
        }
**********************************************************/
char AT::GetAuthorizedSMS(byte position, char *phone_number, char *SMS_text, byte max_SMS_len,
                           byte first_authorized_pos, byte last_authorized_pos)
{
  char ret_val = -1;
  byte i;

#ifdef DEBUG_PRINT
    DebugPrint("DEBUG GetAuthorizedSMS\r\n", 0);
    DebugPrint("      #1: ", 0);
    DebugPrint(position, 0);
    DebugPrint("      #5: ", 0);
    DebugPrint(first_authorized_pos, 0);
    DebugPrint("      #6: ", 0);
    DebugPrint(last_authorized_pos, 1);
#endif  

  ret_val = GetSMS(position, phone_number, SMS_text, max_SMS_len);
  if (ret_val < 0) {
    // here is ERROR return code => finish
    // -----------------------------------
  }
  else if (ret_val == GETSMS_NO_SMS) {
    // no SMS detected => finish
    // -------------------------
  }
  else if (ret_val == GETSMS_READ_SMS) {
    // now SMS can has only READ attribute because we have already read
    // this SMS at least once by the previous function GetSMS()
    //
    // new READ SMS was detected on the specified SMS position =>
    // make authorization now
    // ---------------------------------------------------------
    if ((first_authorized_pos == 0) && (last_authorized_pos == 0)) {
      // authorization is not required => it means authorization is OK
      // -------------------------------------------------------------
      ret_val = GETSMS_AUTH_SMS;
    }
    else {
      ret_val = GETSMS_NOT_AUTH_SMS;  // authorization not valid yet
      for (i = first_authorized_pos; i <= last_authorized_pos; i++) {
        if (ComparePhoneNumber(i, phone_number)) {
          // phone numbers are identical
          // authorization is OK
          // ---------------------------
          ret_val = GETSMS_AUTH_SMS;
          break;  // and finish authorization
        }
      }
    }
  }
  return (ret_val);
}


/**********************************************************
Method deletes SMS from the specified SMS position

position:     SMS position <1..20>

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - SMS was not deleted
        1 - SMS was deleted
**********************************************************/
char AT::DeleteSMS(byte position) 
{
  char ret_val = -1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // not deleted yet
  
  //send "AT+CMGD=XY" - where XY = position
  Serial.print("AT+CMGD=");
  Serial.print((int)position);  
  Serial.print("\r");


  // 5000 msec. for initial comm tmout
  // 20 msec. for inter character timeout
  switch (WaitResp(START_XLONG_COMM_TMOUT, MAX_INTERCHAR_TMOUT, "OK")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED_STR_RECV:
      // OK was received => SMS deleted
      ret_val = 1;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // other response: e.g. ERROR => SMS was not deleted
      ret_val = 0; 
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}


/**********************************************************
Method reads phone number string from specified SIM position

position:     SMS position <1..20>

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0
        phone_number is empty string

        OK ret val:
        -----------
        0 - there is no phone number on the position
        1 - phone number was found
        phone_number is filled by the phone number string finished by 0x00
                     so it is necessary to define string with at least
                     15 bytes(including also 0x00 termination character)

an example of usage:
        GSM gsm;
        char phone_num[20]; // array for the phone number string

        if (1 == gsm.GetPhoneNumber(1, phone_num)) {
          // valid phone number on SIM pos. #1 
          // phone number string is copied to the phone_num array
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone number: ", 0);
            gsm.DebugPrint(phone_num, 1);
          #endif
        }
        else {
          // there is not valid phone number on the SIM pos.#1
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG there is no phone number", 1);
          #endif
        }
**********************************************************/
char AT::GetPhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;

  char *p_char; 
  char *p_char1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // not found yet
  phone_number[0] = 0; // phone number not found yet => empty string
  
  //send "AT+CPBR=XY" - where XY = position
  Serial.print("AT+CPBR=");
  Serial.print((int)position);  
  Serial.print("\r");

  // 5000 msec. for initial comm tmout
  // 20 msec. for inter character timeout
  switch (WaitResp(START_XLONG_COMM_TMOUT, MAX_INTERCHAR_TMOUT, "+CPBR")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED_STR_RECV:
      // response in case valid phone number stored:
      // <CR><LF>+CPBR: <index>,<number>,<type>,<text><CR><LF>
      // <CR><LF>OK<CR><LF>

      // response in case there is not phone number:
      // <CR><LF>OK<CR><LF>
      p_char = strchr((char *)(comm_buf),'"');
      if (p_char != NULL) {
        p_char++;       // we are on the first phone number character
        // find out '"' as finish character of phone number string
        p_char1 = strchr((char *)(p_char),'"');
        if (p_char1 != NULL) {
          *p_char1 = 0; // end of string
        }
        // extract phone number string
        strcpy(phone_number, (char *)(p_char));
        // output value = we have found out phone number string
        ret_val = 1;
      }
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // only OK or ERROR => no phone number
      ret_val = 0; 
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method writes phone number string to the specified SIM position

position:     SMS position <1..20>
phone_number: phone number string for the writing

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - phone number was not written
        1 - phone number was written
**********************************************************/
char AT::WritePhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // phone number was not written yet
  
  //send: AT+CPBW=XY,"00420123456789"
  // where XY = position,
  //       "00420123456789" = phone number string
  Serial.print("AT+CPBW=");
  Serial.print((int)position);  
  Serial.print(",\"");
  Serial.print(phone_number);
  Serial.print("\"\r");

  // 5000 msec. for initial comm tmout
  // 20 msec. for inter character timeout
  switch (WaitResp(START_XLONG_COMM_TMOUT, MAX_INTERCHAR_TMOUT, "OK")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      break;

    case RX_FINISHED_STR_RECV:
      // response is OK = has been written
      ret_val = 1;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // other response: e.g. ERROR
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Function compares specified phone number string 
with phone number stored at the specified SIM position

position:       SMS position <1..20>
phone_number:   phone number string which should be compare

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - phone numbers are different
        1 - phone numbers are the same


an example of usage:
        if (1 == gsm.ComparePhoneNumber(1, "123456789")) {
          // the phone num. "123456789" is stored on the SIM pos. #1
          // phone number string is copied to the phone_num array
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone numbers are the same", 1);
          #endif
        }
        else {
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone numbers are different", 1);
          #endif
        }
**********************************************************/
char AT::ComparePhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;
  char sim_phone_number[20];

#ifdef DEBUG_PRINT
    DebugPrint("DEBUG ComparePhoneNumber\r\n", 0);
    DebugPrint("      #1: ", 0);
    DebugPrint(position, 0);
    DebugPrint("      #2: ", 0);
    DebugPrint(phone_number, 1);
#endif


  ret_val = 0; // numbers are not the same so far
  if (position == 0) return (-3);
  if (1 == GetPhoneNumber(position, sim_phone_number)) {
    // there is a valid number at the spec. SIM position
    // -------------------------------------------------
    if (0 == strcmp(phone_number, sim_phone_number)) {
      // phone numbers are the same
      // --------------------------
#ifdef DEBUG_PRINT
    DebugPrint("DEBUG ComparePhoneNumber: Phone numbers are the same", 1);
#endif
      ret_val = 1;
    }
  }
  return (ret_val);
}
