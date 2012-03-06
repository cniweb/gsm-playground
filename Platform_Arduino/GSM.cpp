/*
   GSM.cpp - library for the GSM Playground - GSM Shield for Arduino
    www.hwkitchen.com

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
#include "GSM.h"

extern "C" {
  #include <string.h>
}


/**********************************************************
Method returns GSM library version

return val: 100 means library version 1.00
            101 means library version 1.01
**********************************************************/
int GSM::GSMLibVer(void)
{
  return (GSM_LIB_VERSION);
}

/**********************************************************
  Constructor definition
***********************************************************/

GSM::GSM(void)
{
  // set some GSM pins as inputs, some as outputs
  pinMode(GSM_ON, OUTPUT);               // sets pin 5 as output
  pinMode(GSM_RESET, OUTPUT);            // sets pin 4 as output

  pinMode(DTMF_OUTPUT_ENABLE, OUTPUT);   // sets pin 2 as output
  // deactivation of IC8 so DTMF is disabled by default
  digitalWrite(DTMF_OUTPUT_ENABLE, LOW);
  
  // not registered yet
  module_status = STATUS_NONE;
  
  // initialization of speaker volume
  last_speaker_volume = 0; 
}


/**********************************************************
Methods return the state of corresponding
bits in the status variable

- these methods do not communicate with the GSM module

return values: 
      0 - not true (not active)
      >0 - true (active)
**********************************************************/
byte GSM::IsRegistered(void)
{
  return (module_status & STATUS_REGISTERED);
}

byte GSM::IsInitialized(void)
{
  return (module_status & STATUS_INITIALIZED);
}

/**********************************************************
  Checks if the GSM module is responding 
  to the AT command
  - if YES  nothing is made 
  - if NO   switch on sequence is repeated until there is a response
            from GSM module
**********************************************************/
void GSM::TurnOn(void)
{
  SetCommLineStatus(CLS_ATCMD);

  while (AT_RESP_ERR_NO_RESP == SendATCmdWaitResp("AT", 500, 20, "OK", 5)) {
    // there is no response => turn on the module
  
    
    // generate switch on pulse
    digitalWrite(GSM_ON, HIGH);
    delay(1200);
    digitalWrite(GSM_ON, LOW);
    delay(1200);

#ifdef DEBUG_PRINT
    // parameter 0 - because module is off so it is not necessary 
    // to send finish AT<CR> here
    DebugPrint("DEBUG: GSM module is off\r\n", 0);
#endif

    // and reset the module
    digitalWrite(GSM_RESET, HIGH);
    delay(400);
    digitalWrite(GSM_RESET, LOW);
    delay(500);
    
    

    delay(3000); // wait before next try
  }
  SetCommLineStatus(CLS_FREE);

  // send collection of first initialization parameters for the GSM module    
  InitParam(PARAM_SET_0);
}


