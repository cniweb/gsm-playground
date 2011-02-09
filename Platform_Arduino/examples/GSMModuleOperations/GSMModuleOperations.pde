/*
 General application showing DTMF on the GSM Playground - GSM Shield for Arduino

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
#include "GSM.h"  


// definition of instance of GSM class
GSM gsm;

// variables used for timing
unsigned long	previous_timer;
byte timer100msec;
byte answer_delay;
// current DTMF
byte DTMF_value;
char phone_number[20];      // array for the phone number string
 

void setup()
{
  // initialization of serial line
  gsm.InitSerLine(115200);		

  // turn on GSM module
  gsm.TurnOn();

  #ifdef DEBUG_PRINT
    // print library version
    gsm.DebugPrint("DEBUG AT library version: ", 0);
    gsm.DebugPrint(gsm.LibVer(), 0);
    gsm.DebugPrint("DEBUG GSM library version: ", 0);
    gsm.DebugPrint(gsm.GSMLibVer(), 1);
  #endif

  // enable DTMF operations
  gsm.EnableDTMF();

  // set direction for GPIO pisn
  gsm.SetGPIODir(GPIO10, GPIO_DIR_OUT);
  gsm.SetGPIODir(GPIO11, GPIO_DIR_OUT);
  
  // periodic timer initialization
  timer100msec = 0;
  previous_timer = millis();
}

void loop()
{
  // -------------------
  // timing of main loop
  // -------------------
  if ((unsigned long)(millis() - previous_timer) >= 100) { 
    previous_timer = millis();  

    //*******************************
    //****** EVERY 100msec. *********
    //*******************************
    if (gsm.IsUserButtonEnable() && gsm.IsUserButtonPushed()) {
      // operation with the user button is enabled and user button
      // is pushed => make a call 
      // and disable user button until call is not finished
      gsm.DisableUserButton();
      gsm.TurnOffLED();
      gsm.Call(1);  // call to the first SIM position
    }

    // DTMF output value is checked every 100 msec.
    // and in case there is a required value
    // corresponding action is made
    DTMF_value = gsm.GetDTMFSignal();
    switch (DTMF_value) {
      case 1:
        // GPIO10 off
        gsm.SetGPIOVal(GPIO10, 0);
        gsm.SendDTMFSignal(1);
        break;
      case 2:
        // GPIO10 on
        gsm.SetGPIOVal(GPIO10, 1);
        gsm.SendDTMFSignal(9);
        gsm.SendDTMFSignal(9);
        gsm.SendDTMFSignal(9);
        break;
      case 3:
        // GPIO11 off
        gsm.SetGPIOVal(GPIO11, 0); 
        gsm.SendDTMFSignal(1);
        break;
      case 4:
        // GPIO11 on
        gsm.SetGPIOVal(GPIO11, 1);
        gsm.SendDTMFSignal(9);
        gsm.SendDTMFSignal(9);
        gsm.SendDTMFSignal(9);
        break;
      case 5:
        // switch off the speaker
        gsm.SetSpeaker(0); 
        break;
      case 6:
        // switch on the speaker
        gsm.SetSpeaker(1);
        break;
      case 7:
        // decrease speaker volume
        gsm.DecSpeakerVolume();
        break;
      case 8:
        // increase speaker volume
        gsm.IncSpeakerVolume();
        break;
    }
    

    
    //*******************************
    //****** EVERY 200msec. *********
    // %2 means every 200 msec.
    // +1 means - 100msec. "before" a previous 100msec. action
    // so the processor power is better spreaded
    //  
    // in case +1 is not used but only: if (timer100msec % 2 == 0)  
    // then this action will also executed every 200msec.
    // but previous EVERY 100msec. and this EVERY 200msec.
    // is executed in the same 100msec. point so the processor
    // time is not used so effectively
    //*******************************
    if ((timer100msec+1) % 2 == 0) {
      // here it is possible to place your code which will be executed
      // each 200 msec.
      // ---------------------------------------------------------
    }
    
    //*******************************
    //****** EVERY 500msec. *********
    // %5 means every 500 msec.
    // +2 means - 200msec. "before" a first 100msec. action
    // so the processor power is better spreaded
    //*******************************
    if ((timer100msec+2) % 5 == 0) {
      // here it is possible to place your code which will be executed
      // each 500 msec.
      // ---------------------------------------------------------
    }
    
    //*******************************
    //****** EVERY 1 sec. ***********
    // %10 means every 1000 msec. = 1sec.
    //*******************************
    if ((timer100msec+3) % 10 == 0) {

      gsm.CheckRegistration();
      if (gsm.IsRegistered()) {
      #ifdef DEBUG_PRINT
        gsm.DebugPrint("DEBUG GSM module is registered", 1);
      #endif
        // GSM modul is still registered:
        // So find out call status with authorization
        // with SIM phonebook at the positions 1..3
        //
        // In case we don't need authorization we can use
        // gsm.CallStatusWithAuth(phone_number, 0, 0) instead
        // and every incoming call will be authorized = picked up
        // ------------------------------------------------------
        switch (gsm.CallStatusWithAuth(phone_number, 1, 3)) {
          case CALL_NONE:
            // there is no call => enable user button
            gsm.EnableUserButton();
            gsm.TurnOnLED();
            answer_delay = 5; // 5 sec. delay before PickUp
            break;
          case CALL_INCOM_VOICE_AUTH:
            // there is incoming call from authorized phone number 
            // make some small delay and pick it up
            // and disable user button until call is not finished
            if (--answer_delay == 0) {
              gsm.DisableUserButton();
              gsm.TurnOffLED();
              gsm.PickUp();
            }
            break;
          case CALL_INCOM_VOICE_NOT_AUTH:
            // there is incoming call from not authorized phone number
            // make some small delay and hang it up
            if (--answer_delay == 0) {
              gsm.HangUp();
            }
            break;
          case CALL_ACTIVE_VOICE:
            break;
        }          
      }
      else {
        // not registered - so disable button
        // ----------------------------------
        gsm.DisableUserButton();
        gsm.TurnOffLED();
      }
    }
    
    
    
    //********************************************
    //********WRAP AROUND COUNTER 10 sec. *********
    //********************************************
    timer100msec = (timer100msec + 1) % 100;
  }	  
}
