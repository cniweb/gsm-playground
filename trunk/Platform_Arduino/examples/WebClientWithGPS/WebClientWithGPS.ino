/*
    This sketch demonstrates how to use HW kitchen GSM shield as Web Client connected through GPRS

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
#include <avr/pgmspace.h>
#include "GPS_GE863.h"


// definition of instance of GPS class
GPS_GE863 gps;


// ---------------------------------------------------------------------------
// Important:
// ========== 
// instance of GSM class("GSM gsm;") is already defined in the GSM.cpp module
// so we cannot define this instance here
// see explanation in the GSM.cpp module
// ---------------------------------------------------------------------------

// return variable
signed char ret_val;
uint16_t num_of_rx_bytes;
byte* ptr_to_data;
byte buffer[30];
byte user_button_last_state = 0;
unsigned short last_temperature;
byte user_LED_last_request = 0;
byte GPIO10_last_state = 0;
byte GPIO11_last_state = 0;
byte GPIO12_last_request = 0;
byte GPIO13_last_request = 0;
char* ptr_char;


// GPS related data
Position position;
Time time;
Date date;
char  string[15];

signed char gps_data_valid;



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
    gsm.DebugPrint("DEBUG GPRS library version: ", 0);
    gsm.DebugPrint(gsm.GPRSLibVer(), 1);
  #endif

  // set direction for GPIO pins
  // GPIO10 and GPIO11 as inputs
  gsm.SetGPIODir(GPIO10, GPIO_DIR_IN);
  gsm.SetGPIODir(GPIO11, GPIO_DIR_IN);
  // GPIO12 and GPIO13 as outputs
  gsm.SetGPIODir(GPIO12, GPIO_DIR_OUT);
  gsm.SetGPIODir(GPIO13, GPIO_DIR_OUT);

  // wait until a GSM module is registered in the GSM network
  while (!gsm.IsRegistered()) {
    gsm.CheckRegistration();
    delay(1000);
  }
  
  // use GPRS APN "internet" - it is necessary to find out right one for
  // your GSM provider
  ret_val = gsm.InitGPRS("internet", "", "");

  // activate GPRS context - IP address will be assigned
  ret_val = gsm.EnableGPRS(CLOSE_AND_REOPEN);

  // enable user button
  gsm.EnableUserButton();

  // reset GPS modul
  // hot start is used for faster connection to GPS
  // -----------------------------------------------
  ret_val = gps.ResetGPSModul(GPS_RESET_HOTSTART);

  // delay after initialization
  delay(5000);
  // turn on GPS antenna
  gps.ControlGPSAntenna(1); 
}


void loop()
{
    // Here TCP socket is not activated yet so we can use line for AT commands
    // ---------------------------------------------------------------------

    // Now we can try standard AT command - e.g. try to read User button state
    // or we can check new SMS etc.
    // ----------------------------------------------------------------------------
    if (gsm.IsUserButtonPushed()) user_button_last_state = 1;
    else user_button_last_state = 0;

    if (gsm.GetGPIOVal(GPIO10) == 1) GPIO10_last_state = 1;
    else GPIO10_last_state = 0;

    if (gsm.GetGPIOVal(GPIO11) == 1) GPIO11_last_state = 1;
    else GPIO11_last_state = 0;


    // read GSM module temperature
    // ----------------------------
    last_temperature = gsm.GetTemp();

    // read GPS data
    // -------------
    gps_data_valid = gps.GetGPSData(&position, &time, &date);


    // open the TCP socket - in case TCP socket is activated the communication line is 
    // in transparent data state so we can not use standard AT commands until TCP socked is not closed
    // -----------------------------------------------------------------------------------------------
    ret_val = gsm.OpenSocket(TCP_SOCKET, 80, "www.hwkitchen.4fan.cz", 0, 0);
    if (ret_val == 1) {
      // socket was successfully opened
      // so we can exchange data
      // here we are trying GET request
      // GET request must be finished by sequence <CR><LF><CR><LF> == \r\n\r\n

           
      // generate GET reguest with data:
      // http://www.hwkitchen.4fan.cz/example1/Client2WebData.php?id=ID_1&temp=41&user_button=NOT_ACTIVATED&GPIO10=LOW&GPIO11=HIGH&GPS_valid=1&GPS_latitude=DD.DDDDDN&GPS_longitude=DD.DDDDDE
      // PSTR means that constant data string is placed in Flash program memory to save RAM memory
      gsm.SendDataF(PSTR("GET http://www.hwkitchen.4fan.cz/example1/Client2WebData.php?"));
      

      // -------------------------------------------------------------------------------
      // -------------------------------------------------------------------------------
      // !!!!! change this ID for identification of your module on the Web server !!!!!
      // -------------------------------------------------------------------------------
      // -------------------------------------------------------------------------------
      gsm.SendDataF(PSTR("id=ID_123"));

      // send actual temperature of the GSM module
      gsm.SendDataF(PSTR("&temp="));
      gsm.Print(last_temperature/10);

      // send state of the user button
      gsm.SendDataF(PSTR("&user_button="));
      if (user_button_last_state == 1) gsm.PrintF(PSTR("ACTIVATED"));
      else gsm.PrintF(PSTR("NOT_ACTIVATED"));

      // send state of the GPIO10
      gsm.SendDataF(PSTR("&GPIO10="));
      if (GPIO10_last_state == 1) gsm.PrintF(PSTR("HIGH"));
      else gsm.PrintF(PSTR("LOW"));

      // send state of the GPIO11
      gsm.SendDataF(PSTR("&GPIO11="));
      if (GPIO11_last_state == 1) gsm.PrintF(PSTR("HIGH"));
      else gsm.PrintF(PSTR("LOW"));

      // send state of the GPS module
      gsm.SendDataF(PSTR("&GPS_valid="));
      gsm.Print(gps_data_valid);
      if (gps_data_valid) {
        gsm.SendDataF(PSTR("&GPS_latitude="));
        gps.ConvertPosition2String(&position, PART_LATITUDE, GPS_POS_FORMAT_3, string);
        gsm.Print(string);

        gsm.SendDataF(PSTR("&GPS_longitude="));
        gps.ConvertPosition2String(&position, PART_LONGITUDE, GPS_POS_FORMAT_3, string);
        gsm.Print(string);
      }
      else {
        gsm.SendDataF(PSTR("&GPS_latitude="));
        gsm.PrintF(PSTR("0.000000"));
        gsm.SendDataF(PSTR("&GPS_longitude="));
        gsm.PrintF(PSTR("0.000000"));
      }
      
      // termination of HTML request
      gsm.SendDataF(PSTR(" HTTP/1.1\r\n")); 

      gsm.SendDataF(PSTR("Host:hwkitchen.cz\r\n"));

      gsm.SendDataF(PSTR("Connection: close\r\n\r\n"));

      // and wait for first incomming data max. 20sec.
      // receiving will be finished either in case NO_CARRIER is received
      // or RET_S;OK string is finished
      // In case there is no incomming data more than 20sec. reception is alo finished

      // Data which are sent beck has a following structure:
      // "RET_S;OK;X;Y;Z;RET_E"
      // where X is required state for user LED: '0'=deactivate, '1'=activate
      //       Y is required state for GPIO12: '0'=deactivate, '1'=activate
      //       Z is required state for GPIO13: '0'=deactivate, '1'=activate
      // 
      // or 
      // "RET_S;ERROR;Reason of error;RET_E"
      // --------------------------------------------------------------------
      if (gsm.FindUntil("RET_S;OK;", "NO CARRIER", 20000)) {
        // RET_S;OK; has been found
        gsm.ReadBytes((char *)buffer, 5);
        if (buffer[0] == '1') user_LED_last_request = 1;
        else user_LED_last_request = 0;
        if (buffer[2] == '1') GPIO12_last_request = 1;
        else GPIO12_last_request = 0;
        if (buffer[4] == '1') GPIO13_last_request = 1;
        else GPIO13_last_request = 0;
      }
      else {
        // timeout occured or "NO CARRIER" was found
        // so RET_S;OK; string has not been find
      }

      // close the socket      
      gsm.CloseSocket();


      // activate or deactivate LED according received command
      // ------------------------------------------------------
      if (user_LED_last_request == 1) gsm.TurnOnLED();
      else gsm.TurnOffLED();

      // activate or deactivate GPIO12 and GPIO13 according received command
      // -------------------------------------------------------------------
      if (GPIO12_last_request == 1) gsm.SetGPIOVal(GPIO12, 1);
      else gsm.SetGPIOVal(GPIO12, 0);

      if (GPIO13_last_request == 1) gsm.SetGPIOVal(GPIO13, 1);
      else gsm.SetGPIOVal(GPIO13, 0);


      // wait some time
      delay(2000);
      
    }
    else {
      // socket was not opened successfully
      gsm.EnableGPRS(CLOSE_AND_REOPEN); // try to close and open socket again
    }
  
}