/**********************************************************
  Sends parameters for initialization of GSM module

  group:  0 - parameters of group 0 - not necessary to be registered in the GSM
          1 - parameters of group 1 - it is necessary to be registered
**********************************************************/
void GSM::InitParam(byte group)
{

  switch (group) {
    case PARAM_SET_0:
      // check comm line
      if (CLS_FREE != GetCommLineStatus()) return;
      SetCommLineStatus(CLS_ATCMD);

      // Reset to the factory settings
      SendATCmdWaitResp("AT&F1", 1000, 20, "OK", 5);      
      // switch off echo
      SendATCmdWaitResp("ATE0", 500, 20, "OK", 5);
      // setup fixed baud rate
      SendATCmdWaitResp("AT+IPR=115200", 500, 20, "OK", 5);
      // setup mode
      SendATCmdWaitResp("AT#SELINT=1", 500, 20, "OK", 5);
      // Switch ON User LED - just as signalization we are here
      SendATCmdWaitResp("AT#GPIO=8,1,1", 500, 20, "OK", 5);
      // Sets GPIO9 as an input = user button
      SendATCmdWaitResp("AT#GPIO=9,0,0", 500, 20, "OK", 5);
      // allow audio amplifier control
      SendATCmdWaitResp("AT#GPIO=5,0,2", 500, 20, "OK", 5);
      // Switch OFF User LED- just as signalization we are finished
      SendATCmdWaitResp("AT#GPIO=8,0,1", 500, 20, "OK", 5);
      SetCommLineStatus(CLS_FREE);
      break;

    case PARAM_SET_1:
      // check comm line
      if (CLS_FREE != GetCommLineStatus()) return;
      SetCommLineStatus(CLS_ATCMD);

      // Audio codec - Full Rate (for DTMF usage)
      SendATCmdWaitResp("AT#CODEC=1", 500, 20, "OK", 5);
      // Hands free audio path
      SendATCmdWaitResp("AT#CAP=1", 500, 20, "OK", 5);
      // Echo canceller enabled 
      SendATCmdWaitResp("AT#SHFEC=1", 500, 20, "OK", 5);
      // Ringer tone select (0 to 32)
      SendATCmdWaitResp("AT#SRS=26,0", 500, 20, "OK", 5);
      // Microphone gain (0 to 7) - response here sometimes takes 
      // more than 500msec. so 1000msec. is more safety
      SendATCmdWaitResp("AT#HFMICG=7", 1000, 20, "OK", 5);
      // set the SMS mode to text 
      SendATCmdWaitResp("AT+CMGF=1", 500, 20, "OK", 5);
      // Auto answer after first ring enabled
      // auto answer is not used
      //SendATCmdWaitResp("ATS0=1", 500, 20, "OK", 5);

      // select ringer path to handsfree
      SendATCmdWaitResp("AT#SRP=1", 500, 20, "OK", 5);
      // select ringer sound level
      SendATCmdWaitResp("AT+CRSL=2", 500, 20, "OK", 5);
      // we must release comm line because SetSpeakerVolume()
      // checks comm line if it is free
      SetCommLineStatus(CLS_FREE);
      // select speaker volume (0 to 14)
      SetSpeakerVolume(9);
      // init SMS storage
      InitSMSMemory();
      // select phonebook memory storage
      SendATCmdWaitResp("AT+CPBS=\"SM\"", 1000, 20, "OK", 5);
      break;
  }
  
}

/**********************************************************
  Enables DTMF receiver = IC8 device 
**********************************************************/
void GSM::EnableDTMF(void)
{
  // configuration of corresponding DTMF pins
  pinMode(DTMF_DATA_VALID, INPUT);       // sets pin 3 as input
  pinMode(DTMF_DATA0, INPUT);            // sets pin 6 as input
  pinMode(DTMF_DATA1, INPUT);            // sets pin 7 as input
  pinMode(DTMF_DATA2, INPUT);            // sets pin 8 as input
  pinMode(DTMF_DATA3, INPUT);            // sets pin 9 as input
  // activation of IC8 
  digitalWrite(DTMF_OUTPUT_ENABLE, HIGH);
}

/**********************************************************
Checks if DTMF signal is valid

return: >= 0x10hex(16dec) - DTMF signal is NOT valid
        0..15 - valid DTMF signal
**********************************************************/
byte GSM::GetDTMFSignal(void)
{
  static byte last_state = LOW; // initialization
  byte ret_val = 0xff;          // default - not valid

  if (digitalRead(DTMF_DATA_VALID)==HIGH
      && last_state == LOW) {
    // valid DTMF signal => decode it
    // ------------------------------
    last_state = HIGH; // remember last state

    ret_val = 0;
    if (digitalRead(DTMF_DATA0)) ret_val |= 0x01;
    if (digitalRead(DTMF_DATA1)) ret_val |= 0x02;
    if (digitalRead(DTMF_DATA2)) ret_val |= 0x04;
    if (digitalRead(DTMF_DATA3)) ret_val |= 0x08;

    // confirm DTMF signal by the tone 5
    // ---------------------------------
    SendDTMFSignal(5);
  }
  else if (digitalRead(DTMF_DATA_VALID) == LOW) {
    // pulse is not valid => enable to check DTMF signal again
    // -------------------------------------------------------
    last_state = LOW;
  }
  return (ret_val);
}

