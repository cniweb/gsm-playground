/*
  GPS_GE863.h - library for the GSM-GPS Playground - GSM-GPS Shield 
  for Arduino www.hwkitchen.com

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

//#include "Debug.h"
//#include <SoftwareSerial.h>
#include "GPS_GE863.h"



extern "C" {
  #include <string.h>
}

/*
#define rxPin A4
#define txPin A5
SoftwareSerial debugserial(rxPin, txPin);
*/

/**********************************************************
Constructor
**********************************************************/
GPS_GE863::GPS_GE863(void)
{
}

/**********************************************************
Method returns GPS library version

return val: 010 means library version 0.10
            101 means library version 1.01
**********************************************************/
int GPS_GE863::GPSLibVer(void)
{
  return (GPS_LIB_VERSION);
}


/**********************************************************
Method makes a reset of GPS module

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free


        OK ret val:
        -----------
        0 - GPS module was not initialized
        1 - GPS module was initialized


an example of usage:
        GPS_GE863 gps;
        gps.ResetGPSModul(RESET); 
**********************************************************/
char GPS_GE863::ResetGPSModul(byte reset_type) 
{
  char ret_val = -1;
  char cmd[15];

//debugserial.begin(57600);
//debugserial.println("TEMP");

  if (CLS_FREE != gsm.GetCommLineStatus()) return (ret_val);
  gsm.SetCommLineStatus(CLS_ATCMD);
  // prepare command:  AT$GPSR=X ; X = "0" "1" "2" "3"
  strcpy(cmd, "AT$GPSR=");

  switch (reset_type) {
    case GPS_RESET_HW:
      strcat(cmd, "0"); // add character "0"
      break;
    case GPS_RESET_COLDSTART:
      strcat(cmd, "1"); // add character "1"
      break;
    case GPS_RESET_WARMSTART:
      strcat(cmd, "2"); // add character "2"
      break;
    case GPS_RESET_HOTSTART:
      strcat(cmd, "3"); // add character "3"
      break;
  }
#ifdef DEBUG_PRINT_DEVELOPMENT
  debug.Println(cmd);
#endif
//DEBUG_PRINTLN_STRING(cmd);
  ret_val = gsm.SendATCmdWaitResp(cmd, 5000, 100, "OK", 1);
  if (ret_val == AT_RESP_OK) {
    // OK response
    ret_val = 1;
  }
  else ret_val = 0;
#ifdef DEBUG_PRINT_DEVELOPMENT
  debug.Println(ret_val);
#endif
  gsm.SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method reads sw. version of GPS module

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free


        OK ret val:
        -----------
        0 - reading not OK
        1 - reading OK


an example of usage:
        GPS_GE863 gps;
        unsigned char sw_ver_string[50];
        gps.GetGPSSwVers(&sw_ver_string[0]); 
**********************************************************/
char GPS_GE863::GetGPSSwVers(char *sw_ver_string) 
{
  char ret_val = -1;
  char *p_resp;
  char *p_buf = (char*)&gsm.comm_buf[0];


  if (CLS_FREE != gsm.GetCommLineStatus()) return (ret_val);
  gsm.SetCommLineStatus(CLS_ATCMD);

  // send command:  AT$GPSSW
  ret_val = gsm.SendATCmdWaitResp("AT$GPSSW", 5000, 100, "$GPSSW", 1);
  if (ret_val == AT_RESP_OK) {
    // OK response
    // response example: {0D}{0A}$GPSSW: GSW3.2.4Ti_3.1.00.12-C23P1.00 {0D}{0A}{0D}{0A}OK{0D}{0A}
    // copy firmware string to buffer
    p_resp = gsm.Skip(p_buf, ':');                // Skip prolog 
    p_resp = gsm.ReadToken(p_resp, sw_ver_string, '\r');     // until first {OD}
    ret_val = 1;
  }
  else ret_val = 0;


  gsm.SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method allows to manage power-up or down of the GPS controller

command: 
        0 - GPS controller is powered down
        1 - GPS controller is powered up (default)

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free


        OK ret val:
        -----------
        0 - response is not OK
        1 - response is OK


an example of usage:
        GPS_GE863 gps;
        gps.GPSPowerUpOrDown(1); //power up the module
**********************************************************/
char GPS_GE863::GPSPowerUpOrDown(unsigned char command) 
{
  char ret_val = -1;


  if (CLS_FREE != gsm.GetCommLineStatus()) return (ret_val);
  gsm.SetCommLineStatus(CLS_ATCMD);

  if (command == 0) {
    // switch off the GPS module  
    ret_val = gsm.SendATCmdWaitResp("AT$GPSP=0", 5000, 100, "OK", 1);
  }
  else {
    // switch on the GPS module(this is a default state)  
    ret_val = gsm.SendATCmdWaitResp("AT$GPSP=1", 5000, 100, "OK", 1);
  }
  if (ret_val == AT_RESP_OK) {
    // OK response
    ret_val = 1;
  }
  else ret_val = 0;




  gsm.SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method controls GPS antenna

command: 
        0 - GPS Antenna not power supplied by the module
        1 - GPS Antenna power supplied by the module 
            (antenna is powered by default)

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free


        OK ret val:
        -----------
        0 - response is not OK
        1 - response is OK


an example of usage:
        GPS_GE863 gps;
        gps.ControlGPSAntenna(1); //power the antenna
**********************************************************/
char GPS_GE863::ControlGPSAntenna(unsigned char command) 
{
  char ret_val = -1;


  if (CLS_FREE != gsm.GetCommLineStatus()) return (ret_val);
  gsm.SetCommLineStatus(CLS_ATCMD);

  if (command == 0) {
    // send command:  AT$GPSAT=0
    ret_val = gsm.SendATCmdWaitResp("AT$GPSAT=0", 5000, 100, "OK", 1);
  }
  else {
    // send command:  AT$GPSAT=1
    ret_val = gsm.SendATCmdWaitResp("AT$GPSAT=1", 5000, 100, "OK", 1);
  }
  if (ret_val == AT_RESP_OK) {
    // OK response
    ret_val = 1;
  }
  else ret_val = 0;
  
  gsm.SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method reads GPS Antenna Supply Voltage

redout_voltage: pointer to the readout value in mV
                example: 3900 means 3900mV  

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free


        OK ret val:
        -----------
        0 - response is not OK
        1 - response is OK


an example of usage:
        GPS_GE863 gps;
        unsigned short redout_voltage;
        gps.GetGPSAntennaSupplyVoltage(&redout_voltage); 
**********************************************************/
char GPS_GE863::GetGPSAntennaSupplyVoltage(unsigned short *redout_voltage) 
{
  char ret_val = -1;

  char *p_resp;
  char *p_buf = (char*)&gsm.comm_buf[0];
  char val_str[5];

  if (CLS_FREE != gsm.GetCommLineStatus()) return (ret_val);
  gsm.SetCommLineStatus(CLS_ATCMD);

  *redout_voltage = 0; //until now
  // send command:  AT$GPSAV
  ret_val = gsm.SendATCmdWaitResp("AT$GPSAV", 5000, 100, "$GPSAV", 1);
  if (ret_val == AT_RESP_OK) {
    // OK response
    // response example: {0D}{0A}$GPSAV: 3962{0D}{0A}{0D}{0A}OK{0D}{0A}
    p_resp = gsm.Skip(p_buf, ':');                // Skip prolog 
    p_resp = gsm.ReadToken(p_resp, val_str, '\r');     // until first {OD}
    *redout_voltage = atoi(val_str);
    ret_val = 1;
  }
  else ret_val = 0;


  gsm.SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method reads GPS Antenna Current consumption


redout_current: pointer to the readout current in mA
                example: 10 means 10mA
return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free


        OK ret val:
        -----------
        0 - reading not OK
        1 - reading OK


an example of usage:
        GPS_GE863 gps;
        unsigned short current;
        gps.GetGPSAntennaCurrent(&current); 
**********************************************************/
char GPS_GE863::GetGPSAntennaCurrent(unsigned short *redout_current) 
{
  char ret_val = -1;

  char *p_resp;
  char *p_buf = (char*)&gsm.comm_buf[0];
  char val_str[5];


  if (CLS_FREE != gsm.GetCommLineStatus()) return (ret_val);
  gsm.SetCommLineStatus(CLS_ATCMD);

  *redout_current = 0; // until now
  // send command:  AT$GPSAI?
  ret_val = gsm.SendATCmdWaitResp("AT$GPSAI?", 5000, 100, "$GPSAI", 1);
  if (ret_val == AT_RESP_OK) {
    // OK response
    // response example: {0D}{0A}$GPSAI: 0{0D}{0A}{0D}{0A}OK{0D}{0A}
    p_resp = gsm.Skip(p_buf, ':');                // Skip prolog 
    p_resp = gsm.ReadToken(p_resp, val_str, '\r');     // until first {OD}
    *redout_current = atoi(val_str);
    ret_val = 1;
  }
  else ret_val = 0;


  gsm.SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method returns GPS coordinates, time and date

position: pointer to the redout GPS position

return: 
        ERROR ret. val:
        ---------------
        -1  - comm. line is not free
        0   - GPS coordinates are not OK

        OK ret val:
        -----------
        1 - GPS coordinates are OK


an example of usage:
        GPS_GE863 gps;
        Position position;
        Time time;
        Date date;
        gps.GetGPSData(&position, &time, &date); 
**********************************************************/
char GPS_GE863::GetGPSData(Position *position, Time *time, Date *date)
{
  char ret_val = -1;
  char cmd[15];

  if (CLS_FREE != gsm.GetCommLineStatus()) return (ret_val);
  gsm.SetCommLineStatus(CLS_ATCMD);

  ret_val = gsm.SendATCmdWaitResp("AT$GPSACP", 5000, 100, "", 1);
  if (ret_val == AT_RESP_ERR_DIF_RESP) {
    // there is some response
    // ----------------------
    actual_position.fix = 0;
    ParseGPS((char *)gsm.comm_buf, &actual_position, &actual_time, &actual_date);
    if (actual_position.fix > 0) {
      // coordinates were read correctly
      // -------------------------------
      ret_val = 1;
    }
    
  }
  else {
    // coordinates were not read correctly
    // -----------------------------------
    actual_position.fix = 0;
    ret_val = 0;
  }

  // copy of coordinates
  // -------------------
  *position = actual_position;
  *time = actual_time;
  *date = actual_date;


  gsm.SetCommLineStatus(CLS_FREE);
  return (ret_val);
}


/*
 * Parse the given string into a position record.
 * example:
 * $GPSACP: 120631.999,5433.9472N,00954.8768E,1.0,46.5,3,167.28,0.36,0.19,130707,11\r
 */
void GPS_GE863::ParseGPS(char *gpsMsg, Position *pos, Time *time, Date *date) {

  char time_str[7];
  char lat_buf[12];
  char lon_buf[12];
  char alt_buf[7];
  char fix;
  char date_str[7];
  char nr_sat[4];

	//$GPSACP: 120631.999,5433.9472N,00954.8768E,1.0,46.5,3,167.28,0.36,0.19,130707,11\r
  
  gpsMsg = gsm.Skip(gpsMsg, ':');                // Skip prolog
  gpsMsg = gsm.ReadToken(gpsMsg, time_str, '.');     // time, hhmmss
  gpsMsg = gsm.Skip(gpsMsg, ',');                // Skip ms
  gpsMsg = gsm.ReadToken(gpsMsg, lat_buf, ',');  // latitude
  gpsMsg = gsm.ReadToken(gpsMsg, lon_buf, ',');  // longitude
  gpsMsg = gsm.Skip(gpsMsg, ',');                // hdop
  gpsMsg = gsm.ReadToken(gpsMsg, alt_buf, ',');  // altitude
  fix = *gpsMsg++;                           // fix, 0, 2d, 3d
  gpsMsg++;
  gpsMsg = gsm.Skip(gpsMsg, ',');                // cog, cource over ground
  gpsMsg = gsm.Skip(gpsMsg, ',');                // speed [km]
  gpsMsg = gsm.Skip(gpsMsg, ',');                // speed [kn]
  gpsMsg = gsm.ReadToken(gpsMsg, date_str, ',');     // date ddmmyy
  gpsMsg = gsm.ReadToken(gpsMsg, nr_sat, '\n');  // number of sats

  if (fix != '0') {
    ParseDateTime(time, date, time_str, date_str);
    ParsePosition(pos, lat_buf, lon_buf, alt_buf);
    pos->fix = fix;
  }

}



/*
 * Parse and convert the position tokens. 
 */
void GPS_GE863::ParseDateTime(Time *time, Date *date, char *time_str, char *date_str) {
  char buf[3];

  // time string "hhmmss"
  // hours
  memcpy(&buf[0], &time_str[0], 2);
  buf[2] = 0x00;
  time->hours = atoi(buf);
  // min
  memcpy(&buf[0], &time_str[2], 2);
  buf[2] = 0x00;
  time->min = atoi(buf);
  // sec
  memcpy(&buf[0], &time_str[4], 2);
  buf[2] = 0x00;
  time->sec = atoi(buf);

  // date string "ddmmyy"
  // day
  memcpy(&buf[0], &date_str[0], 2);
  buf[2] = 0x00;
  date->day = atoi(buf);
  // month
  memcpy(&buf[0], &date_str[2], 2);
  buf[2] = 0x00;
  date->month = atoi(buf);
  // year
  memcpy(&buf[0], &date_str[4], 2);
  buf[2] = 0x00;
  date->year = atoi(buf);
  date->year += 2000;
}


/*
 * Parse and convert the position tokens. 
 */
void GPS_GE863::ParsePosition(Position *pos, char *lat_str, char *lon_str, char *alt_str) {
  char buf[10];
  ParseDegrees(lat_str, &pos->lat_deg, &pos->lat_min);
  ParseDegrees(lon_str, &pos->lon_deg, &pos->lon_min);
  gsm.ReadToken(alt_str, buf, '.');
  pos->alt = atol(buf);
}


/*
 * Parse and convert the given string into degrees and minutes.
 * Example: 5333.9472N --> 53 degrees, 33.9472 minutes
 * converted to: 53.565786 degrees 
 */
void GPS_GE863::ParseDegrees(char *str, int *degree, long *minutes) {
  char buf[6];
  byte c = 0;
  byte i = 0;
  char *tmp_str;
	
  tmp_str = str;
  while ((c = *tmp_str++) != '.') i++;
  strlcpy(buf, str, i-1);
  *degree = atoi(buf);
  tmp_str -= 3;
  i = 0;
  while (true) {
    c = *tmp_str++;
    if ((c == '\0') || (i == 5)) {
      break;
    }
    else if (c != '.') {
      buf[i++] = c;
    }
  }
  buf[i] = 0;
  *minutes = atol(buf);
  *minutes *= 16667;
  *minutes /= 1000;
}


