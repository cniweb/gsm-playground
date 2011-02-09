/*
  Test of user buttons and LED on the GSM Playground - GSM Shield for Arduino

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