/**********************************************************
Turns on/off the speaker

off_on: 0 - off
        1 - on
**********************************************************/
void GSM::SetSpeaker(byte off_on)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  if (off_on) {
    SendATCmdWaitResp("AT#GPIO=5,1,2", 500, 20, "#GPIO:", 1);
  }
  else {
    SendATCmdWaitResp("AT#GPIO=5,0,2", 500, 20, "#GPIO:", 1);
  }
  SetCommLineStatus(CLS_FREE);
}




/**********************************************************
Method checks if the GSM module is registered in the GSM net
- this method communicates directly with the GSM module
  in contrast to the method IsRegistered() which reads the
  flag from the module_status (this flag is set inside this method)

- must be called regularly - from 1sec. to cca. 10 sec.

return values: 
      REG_NOT_REGISTERED  - not registered
      REG_REGISTERED      - GSM module is registered
      REG_NO_RESPONSE     - GSM doesn't response
      REG_COMM_LINE_BUSY  - comm line between GSM module and Arduino is not free
                            for communication
**********************************************************/
byte GSM::CheckRegistration(void)
{
  byte status;
  byte ret_val = REG_NOT_REGISTERED;

  if (CLS_FREE != GetCommLineStatus()) return (REG_COMM_LINE_BUSY);
  SetCommLineStatus(CLS_ATCMD);
  Serial.println("AT+CREG?");
  // 5 sec. for initial comm tmout
  // 20 msec. for inter character timeout
  status = WaitResp(5000, 20); 

  if (status == RX_FINISHED) {
    // something was received but what was received?
    // ---------------------------------------------
    if(IsStringReceived("+CREG: 0,1") 
      || IsStringReceived("+CREG: 0,5")) {
      // it means module is registered
      // ----------------------------
      module_status |= STATUS_REGISTERED;
    
    
      // in case GSM module is registered first time after reset
      // sets flag STATUS_INITIALIZED
      // it is used for sending some init commands which 
      // must be sent only after registration
      // --------------------------------------------
      if (!IsInitialized()) {
        module_status |= STATUS_INITIALIZED;
        SetCommLineStatus(CLS_FREE);
        InitParam(PARAM_SET_1);
      }
      ret_val = REG_REGISTERED;      
    }
    else {
      // NOT registered
      // --------------
      module_status &= ~STATUS_REGISTERED;
      ret_val = REG_NOT_REGISTERED;
    }
  }
  else {
    // nothing was received
    // --------------------
    ret_val = REG_NO_RESPONSE;
  }
  SetCommLineStatus(CLS_FREE);
 

  return (ret_val);
}

/**********************************************************
Method checks status of call

return: 
      CALL_NONE         - no call activity
      CALL_INCOM_VOICE  - incoming voice
      CALL_ACTIVE_VOICE - active voice
      CALL_NO_RESPONSE  - no response to the AT command 
      CALL_COMM_LINE_BUSY - comm line is not free
**********************************************************/
byte GSM::CallStatus(void)
{
  byte ret_val = CALL_NONE;

  if (CLS_FREE != GetCommLineStatus()) return (CALL_COMM_LINE_BUSY);
  SetCommLineStatus(CLS_ATCMD);
  Serial.println("AT+CPAS");

  // 5 sec. for initial comm tmout
  // 20 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(5000, 20)) {
    // nothing was received (RX_TMOUT_ERR)
    // -----------------------------------
    ret_val = CALL_NO_RESPONSE;
  }
  else {
    // something was received but what was received?
    // ---------------------------------------------
    // ready (device allows commands from TA/TE)
    // <CR><LF>+CPAS: 0<CR><LF> <CR><LF>OK<CR><LF>
    // unavailable (device does not allow commands from TA/TE)
    // <CR><LF>+CPAS: 1<CR><LF> <CR><LF>OK<CR><LF> 
    // unknown (device is not guaranteed to respond to instructions)
    // <CR><LF>+CPAS: 2<CR><LF> <CR><LF>OK<CR><LF> - NO CALL
    // ringing
    // <CR><LF>+CPAS: 3<CR><LF> <CR><LF>OK<CR><LF> - NO CALL
    // call in progress
    // <CR><LF>+CPAS: 4<CR><LF> <CR><LF>OK<CR><LF> - NO CALL
    if(IsStringReceived("0")) { 
      // ready - there is no call
      // ------------------------
      ret_val = CALL_NONE;
    }
    else if(IsStringReceived("3")) { 
      // incoming call
      // --------------
      ret_val = CALL_INCOM_VOICE;
    }
    else if(IsStringReceived("4")) { 
      // active call
      // -----------
      ret_val = CALL_ACTIVE_VOICE;
    }
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);

}

