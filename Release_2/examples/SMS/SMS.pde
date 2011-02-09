/*
    SMS with GSM Playground - GSM Shield for Arduino

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
  Important:
  ==========
  This sketch step by step reads and erases all SMSs from your SIM
  card so if there are some important SMSs stored in your
  SIM card please backup them up before inserting SIM card
  to the GSM Playground
*/

#include "GSM.h"  

// max length for SMS buffer(including also string terminator 0x00)
// here SMS can have max. 99 characters (1 character is reserved for
// string termination 0x00)
#define SMS_MAX_LEN 100

// definition of instance of GSM class
GSM gsm;

// variables used for timing
unsigned long	previous_timer;
byte timer100msec;

int val;
char string[30];
char position;          
char phone_number[20];      // array for the phone number string
char sms_text[SMS_MAX_LEN]; // array for the SMS text
char *ch;                   // pointer to the character
 

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

  // set direction for GPIO pins
  gsm.SetGPIODir(GPIO10, GPIO_DIR_OUT);
  gsm.SetGPIODir(GPIO11, GPIO_DIR_OUT);
  
  // periodic timer initialization
  timer100msec = 0;
  previous_timer = millis();
  // we are not registered so far => disable button
  gsm.DisableUserButton();
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
      // is pushed => send SMS
      // and disable user button until SMS is not sent 
      gsm.DisableUserButton();
      gsm.TurnOffLED();

      // read temperature
      val = gsm.GetTemp();
      if (val > -1000) {
        // temperature is OK -> send SMS to the specified 
        // phone number
        // ----------------------------------------------
        
        // prepare a string which will be send by the SMS:
        // "Temperature: 25 C"  
        sprintf(string, "Temperature: %i C", val/10);

        // in case you want to send SMS to the specific number
        // change 123456789 to your phone number
         gsm.SendSMS("123456789", string); 

        // in case you want to send SMS to the specific SIM phonebook position
        // here we send to the first SIM phonebook position
        //gsm.SendSMS(1, string);
      }
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

      // is the GSM module registered?
      // -----------------------------
      gsm.CheckRegistration();
      if (gsm.IsRegistered()) {

        // GSM modul is still registered
        // -----------------------------
        gsm.EnableUserButton();
        gsm.TurnOnLED();
      }
      else {
        // not registered - so disable button
        // ----------------------------------
        gsm.DisableUserButton();
        gsm.TurnOffLED();
      }
    }


    //*******************************
    //****** EVERY 3 sec. ***********
    // %30 means every 3000 msec. = 3sec.
    //*******************************
    if ((timer100msec+4) % 30 == 0) {

      // is there a new UNREAD SMS ?
      // if YES - SIM position > 0 is returned
      // -------------------------------------
      if (gsm.IsRegistered()) {
        // GSM module is registered

        // Note: if there is new SMS before IsSMSPresent() is executed
        // this SMS has a status UNREAD
        // after calling IsSMSPresent() method status of SMS
        // is automatically changed to READ
   
        position = gsm.IsSMSPresent(SMS_ALL);
        if (position > 0) {
          // we have new SMS 
          // now we will make authorization with SIM phonebook pos. 1,2,3
          // ------------------------------------------------------------
          if (GETSMS_AUTH_SMS == gsm.GetAuthorizedSMS(position, phone_number, sms_text, SMS_MAX_LEN,
                                                      1, 3)) {

            // SMS comes from authorized phone number 
            // so lets check SMS text
            // --------------------------------------

            // 1) e.g. text "Temp?"
            // --------------------
            ch = strstr(sms_text, "Temp?");
            if (ch != NULL) {
              // there is text Temp? => sends SMS with temperature back
              // read temperature
              val = gsm.GetTemp();
              sprintf(string, "Temperature: %i C", val/10);
              gsm.SendSMS(phone_number, string);
            }

            // 2) e.g. text "GPIO10 ON"
            // -------------------------
            ch = strstr(sms_text, "GPIO10 ON");
            if (ch != NULL) {
              // turn on the GPIO10
              gsm.SetGPIOVal(GPIO10, 1);

              // and send confirmation back
              strcpy(string, "GPIO10 turned on");
              gsm.SendSMS(phone_number, string);
            }

            // 3) e.g. text "GPIO10 OFF"
            // -------------------------
            ch = strstr(sms_text, "GPIO10 OFF");
            if (ch != NULL) {
              // turn off the GPIO10
              gsm.SetGPIOVal(GPIO10, 0);


              // and send confirmation back
              strcpy(string, "GPIO10 turned off");
              gsm.SendSMS(phone_number, string);
            }
          }

          // and delete received SMS 
          // to leave place for next new SMS's
          // ---------------------------------
          gsm.DeleteSMS(position);
        }
      }
    }
    
    
    
    //********************************************
    //********WRAP AROUND COUNTER 10 sec. ********
    //********************************************
    timer100msec = (timer100msec + 1) % 100;
  }	  
}
