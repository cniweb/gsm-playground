/*
    This sketch demonstrates how to use basic GPS functionality based
    on the GE863GPS module.

    1. The GE863GPS GSM module is switched on.
    2. Internal GPS module is reset.
    3. External GPS antenna is switched on.
    4. Regularly every 500msec.:
       a) registration to the GSM network is checked 
       b) valid GPS data is checked
       c) if registration is valid AND GSP data are valid
          the user button is enabled(blue LED is switched on)
       b) if user button is enabled and pushed SMS with GPS data
          is sent to specified number
       c) if there is an incoming call from any number then call is hang up
          and SMS with GPS data is sent to incoming call number
          


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

#include "GPS_GE863.h"




// definition of instance of GPS class
GPS_GE863 gps;

// return variable
signed char ret_val;
uint16_t num_of_rx_bytes;
byte* ptr_to_data;
Position position;
Time time;
Date date;
signed char gps_data_valid;

unsigned short redout_voltage;
unsigned short redout_current;
char  phone_number[15];      // array for the phone number string
char  string[120];           // buffer for SMS with GPS data



/****************************************************************************************
 * This routine sends SMS with GPS data to the specified phone number
 
 SMS has following format:

 Lat: DD°MM´SS.SS˝<CR><LF>        means GPS Latitude
 Long: DD°MM´SS.SS˝<CR><LF>       means GPS Longitude
 Alt: xx m<CR><LF>                means GPS Altitude
 Time: hh:mm:ss<CR><LF>           means GPS Time
 Date: yyyy-mm-dd<CR><LF>         means GPS Date
 ****************************************************************************************/
void SendSMSWithGPSData(char *phone_num)
{
  unsigned char pos;

/*
  // short SMS variant
  pos = 0;
  pos = sprintf(string, "%s", "GPS:");
  gps.ConvertPosition2String(&position, PART_LATITUDE, GPS_POS_FORMAT_1, string+strlen(string));
  sprintf(string+strlen(string), "%s", " ");
  gps.ConvertPosition2String(&position, PART_LONGITUDE, GPS_POS_FORMAT_1, string+strlen(string));
  sprintf(string+strlen(string), "%s", "\r\n");
  sprintf(string+strlen(string), "%s", "D/T:");
  gps.ConvertDate2String(&date, string+strlen(string));
  sprintf(string+strlen(string), "%s", " ");
  gps.ConvertTime2String(&time, string+strlen(string));
  sprintf(string+strlen(string), "%s", "\r\n");
*/


  // longer SMS variant
  pos = 0;
  pos = sprintf(string, "%s", "Lat: "); // Latitude
  gps.ConvertPosition2String(&position, PART_LATITUDE, GPS_POS_FORMAT_1, string+strlen(string));
  sprintf(string+strlen(string), "%s", "\r\nLong: "); // Longitude
  gps.ConvertPosition2String(&position, PART_LONGITUDE, GPS_POS_FORMAT_1, string+strlen(string));
  sprintf(string+strlen(string), "%s", "\r\nAlt: ");  // Altitude
  gps.ConvertPosition2String(&position, PART_ALTITUDE, GPS_POS_FORMAT_1, string+strlen(string));
  sprintf(string+strlen(string), "%s", "\r\nTime: ");
  gps.ConvertTime2String(&time, string+strlen(string));
  sprintf(string+strlen(string), "%s", "\r\nDate: ");
  gps.ConvertDate2String(&date, string+strlen(string));

  
  gsm.SendSMS(phone_num, string);
}

void setup()
{
  // initialization of serial line
  gsm.InitSerLine(57600);
  // turn on GSM module
  gsm.TurnOn();

  
  
  #ifdef DEBUG_PRINT
    // print library version
    gsm.DebugPrint("DEBUG AT library version: ", 0);
    gsm.DebugPrint(gsm.LibVer(), 0);
    gsm.DebugPrint("DEBUG GSM library version: ", 0);
    gsm.DebugPrint(gsm.GSMLibVer(), 0);
    gsm.DebugPrint("DEBUG GPS library version: ", 0);
    gsm.DebugPrint(gps.GPSLibVer(), 1);
  #endif

  
  // reset GPS modul
  // hot start is used for faster connection to GPS
  // -----------------------------------------------
  ret_val = gps.ResetGPSModul(GPS_RESET_HOTSTART);
  //  ret_val = gps.ResetGPSModul(GPS_RESET_WARMSTART);


  // delay after initialization
  delay(5000);
  // turn on GPS antenna
  gps.ControlGPSAntenna(1); 

  // read some technical data if necessary
  /*
  gps.GetGPSAntennaSupplyVoltage(&redout_voltage); 
  gps.GetGPSAntennaCurrent (&redout_current);
  gps.GetGPSSwVers(string);
  */
}


void loop()
{
  // check registration
  // ------------------
  gsm.CheckRegistration();

  // read GPS data
  // -------------
  gps_data_valid = gps.GetGPSData(&position, &time, &date);


  // enable user button if GSM modul is connected to the network
  // and GPS modul is connected to some satelites(=GPS data are valid)
  // -----------------------------------------------------------------
  if (gsm.IsRegistered() && (gps_data_valid == 1) ) {
    // GSM modul is registered and GPS data are valid
    // ----------------------------------------------
    gsm.EnableUserButton();
    gsm.TurnOnLED();
  }
  else {
    // not registered - so disable button
    // ----------------------------------
    gsm.DisableUserButton();
    gsm.TurnOffLED();
  }


  // send SMS with GPS position if user button is pushed
  // ---------------------------------------------------
  if (gsm.IsUserButtonEnable() && gsm.IsUserButtonPushed()) {
    // uncomment following line 
    // and use your real phone number for sending a SMS about GPS data

    //SendSMSWithGPSData("123456789");
  }


  // Is there incomming call?
  // if yes hang it up and send SMS about actual GPS positions
  // ---------------------------------------------------------
  switch (gsm.CallStatusWithAuth(phone_number, 0, 0)) { // 0,0 means that we do not need authorization
                                                        // so every incomming call is regarded as valid
    case CALL_NONE:
      break;
    case CALL_INCOM_VOICE_AUTH:
      // there is incomming call and we do not need authorization
      // make some small delay and hang it up
      delay(5000);  // 5 sec. delay
      gsm.HangUp();

      SendSMSWithGPSData(phone_number);
      break;
    case CALL_ACTIVE_VOICE:
      // there is active call
      // make some small delay and hang it up
      delay(2000);  // 2 sec. delay
      gsm.HangUp();
      break;
  }   
  
   
  // wait 500msec. before next iteration
  // -----------------------------------
  delay(500);
}