/**********************************************************
Method checks status of call(incoming or active) 
and makes authorization with specified SIM positions range

phone_number: a pointer where the tel. number string of current call will be placed
              so the space for the phone number string must be reserved - see example
first_authorized_pos: initial SIM phonebook position where the authorization process
                      starts
last_authorized_pos:  last SIM phonebook position where the authorization process
                      finishes

                      Note(important):
                      ================
                      In case first_authorized_pos=0 and also last_authorized_pos=0
                      the received incoming phone number is NOT authorized at all, so every
                      incoming is considered as authorized (CALL_INCOM_VOICE_NOT_AUTH is returned)

return: 
      CALL_NONE                   - no call activity
      CALL_INCOM_VOICE_AUTH       - incoming voice - authorized
      CALL_INCOM_VOICE_NOT_AUTH   - incoming voice - not authorized
      CALL_ACTIVE_VOICE           - active voice
      CALL_INCOM_DATA_AUTH        - incoming data call - authorized
      CALL_INCOM_DATA_NOT_AUTH    - incoming data call - not authorized  
      CALL_ACTIVE_DATA            - active data call
      CALL_NO_RESPONSE            - no response to the AT command 
      CALL_COMM_LINE_BUSY         - comm line is not free
**********************************************************/
byte GSM::CallStatusWithAuth(char *phone_number,
                             byte first_authorized_pos, byte last_authorized_pos)
{
  byte ret_val = CALL_NONE;
  byte search_phone_num = 0;
  byte i;
  byte status;
  char *p_char; 
  char *p_char1;

  phone_number[0] = 0x00;  // no phonr number so far
  if (CLS_FREE != GetCommLineStatus()) return (CALL_COMM_LINE_BUSY);
  SetCommLineStatus(CLS_ATCMD);
  Serial.println("AT+CLCC");

  // 5 sec. for initial comm tmout
  // and max. 1500 msec. for inter character timeout
  RxInit(5000, 1500, 1, 1);
  // wait response is finished
  do {
    if (IsStringReceived("OK\r\n")) { 
      // perfect - we have some response, but what:

      // there is either NO call:
      // <CR><LF>OK<CR><LF>

      // or there is at least 1 call
      // +CLCC: 1,1,4,0,0,"+420XXXXXXXXX",145<CR><LF>
      // <CR><LF>OK<CR><LF>
      status = RX_FINISHED;
      break; // so finish receiving immediately and let's go to 
             // to check response 
    }
    status = IsRxFinished();
  } while (status == RX_NOT_FINISHED);

  // generate tmout 30msec. before next AT command
  delay(30);

  if (status == RX_FINISHED) {
    // something was received but what was received?
    // example: //+CLCC: 1,1,4,0,0,"+420XXXXXXXXX",145
    // ---------------------------------------------
    if(IsStringReceived("+CLCC: 1,1,4,0,0")) { 
      // incoming VOICE call - not authorized so far
      // -------------------------------------------
      search_phone_num = 1;
      ret_val = CALL_INCOM_VOICE_NOT_AUTH;
    }
    else if(IsStringReceived("+CLCC: 1,1,4,1,0")) { 
      // incoming DATA call - not authorized so far
      // ------------------------------------------
      search_phone_num = 1;
      ret_val = CALL_INCOM_DATA_NOT_AUTH;
    }
    else if(IsStringReceived("+CLCC: 1,0,0,0,0")) { 
      // active VOICE call - GSM is caller
      // ----------------------------------
      search_phone_num = 1;
      ret_val = CALL_ACTIVE_VOICE;
    }
    else if(IsStringReceived("+CLCC: 1,1,0,0,0")) { 
      // active VOICE call - GSM is listener
      // -----------------------------------
      search_phone_num = 1;
      ret_val = CALL_ACTIVE_VOICE;
    }
    else if(IsStringReceived("+CLCC: 1,1,0,1,0")) { 
      // active DATA call - GSM is listener
      // ----------------------------------
      search_phone_num = 1;
      ret_val = CALL_ACTIVE_DATA;
    }
    else if(IsStringReceived("+CLCC:")){ 
      // other string is not important for us - e.g. GSM module activate call
      // etc.
      // IMPORTANT - each +CLCC:xx response has also at the end
      // string <CR><LF>OK<CR><LF>
      ret_val = CALL_OTHERS;
    }
    else if(IsStringReceived("OK")){ 
      // only "OK" => there is NO call activity
      // --------------------------------------
      ret_val = CALL_NONE;
    }

    
    // now we will search phone num string
    if (search_phone_num) {
      // extract phone number string
      // ---------------------------
      p_char = strchr((char *)(comm_buf),'"');
      p_char1 = p_char+1; // we are on the first phone number character
      p_char = strchr((char *)(p_char1),'"');
      if (p_char != NULL) {
        *p_char = 0; // end of string
        strcpy(phone_number, (char *)(p_char1));
      }
      
      if ( (ret_val == CALL_INCOM_VOICE_NOT_AUTH) 
           || (ret_val == CALL_INCOM_DATA_NOT_AUTH)) {

        if ((first_authorized_pos == 0) && (last_authorized_pos == 0)) {
          // authorization is not required => it means authorization is OK
          // -------------------------------------------------------------
          if (ret_val == CALL_INCOM_VOICE_NOT_AUTH) ret_val = CALL_INCOM_VOICE_AUTH;
          else ret_val = CALL_INCOM_DATA_AUTH;
        }
        else {
          // make authorization
          // ------------------
          SetCommLineStatus(CLS_FREE);
          for (i = first_authorized_pos; i <= last_authorized_pos; i++) {
            if (ComparePhoneNumber(i, phone_number)) {
              // phone numbers are identical
              // authorization is OK
              // ---------------------------
              if (ret_val == CALL_INCOM_VOICE_NOT_AUTH) ret_val = CALL_INCOM_VOICE_AUTH;
              else ret_val = CALL_INCOM_DATA_AUTH;
              break;  // and finish authorization
            }
          }
        }
      }
    }
    
  }
  else {
    // nothing was received (RX_TMOUT_ERR)
    // -----------------------------------
    ret_val = CALL_NO_RESPONSE;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method picks up an incoming call

return: 
**********************************************************/
void GSM::PickUp(void)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  Serial.println("ATA");
  SetCommLineStatus(CLS_FREE);
}

/**********************************************************
Method hangs up incoming or active call

return: 
**********************************************************/
void GSM::HangUp(void)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  Serial.println("ATH");
  SetCommLineStatus(CLS_FREE);
}

