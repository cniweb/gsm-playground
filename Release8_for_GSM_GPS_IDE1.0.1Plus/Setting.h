/*
    Setting.h - setting for GSM-GPS Playground Shield V2.0
              - also valid for old GSM Playground Shield
              

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



#ifndef __SETTING_h
#define __SETTING_h

// must be ENABLED(= #define GE836_GPS) if new GSM-GPS Playground Shield V2.0 is used
// must be DISABLED(= //#define GE836_GPS) if old GSM Playground is used
// ----------------------------------------------------------------------------------
#define GE836_GPS


// Simple debug print
// Keep in mind that DEBUG PRINT has only very limited functionality
// and can not be 100% guaranteed becuase all debug strings are sent also to the 
// GSM module and thus can be interpreted as AT commands.
// -------------------------------------------------------------
//#define DEBUG_PRINT


// if defined - debug LED is enabled, otherwise debug LED is disabled
// -------------------------------------------------------------
//#define DEBUG_LED_ENABLED



// if defined - SMSs are not send(are finished by the character 0x1b
// which causes that SMS are not sent)
// by this way it is possible to develop program without paying for SMSs 
// -------------------------------------------------------------
//#define DEBUG_SMS_ENABLED




#endif // end of ifndef __SETTING_h
