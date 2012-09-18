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
#ifndef __GPS_GE863
#define __GPS_GE863

#include "GSM.h"


#define GPS_LIB_VERSION 100 // library version X.YY (e.g. 1.00)
/*
    Version
    --------------------------------------------------------------------------
    100       Initial version
    --------------------------------------------------------------------------
*/

enum reset_type_enum
{
  GPS_RESET_HW = 0,
  GPS_RESET_COLDSTART,
  GPS_RESET_WARMSTART,
  GPS_RESET_HOTSTART,
    
  GPS_RESET_LAST_ITEM
};

#define PART_LATITUDE         0
#define PART_LONGITUDE        1
#define PART_ALTITUDE         2


#define FORMAT_DEG            0 // degree
#define FORMAT_MIN            1 // minute
#define FORMAT_SEC            2 // seconds
#define FORMAT_0POINT01_SEC   3 // hundredth of seconds
                                // it means result:  1 = 0.01 sec.
                                //                  10 = 0.10 sec.                                                               
// string format
#define GPS_POS_FORMAT_1      1 // format DD°MM´SS.SS½
#define GPS_POS_FORMAT_2      2 // format DD°MM.MMM´ - not supported yet
#define GPS_POS_FORMAT_3      3 // format DD.DDDDD° - not supported yet


typedef struct {
  unsigned long latitude_raw;
  char          latitude_dir;

  unsigned long longitude_raw;
  char          longitude_dir;
	
	long          altitude;

	byte          fix;
} Position;

typedef struct {
	unsigned char sec;
	unsigned char min;
	unsigned char hours;
} Time;

typedef struct {
	unsigned char day;
	unsigned char month;
	unsigned short year;
} Date;


class GPS_GE863 
{
  public:
    GPS_GE863(void);
    int  GPSLibVer(void);
    char GPSPowerUpOrDown(unsigned char command);
    char ResetGPSModul(byte reset_type);
    char GetGPSSwVers(char *sw_ver_string);
    char ControlGPSAntenna(unsigned char command); 
    char GetGPSAntennaSupplyVoltage(unsigned short *redout_voltage);
    char GetGPSAntennaCurrent(unsigned short *redout_current); 
    char GetGPSData(Position *position, Time *time, Date *date);
    long GetPositionPart(Position *position, char part, char format);
    void ConvertPosition2String(Position *position, char part, char format, char *out_pos_string);
    void ConvertTime2String(Time *time, char *time_string);
    void ConvertDate2String(Date *date, char *date_string);


  private:
    void ParseGPS(char *gpsMsg, Position *pos, Time *time, Date *date);
    void ParseDateTime(char *time_str, char *date_str, Time *time, Date *date);
    void ParsePosition(char *lat_str, char *lon_str, char *alt_str, Position *pos);
    void ParseRawPosition(char *pos_str,
                          unsigned long *raw_position,
                          char *direction_char); 
    Position actual_position;
    Time actual_time;
    Date actual_date;
};


#endif