/**********************************************************
Method calls the specific number

number_string: pointer to the phone number string 
               e.g. gsm.Call("+420123456789"); 

**********************************************************/
void GSM::Call(char *number_string)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  // ATDxxxxxx;<CR>
  Serial.print("ATD");
  Serial.print(number_string);    
  Serial.print(";\r");
  // 10 sec. for initial comm tmout
  // 20 msec. for inter character timeout
  WaitResp(10000, 20);
  SetCommLineStatus(CLS_FREE);
}

/**********************************************************
Method calls the number stored at the specified SIM position

sim_position: position in the SIM <1...>
              e.g. gsm.Call(1);
**********************************************************/
void GSM::Call(int sim_position)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  // ATD>"SM" 1;<CR>
  Serial.print("ATD>\"SM\" ");
  Serial.print(sim_position);    
  Serial.print(";\r");

  // 10 sec. for initial comm tmout
  // 20 msec. for inter character timeout
  WaitResp(10000, 20);

  SetCommLineStatus(CLS_FREE);
}

/**********************************************************
Method sets speaker volume

speaker_volume: volume in range 0..14

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..14 current speaker volume 
**********************************************************/
char GSM::SetSpeakerVolume(byte speaker_volume)
{
  
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // remember set value as last value
  if (speaker_volume > 14) speaker_volume = 14;
  // select speaker volume (0 to 14)
  // AT+CLVL=X<CR>   X<0..14>
  Serial.print("AT+CLVL=");
  Serial.print((int)speaker_volume);    
  Serial.print("\r"); // send <CR>
  // 10 sec. for initial comm tmout
  // 20 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(10000, 20)) {
    ret_val = -2; // ERROR
  }
  else {
    if(IsStringReceived("OK")) {
      last_speaker_volume = speaker_volume;
      ret_val = last_speaker_volume; // OK
    }
    else ret_val = -3; // ERROR
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method increases speaker volume

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..14 current speaker volume 
**********************************************************/
char GSM::IncSpeakerVolume(void)
{
  char ret_val;
  byte current_speaker_value;

  current_speaker_value = last_speaker_volume;
  if (current_speaker_value < 14) {
    current_speaker_value++;
    ret_val = SetSpeakerVolume(current_speaker_value);
  }
  else ret_val = 14;

  return (ret_val);
}

/**********************************************************
Method decreases speaker volume

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..14 current speaker volume 
**********************************************************/
char GSM::DecSpeakerVolume(void)
{
  char ret_val;
  byte current_speaker_value;

  current_speaker_value = last_speaker_volume;
  if (current_speaker_value > 0) {
    current_speaker_value--;
    ret_val = SetSpeakerVolume(current_speaker_value);
  }
  else ret_val = 0;

  return (ret_val);
}

/**********************************************************
Method sends DTMF signal
This function only works when call is in progress

dtmf_tone: tone to send 0..15

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0.. tone
**********************************************************/
char GSM::SendDTMFSignal(byte dtmf_tone)
{
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // e.g. AT+VTS=5<CR>
  Serial.print("AT+VTS=");
  Serial.print((int)dtmf_tone);    
  Serial.print("\r");
  // 1 sec. for initial comm tmout
  // 20 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(1000, 20)) {
    ret_val = -2; // ERROR
  }
  else {
    if(IsStringReceived("OK")) {
      ret_val = dtmf_tone; // OK
    }
    else ret_val = -3; // ERROR
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}


/**********************************************************
Method returns state of user button


return: 0 - not pushed = released
        1 - pushed
**********************************************************/
byte GSM::IsUserButtonPushed(void)
{
  byte ret_val = 0;
  if (CLS_FREE != GetCommLineStatus()) return(0);
  SetCommLineStatus(CLS_ATCMD);
  if (AT_RESP_OK == SendATCmdWaitResp("AT#GPIO=9,2", 500, 20, "#GPIO: 0,0", 1)) {
    // user button is pushed
    ret_val = 1;
  }
  else ret_val = 0;
  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Turns on the user LED
**********************************************************/
void GSM::TurnOnLED(void)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  // response here is not important
  SendATCmdWaitResp("AT#GPIO=8,1,1", 500, 20, "#GPIO:", 1);
  SetCommLineStatus(CLS_FREE);
}

/**********************************************************
Turns off the user LED
**********************************************************/
void GSM::TurnOffLED(void)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  // response here is not important
  SendATCmdWaitResp("AT#GPIO=8,0,1", 500, 20, "#GPIO:", 1);
  SetCommLineStatus(CLS_FREE);
}



