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
#include <SoftwareSerial.h>
#include "GPS_GE863.h"



extern "C" {
  #include <string.h>
}


#include "SoftwareSerial.h"
#define rxPin A4
#define txPin A5
SoftwareSerial debugserial(rxPin, txPin);


/**********************************************************
Constructor
**********************************************************/
GPS_GE863::GPS_GE863(void)
{
debugserial.begin(57600);
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

reset_type: use symbolic constant for

            GPS_RESET_HW
            GPS_RESET_COLDSTART
            GPS_RESET_WARMSTART
            GPS_RESET_HOTSTART


            from manual:
            -----------
            0 - Hardware reset: the GPS receiver is reset and restarts 
                by using the values stored in the internal memory of the GPS receiver.
            1 - Coldstart (No Almanac, No Ephemeris): this option clears all data 
                that is currently stored in the internal memory of the GPS receiver 
                including position, almanac, ephemeris, and time. The stored clock drift however, is retained.
                It is available in controlled mode only.
            2 - Warmstart (No ephemeris): this option clears all initialization data in the GPS receiver
                and subsequently reloads the data that is currently displayed 
                in the Receiver Initialization Setup screen. The almanac is retained but the ephemeris is cleared. 
                It is available in controlled mode only.
            3 - Hotstart (with stored Almanac and Ephemeris): the GPS receiver restarts by using 
                the values stored in the internal memory of the GPS receiver; validated ephemeris and almanac. 
                It is available in controlled mode only.

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
        gps.ResetGPSModul(GPS_RESET_WARMSTART); 
**********************************************************/
char GPS_GE863::ResetGPSModul(byte reset_type) 
{
  char ret_val = -1;
  char cmd[15];

//debugserial.begin(57600);
debugserial.println("Debug: GPS_GE863.cpp");

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
Method returns:
 1) actual GPS coordinates in raw values(must be converter latter)
 2) actual time
 3) actual date

position: pointer to the redout GPS position
time:     pointer to the redout GPS time
date:     pointer to the redout GPS date 

return: 
        ERROR ret. val:
        ---------------
        -1  - comm. line is not free
        0   - GPS coordinates are not OK(not valid yet)

        OK ret val:
        -----------
        1 - GPS coordinates are valid


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

debugserial.println("GPS_GE863::GetGPSData();");

  if (CLS_FREE != gsm.GetCommLineStatus()) return (ret_val);
  gsm.SetCommLineStatus(CLS_ATCMD);

  ret_val = gsm.SendATCmdWaitResp("AT$GPSACP", 5000, 100, "", 1);
