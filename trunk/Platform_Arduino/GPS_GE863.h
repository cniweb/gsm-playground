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

typedef struct {
	int lat_deg;
	long lat_min;
	int lon_deg;
	long lon_min;
	long alt;
	byte fix;
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

  private:
    void ParseGPS(char *gpsMsg, Position *pos, Time *time, Date *date);
    void ParseDateTime(Time *time, Date *date, char *time_str, char *date_str);
    void ParsePosition(Position *pos, char *lat_str, char *lon_str, char *alt_str);
    void ParseDegrees(char *str, int *degree, long *minutes);
    Position actual_position;
    Time actual_time;
    Date actual_date;
};


#endif