/**********************************************************
Method sets direction for GPIO pins GPIO10, GPIO11, GPIO12, GPIO13

gpio_pin:   10..13
direction:  0 - input
            1 - output

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0 - set as INPUT
        1 - set as OUTPUT
**********************************************************/
char GSM::SetGPIODir(byte GPIO_pin, byte direction)
{
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);

  // e.g. AT#GPIO=9,0,0 - sets as INPUT
  // e.g. AT#GPIO=9,0,1 - sets as OUTPUT and value is "0" - LOW
  Serial.print("AT#GPIO=");
  // pin number
  Serial.print((int)GPIO_pin);  
  Serial.print(",0,"); // if case pin will be OUTPUT - 
                       // first value after initialization will be 0 
  if (direction > 1) direction = 1;
  Serial.print((int)direction); // 0-INPUT, 1-OUTPUT
  Serial.print("\r");           // finish command = send<CR>

  // 100 msec. for initial comm tmout
  // 20 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(100, 20)) {
    ret_val = -2; // ERROR
  }
  else {
    if(IsStringReceived("OK")) {
      ret_val = direction;  // OK
    }
    else ret_val = -3;      // ERROR
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method sets GPIO pin according required value
it doesn't check if the pin was previously set as OUTPUT
so INPUT pin is automatically switch
as the output pin by using this function 

gpio_pin:   10..13
value:      0 - "0" as LOW
            1 - "1" as HIGH

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0 - set to the "0" - LOW
        1 - set to the "1" - HIGH
**********************************************************/
char GSM::SetGPIOVal(byte GPIO_pin, byte value)
{
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);


  // e.g. AT#GPIO=9,0,1 - set to "0" - LOW
  // or   AT#GPIO=9,1,1 - set to "1" - HIGH
  Serial.print("AT#GPIO=");
  // pin number
  Serial.print((int)GPIO_pin);  
  Serial.print(",");
  if (value > 1) value = 1;
  Serial.print((int)value);
  Serial.print(",1\r");  // pin is always set as output

  // 100 msec. for initial comm tmout
  // 20 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(100, 20)) {
    ret_val = -2; // ERROR
  }
  else {
    if(IsStringReceived("OK")) {
      ret_val = value; // OK
    }
    else ret_val = -3; // ERROR
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}


