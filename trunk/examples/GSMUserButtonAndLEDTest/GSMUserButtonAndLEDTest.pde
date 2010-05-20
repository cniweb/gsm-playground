/*
 *  Test of user buttons and LED on the GSM Playground - GSM Shield for Arduino
 *  Released under the Creative Commons Attribution-Share Alike 3.0 License
 *  http://www.creativecommons.org/licenses/by-sa/3.0/
 *  www.hwkitchen.com
 */
#include "GSM.h"  


// definition of instance of GSM class
GSM gsm;


void setup()
{
  // initialization of serial line
  gsm.InitSerLine(115200);
  // turn on GSM module
  gsm.TurnOn();
  
  #ifdef DEBUG_PRINT
    // print library version
    gsm.DebugPrint("DEBUG Library version: ", 0);
    gsm.DebugPrint(gsm.LibVer(), 1);
  #endif
}

void loop()
{
  // in case user button is pushed turn on the user LED
  // otherwise turn off the user LED
  // --------------------------------------------------
  if (gsm.IsUserButtonPushed()) {
    gsm.TurnOnLED();
  }
  else gsm.TurnOffLED();
}
