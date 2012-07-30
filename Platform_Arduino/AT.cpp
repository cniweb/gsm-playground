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
    Println(string_to_print);
    SendATCmdWaitResp("AT", START_SHORT_COMM_TMOUT, MAX_INTERCHAR_TMOUT, "OK", 1);
  }
  else Print(string_to_print);
}

void AT::DebugPrint(int number_to_print, byte last_debug_print)
{
  Println(number_to_print);
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
  //default
  actual_baud_rate = 115200;
}

/**********************************************************
  Initialization of GSM module serial line
**********************************************************/
void AT::InitSerLine(long baud_rate)
{
  // open the serial line for the communication
  Serial.begin(baud_rate);
  actual_baud_rate = baud_rate;
  // communication line is not used yet = free
  SetCommLineStatus(CLS_FREE);
  // pointer is initialized to the first item of comm. buffer
  p_comm_buf = &comm_buf[0];
}


/**********************************************************
  Methods for sending and receiving characters through
  HW Serial port but can be redefined for using of SW serial
  port in the GPS module if necessary
**********************************************************/
void AT::Write(byte send_as_binary)
{
  Serial.write(send_as_binary);
}

void AT::Write(byte* data_buffer, unsigned short size)
{
  Serial.write(data_buffer, size);
}

void AT::Print(char const *string)
{
  Serial.print(string);
}

void AT::Println(char const *string)
{
  Serial.println(string);
}

void AT::Print(int int_value)
{
  Serial.print(int_value);
}

void AT::Println(int int_value)
{
  Serial.println(int_value);
}

int  AT::Read(void)
{
  return (Serial.read());
}

void AT::Flush(void)
{
  Serial.flush();
}

int  AT::Available(void)
{
  return (Serial.available());
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
    Flush(); // erase rx circular buffer
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
    if (!Available()) {
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
    num_of_bytes = Available();
    // if there are some received bytes postpone the timeout
    if (num_of_bytes) prev_time = millis();
      
    // read all received bytes      
    while (num_of_bytes) {
      num_of_bytes--;
      if (comm_buf_len < COMM_BUF_LEN) {
        // we have still place in the GSM internal comm. buffer =>
        // move available bytes from circular buffer 
        // to the rx buffer
        *p_comm_buf = Read();
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
        Read();
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

    Println(AT_cmd_string);
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