/**********************************************************
Method gets GPIO pin last state

gpio_pin:   10..13
value:      0 - "0" as LOW
            1 - "1" as HIGH

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0 - pin is in the "0" - LOW state
        1 - pin is in the "1" - HIGH state
**********************************************************/
char GSM::GetGPIOVal(byte GPIO_pin)
{
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);


  //e.g. AT#GPIO=9,2
  Serial.print("AT#GPIO=");
  // pin number
  Serial.print((int)GPIO_pin);  
  Serial.print(",2\r");

  // 100 msec. for initial comm tmout
  // 20 msec. for inter character timeout
  // required answer: #GPIO: 0,1 - input pin set to 1(HIGH)
  //                  #GPIO: 0,0 - input pin set to 0(LOW)
  if (RX_TMOUT_ERR == WaitResp(100, 20)) {
    ret_val = -2; // ERROR
  }
  else {
    // there is response but which one..
    // ---------------------------------
    if(IsStringReceived("#GPIO: 0,1")) {
      ret_val = 1; // OK and 1 as HIGH
    }
    else if(IsStringReceived("#GPIO: 0,0")) {
      ret_val = 0; // OK and 0 as LOW
    }
    else ret_val = -3; // else response ERROR
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method gets temperature measured by the Telit GSM module

the temperature is calculated by the formula:
T[C] = (Vin-600)/10
where Vin is the millivolts value

return: 
        ERROR ret. val:
        ---------------
        -1000 - comm. line to the GSM module is not free
        -2000 - GSM temperature answer is not valid

        OK ret val:
        -----------
        temperature in the C in the format XXX ~ XX,XC
        so e.g. 235 means   23.5 C
                -100 means  -10.0 C
**********************************************************/
int GSM::GetTemp(void)
{
  int ret_val = -1000;
  char *pos;

  if (CLS_FREE != GetCommLineStatus()) return(ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = -2000; // we do not have right value yet

  // response is in the format: #ADC: 885
  if (AT_RESP_OK == SendATCmdWaitResp("AT#ADC=2,2,0", 500, 20, "#ADC", 1)) {
    // parse the received string
    pos = strchr((char *)comm_buf, ' ');
    if (pos != NULL) {
      // we are in front of the number
      ret_val = atoi(pos);
      ret_val = ret_val - 600;
    }
  }
 
  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}


/**********************************************************
Functions for manipulation with strings
**********************************************************/
/*
 * Skips the string until the given char is found.
 */
char *GSM::Skip(char *str, char match) {
  byte c = 0;
  while (true) {
    c = *str++;
    if ((c == match) || (c == '\0')) {
      break;
    }
  }
  return str;
}


/*
 * Reads a token from the given string. Token is seperated by the 
 * given delimiter.
 */
char *GSM::ReadToken(char *str, char *buf, char delimiter) {
  byte c = 0;
  while (true) {
    c = *str++;
    if ((c == delimiter) || (c == '\0')) {
      break;
    }
    else if (c != ' ') {
      *buf++ = c;
    }
  }
  *buf = '\0';
  return str;
}