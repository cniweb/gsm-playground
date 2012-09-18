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

// -----------------------------------------------------------------
// IMPORTANT:
// definition of GSM instance is placed here
// to enable implementation of GPS functionality as a standard class
// so the users who use the OLD GSM module without GPS functionality
// will still have GSM module without GPS methods
// -----------------------------------------------------------------
GSM gsm;




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

#ifndef GE836_GPS
    // Be aware: reset pin on the new GE836-GPS module has different functionionlity:
    // it does not mean reset(as for GE836 without GPS) but "Hardware Unconditional Shutdown"

    // reset the module GE836 just for sure
    // this is helpful mainly in situation when GSM Playground+Arduino board are reseted
    // (and not switched-off and switched-on)during development
    digitalWrite(GSM_RESET, HIGH);
    delay(400);
    digitalWrite(GSM_RESET, LOW);
    delay(500);
#endif
   

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
  char string[20];

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
      //      SendATCmdWaitResp("AT+IPR=115200", 500, 20, "OK", 5);
      sprintf(string, "AT+IPR=%li", actual_baud_rate);
      SendATCmdWaitResp(string, 500, 20, "OK", 5);
      // setup communication mode
#ifndef GE836_GPS
      SendATCmdWaitResp("AT#SELINT=1", 500, 20, "OK", 5);
#else
      SendATCmdWaitResp("AT#SELINT=2", 500, 20, "OK", 5);
#endif
      // Switch ON User LED - just as signalization we are here
      SendATCmdWaitResp("AT#GPIO=8,1,1", 500, 20, "OK", 5);
      // Sets GPIO9 as an input = user button
      SendATCmdWaitResp("AT#GPIO=9,0,0", 500, 20, "OK", 5);
      // allow audio amplifier control
      SendATCmdWaitResp("AT#GPIO=5,0,2", 500, 20, "OK", 5);
      // Switch OFF User LED- just as signalization we are finished
      SendATCmdWaitResp("AT#GPIO=8,0,1", 500, 20, "OK", 5);
      // set character set ”8859-1” - ISO 8859 Latin 1    
      //SendATCmdWaitResp("AT+CSCS=\"8859-1\"", 500, 20, "OK", 5);
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
  Println("AT+CREG?");
  // 5 sec. for initial comm tmout
  // 20 msec. for inter character timeout
  status = WaitResp(5000, MAX_MID_INTERCHAR_TMOUT); 

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
char GSM::SendSMS(char *number_str, char *message_str) 
{
  char ret_val = -1;
  byte i;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // still not send
  // try to send SMS 3 times in case there is some problem
  for (i = 0; i < 3; i++) {
    // send  AT+CMGS="number_str"
    Print("AT+CMGS=\"");
    Print(number_str);  
    Print("\"\r");

    // 1000 msec. for initial comm tmout
    // 20 msec. for inter character timeout
    if (RX_FINISHED_STR_RECV == WaitResp(START_LONG_COMM_TMOUT, MAX_INTERCHAR_TMOUT, ">")) {
      // send SMS text
      Print(message_str); 

#ifdef DEBUG_SMS_ENABLED
      // SMS will not be sent = we will not pay => good for debugging
      Write(0x1b);
      if (RX_FINISHED_STR_RECV == WaitResp(START_XXLONG_COMM_TMOUT, MAX_INTERCHAR_TMOUT, "OK")) {
#else 
      Write(0x1a);
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
char GSM::SendSMS(byte sim_phonebook_position, char *message_str) 
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
char GSM::InitSMSMemory(void) 
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
char GSM::IsSMSPresent(byte required_status) 
{
  char ret_val = -1;
  char *p_char;
  byte status;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // still not present

  switch (required_status) {
    case SMS_UNREAD:
      Print("AT+CMGL=\"REC UNREAD\"\r");
      break;
    case SMS_READ:
      Print("AT+CMGL=\"REC READ\"\r");
      break;
    case SMS_ALL:
      Print("AT+CMGL=\"ALL\"\r");
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
char GSM::GetSMS(byte position, char *phone_number, char *SMS_text, byte max_SMS_len) 
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
  Print("AT+CMGR=");
  Print((int)position);  
  Print("\r");

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
char GSM::GetAuthorizedSMS(byte position, char *phone_number, char *SMS_text, byte max_SMS_len,
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
char GSM::DeleteSMS(byte position) 
{
  char ret_val = -1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // not deleted yet
  
  //send "AT+CMGD=XY" - where XY = position
  Print("AT+CMGD=");
  Print((int)position);  
  Print("\r");


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
char GSM::GetPhoneNumber(byte position, char *phone_number)
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
  Print("AT+CPBR=");
  Print((int)position);  
  Print("\r");

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
char GSM::WritePhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // phone number was not written yet
  
  //send: AT+CPBW=XY,"00420123456789"
  // where XY = position,
  //       "00420123456789" = phone number string
  Print("AT+CPBW=");
  Print((int)position);  
  Print(",\"");
  Print(phone_number);
  Print("\"\r");

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
char GSM::ComparePhoneNumber(byte position, char *phone_number)
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
  Println("AT+CPAS");

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

  phone_number[0] = 0x00;  // no phone number so far
  if (CLS_FREE != GetCommLineStatus()) return (CALL_COMM_LINE_BUSY);
  SetCommLineStatus(CLS_ATCMD);
  Println("AT+CLCC");

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
  Println("ATA");
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
  Println("ATH");
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
  Print("ATD");
  Print(number_string);    
  Print(";\r");
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
  Print("ATD>\"SM\" ");
  Print(sim_position);    
  Print(";\r");

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
  Print("AT+CLVL=");
  Print((int)speaker_volume);    
  Print("\r"); // send <CR>
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
  Print("AT+VTS=");
  Print((int)dtmf_tone);    
  Print("\r");
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
  if (AT_RESP_OK == SendATCmdWaitResp("AT#GPIO=9,2", 2000, MAX_MID_INTERCHAR_TMOUT, "#GPIO: 0,0", 1)) {
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
  Print("AT#GPIO=");
  // pin number
  Print((int)GPIO_pin);  
  Print(",0,"); // if case pin will be OUTPUT - 
                       // first value after initialization will be 0 
  if (direction > 1) direction = 1;
  Print((int)direction); // 0-INPUT, 1-OUTPUT
  Print("\r");           // finish command = send<CR>

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
  Print("AT#GPIO=");
  // pin number
  Print((int)GPIO_pin);  
  Print(",");
  if (value > 1) value = 1;
  Print((int)value);
  Print(",1\r");  // pin is always set as output

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
  Print("AT#GPIO=");
  // pin number
  Print((int)GPIO_pin);  
  Print(",2\r");

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
#ifdef GE836_GPS
  // ADC_IN1
  if (AT_RESP_OK == SendATCmdWaitResp("AT#ADC=1,2,0", 2000, 20, "#ADC", 1)) {
#else 
  // ADC_IN2
  if (AT_RESP_OK == SendATCmdWaitResp("AT#ADC=2,2,0", 2000, 20, "#ADC", 1)) {
#endif
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