debugserial.print("  -->ret_val: ");
debugserial.println(ret_val, DEC);
  if (ret_val == AT_RESP_OK) {
    // there is some response
    // ----------------------
debugserial.println("  -->ParseGPS();");
    actual_position.fix = 0;
    ParseGPS((char *)&gsm.comm_buf[0], &actual_position, &actual_time, &actual_date);
    if (actual_position.fix > 0) {
      // coordinates were read correctly
      // -------------------------------
      ret_val = 1;
    }
    else {
      // GSP data still not valid
      // ------------------------
      ret_val = 0;
    }
    
  }
  else {
    // coordinates were not read correctly
    // -----------------------------------
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


/**********************************************************
Method parse the given "GPS" string into a position, time and date record

input:
 gpsMsg: pointer to "GPS" string like $GPSACP: 120631.999,5433.9472N,00954.8768E,1.0,46.5,3,167.28,0.36,0.19,130707,11\r


return: 
        pos:  pointer to position structure where position is extracted from gpsMsg string
        time: pointer to time structure where time is extracted from gpsMsg string
        date: pointer to date structure where date is extracted from gpsMsg string
 **********************************************************/
void GPS_GE863::ParseGPS(char *gpsMsg, Position *pos, Time *time, Date *date) {

  char time_str[7];
  char lat_buf[12];
  char lon_buf[12];
  char alt_buf[7];
  char fix;
  char date_str[7];
  char nr_sat[4];

	//$GPSACP: 120631.999,5433.9472N,00954.8768E,1.0,46.5,3,167.28,0.36,0.19,130707,11\r
debugserial.println("GPS_GE863::ParseGPS();");  
debugserial.print("  -->gpsMsg: ");
debugserial.println(gpsMsg);

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


debugserial.print("  -->time_str: ");
debugserial.println(time_str);
debugserial.print("  -->lat_buf: ");
debugserial.println(lat_buf);
debugserial.print("  -->lon_buf: ");
debugserial.println(lon_buf);
debugserial.print("  -->date_str: ");
debugserial.println(date_str);




  if (fix != '0') {
    ParseDateTime(time_str, date_str, time, date);
    ParsePosition(lat_buf, lon_buf, alt_buf, pos);
    pos->fix = fix - '0';
  }
  else {
    pos->fix = 0;
  }
debugserial.print("  -->fix: ");
debugserial.println(pos->fix, DEC);

}



/*
 * Parse and convert the position tokens. 
 */
void GPS_GE863::ParseDateTime(char *time_str, char *date_str, Time *time, Date *date) {
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
void GPS_GE863::ParsePosition(char *lat_str, char *lon_str, char *alt_str, Position *pos) {
  char buf[10];

  ParseRawPosition(lat_str, &pos->latitude_raw, &pos->latitude_dir);
  ParseRawPosition(lon_str, &pos->longitude_raw, &pos->longitude_dir);
  gsm.ReadToken(alt_str, buf, '.');
  pos->altitude = atol(buf);
}




/*
 * Parse and convert the given string into raw position.
 * Example: 5333.9472N
 * converted to: 53339472
  */
void GPS_GE863::ParseRawPosition(
char *pos_str,  // latitute/longitude string in formats
                // latitude:   "ddmm.mmmmX"   where dd - degrees(00.90), 
                //                                  mm.mmmm - minutes(00.0000..59.9999)
                //                                  X = N or S
                //              
                // longitude: "dddmm.mmmmX"   where ddd - degrees(000..180), 
                //                                  mm.mmmm - minutes(00.0000..59.9999)
                //                                  X = E or W                                  

unsigned long *raw_position,  // "raw" mean that latitude or longitude string is just convert to long variable 1 to 1
                              // it means that e.g. "5333.9472N" is convert to long variable: 53339472
char *direction_char          // for latitude:  "N" or "S"
                              // for longitude: "E" or "W"
) 
{
  char *parseptr;
  unsigned long local_raw_pos;

  parseptr = pos_str;
  local_raw_pos = atol(parseptr);
  if (local_raw_pos != 0) {
    local_raw_pos *= 10000;
    parseptr = strchr(parseptr, '.')+1;
    local_raw_pos += atol(parseptr);
  }
  *raw_position = local_raw_pos;

  // read latitude N/S or longitude E/W data
  *direction_char = parseptr[4]; 
}


/*
 * Gets specified position part in a specified format 
 *
 * Generally there are 3 types of representation of GPS data which are frequently used
   (D=degree, M=minute, S=second):

    1) DD°MM´SS.SS½ - supported now
                      
    2) DD°MM.MMM´ - not supported yet

    3) DD.DDDDD° - not supported yet
 */


long GPS_GE863::GetPositionPart(
Position *position,     // position structure 
char part,              //  for latitude data use symbolic name PART_LATITUDE
                        //      longitude use symbolic name     PART_LONGITUDE
char format             // format: use symbolic constant
                        //#define FORMAT_DEG            0 // degree
                        //#define FORMAT_MIN            1 // minute
                        //#define FORMAT_SEC            2 // seconds
                        //#define FORMAT_0POINT01_SEC   3 // hundredth of seconds
                                // it means result:  1 = 0.01 sec.
                                //                  10 = 0.10 sec.    
)
{
  long ret_val;

  if (part == PART_LATITUDE) {
    switch (format) {
      case FORMAT_DEG:
        ret_val = position->latitude_raw / 1000000;
        break;
      case FORMAT_MIN:
        ret_val = (position->latitude_raw / 10000) % 100;
        break;
      case FORMAT_SEC:
        ret_val = (position->latitude_raw % 10000) * 6 / 1000;
        break;
      case FORMAT_0POINT01_SEC:
        ret_val = ((position->latitude_raw % 10000) * 6 / 10) % 100;
        break;
      default:
        ret_val = -1; // not valid
        break;
    }
  }
  else if (part == PART_LONGITUDE) {
    switch (format) {
      case FORMAT_DEG:
        ret_val = position->longitude_raw / 1000000;
        break;
      case FORMAT_MIN:
        ret_val = (position->longitude_raw / 10000) % 100;
        break;
      case FORMAT_SEC:
        ret_val = (position->longitude_raw % 10000) * 6 / 1000;
        break;
      case FORMAT_0POINT01_SEC:
        ret_val = ((position->longitude_raw % 10000) * 6 / 10) % 100;
        break;
      default:
        ret_val = -1; // not valid
        break;
    }
  }

  return(ret_val);
}

/****************************************************************************************
  Converts position part(latitude or longitude) into specified string
 
  Generally there are 3 types of representation of GPS data which are frequently used
   (D=degree, M=minute, S=second):

  format: use symbolic names for PART_LATITUDE and PART_LONGITUDE: 
    GPS_POS_FORMAT_1          => format DD°MM´SS.SS½ - supported now
                      
    GPS_POS_FORMAT_2          => format DD°MM.MMM´ - not supported yet

    GPS_POS_FORMAT_3          => format DD.DDDDD° - not supported yet
 ****************************************************************************************/
void GPS_GE863::ConvertPosition2String(
Position *position,     // position structure 
char part,              //  for latitude use symbolic name  PART_LATITUDE
                        //  for longitude use symbolic name PART_LONGITUDE
                        //  for altitude use symbolic name PART_ALTITUDE
char format,            // format: use symbolic constant
                        //#define GPS_POS_FORMAT_1  1 // format DD°MM´SS.SS½
char *out_pos_string    // pointer to created string
)
{
  unsigned char pos;

  switch (part) {
    case PART_LATITUDE:
      switch (format) {
        case GPS_POS_FORMAT_1:
          if (out_pos_string != NULL) {
            if (position->fix == 0) {
              sprintf(out_pos_string, "%s", "GPS data are not valid yet."); 
            }
            else {
              /*
              //this doe not work on Arduino: there is probably a lot of arguments
              //so next solution is used instead
              sprintf(out_pos_string, "%d°%d'%d.%d\" %c", 
                                     GetPositionPart(position, PART_LATITUDE, FORMAT_DEG),
                                     GetPositionPart(position, PART_LATITUDE, FORMAT_MIN),
                                     GetPositionPart(position, PART_LATITUDE, FORMAT_SEC),
                                     GetPositionPart(position, PART_LATITUDE, FORMAT_0POINT01_SEC),
                                     position->latitude_dir);
              */
              pos = 0;
              pos = sprintf(out_pos_string, "%dd", GetPositionPart(position, PART_LATITUDE, FORMAT_DEG)); // "d" as degree as we can not use "°" in SMS
              pos += sprintf(out_pos_string+pos, "%d'", GetPositionPart(position, PART_LATITUDE, FORMAT_MIN));
              pos += sprintf(out_pos_string+pos, "%d.", GetPositionPart(position, PART_LATITUDE, FORMAT_SEC));
              pos += sprintf(out_pos_string+pos, "%02d\"", GetPositionPart(position, PART_LATITUDE, FORMAT_0POINT01_SEC));
              sprintf(out_pos_string+pos, "%c", position->latitude_dir);
            }
          }
          break;

        default:
          break;
      }
      break;
    
    case PART_LONGITUDE:
      switch (format) {
        case GPS_POS_FORMAT_1:
          if (out_pos_string != NULL) {
            if (position->fix == 0) {
              //sprintf(out_pos_string, "%s", "GPS data are not valid yet.");
            }
            else {
              /*
              //this doe not work on Arduino: there is probably a lot of arguments
              //so next solution is used instead
              sprintf(out_pos_string, "%d°%d'%d.%d\" %c", 
                                     GetPositionPart(position, PART_LONGITUDE, FORMAT_DEG),
                                     GetPositionPart(position, PART_LONGITUDE, FORMAT_MIN),
                                     GetPositionPart(position, PART_LONGITUDE, FORMAT_SEC),
                                     GetPositionPart(position, PART_LONGITUDE, FORMAT_0POINT01_SEC),
                                     position->longitude_dir);
              */

              pos = 0;
              pos = sprintf(out_pos_string, "%dd", GetPositionPart(position, PART_LONGITUDE, FORMAT_DEG)); // "d" as degree as we can not use "°" in SMS
              pos += sprintf(out_pos_string+pos, "%d'", GetPositionPart(position, PART_LONGITUDE, FORMAT_MIN));
              pos += sprintf(out_pos_string+pos, "%d.", GetPositionPart(position, PART_LONGITUDE, FORMAT_SEC));
              pos += sprintf(out_pos_string+pos, "%02d\"", GetPositionPart(position, PART_LONGITUDE, FORMAT_0POINT01_SEC));
              sprintf(out_pos_string+pos, "%c", position->longitude_dir);
            }
          }
          break;

        default:
          break;
      }
      break;

    case PART_ALTITUDE:
      if (out_pos_string != NULL) {
        if (position->fix == 0) {
          sprintf(out_pos_string, "%s", "GPS data are not valid yet.");
        }
        else {
          sprintf(out_pos_string, "%d m", position->altitude);
        }
      }
      break;

    default:
      break;
  }
}


/****************************************************************************************
 * Converts Time data to string
 
 output: time string "HH:MM:SS" (= hours:minutes:seconds)
 ****************************************************************************************/
void GPS_GE863::ConvertTime2String (
Time *time,               // time data structure
char *time_string         // output string
)
{
  unsigned char pos;    

  if (time_string != NULL) {
    pos = 0;
    pos = sprintf(time_string, "%02d:", time->hours);
    pos += sprintf(time_string+pos, "%02d:", time->min);
    sprintf(time_string+pos, "%02d", time->sec);
  }
}

/****************************************************************************************
 * Converts Date data to string
 
 output: date string "YYYY-MM-DD" (= year-month-day)
 ****************************************************************************************/
void GPS_GE863::ConvertDate2String (
Date *date,               // time data structure
char *date_string         // output string
)
{
  unsigned char pos;    

  if (date_string != NULL) {
    pos = 0;
    pos = sprintf(date_string, "%d-", date->year);
    pos += sprintf(date_string+pos, "%02d-", date->month);
    sprintf(date_string+pos, "%02d", date->day);
  }
}

