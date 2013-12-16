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


    //----------------------------------------
      Description of sketch WebClientWithGPS
    //----------------------------------------

     This sketch works as a web client. It means that GSM-GPS Playground Shield is connected via GPRS connection
     to the Internet through GSM provider and periodically tries to connect to the web server http://www.hwkitchen.4fan.cz.
     During this connection program sends following data to the server(by the GET method):
     - ID of the module: id=ID_123 (Please change it in the code to have your own)
     - GSM module temperature:&temp=xxxx
     - user button state: &user_button=ACTIVATED/DEACTIVATED
     - state of GPIO10: &GPIO10=HIGH/LOW
     - state of GPIO11: &GPIO10=HIGH/LOW
     - GPS data if data are valid: &GPS_valid=1/0
     - in case GPS data are valid there are 2 additional items:
       - latitude: &GPS_latitude=xx.yyyyyyN
       - longitude: &GPS_longitude=ZZ.zzzzzE

    The web server parses received information and saves them to the data file on the server. If client(internet browser)
    connects to this web page server reads data from the file and sends them as a web page to the client.
    This web page also includes some items for controlling of:
    - user LED
    - output GPIO12
    - output GPIO13

    These information are sent back from the server to the GSM-GPS Playground Shield as a response during "GET method".
    One transaction(data are sent to the server from GSM module by GET method and response is received back to the GSM module) takes 
    cca. 2-3sec. This delay is here because of some latency of GPRS connection. Also during this transaction the GSM module is in transparent 
    DATA state so it is not possible to use standard AT command during time. Because the AT commands are used for user button checking
    the user button must hold during whole "one transaction time - cca. 3sec." to detect button ACTIVATED state. To eliminate this 
    disadvantage it would be necessary to use so called "IP Easy" possibilities of the Telit GSM-GPS module(with firmware 7.02.03 and higher)
    where the TCP socket can be controlled by standard AT commands and no transparent DATA connection are necessary. However this sketch
    is general and can be used also together with older GSM Playground Shield.


    The advantage of this approach(unlike the Web Server) is the fact that Web Client can work with dynamic and also with not a public
    IP address(what is the standard setting of GSM provider). The other benefit is that the web pages can be quite complex 
    because they are placed and executed on the server.

    The web pages created for this example are placed here:
    http://www.hwkitchen.4fan.cz

    This is only example so incoming data are stored to the file and are read also from the file and web pages
    displays last received data of one module. So in case more modules are connected "in the same time" the last one is a "winner".

    For serious and more complex application would be necessary to use your own web server and probably some database to store 
    and restore data from corresponding module to maintain more than one GSM-GPS Playground Shield at a time.
    But this is out of scope of this example.


    We use www.endora.cz as a free webhosting server for this example but there are a lot of providers with free hosting services.

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
    gsm.DebugPrintF(PSTR("DEBUG AT library version: "), 0);
    gsm.DebugPrint(gsm.LibVer(), 0);
    gsm.DebugPrintF(PSTR("DEBUG GSM library version: "), 0);
    gsm.DebugPrint(gsm.GSMLibVer(), 0);
    gsm.DebugPrintF(PSTR("DEBUG GPRS library version: "), 0);
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

  // config socket so the data are send 100msec. after last byte is received from serial line
  // so immediatelly to have very fast response
  ret_val = gsm.IPEasyExt_ConfigSocket(1, 1, 300, 90, 600, 1/*100msec.*/);

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
      // In case there is no incomming data more than 20sec. reception is also finished

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

      // close the socket so we will go back from "DATA state" to
      // "AT command state"
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
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      // In case this timeout is increased the response to/from the web server is slower
      // but less data are transfered e.g. per day so the servis is cheaper.
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      delay(2000);
      
    }
    else {
      // socket was not opened successfully
      gsm.EnableGPRS(CLOSE_AND_REOPEN); // try to close and open socket again
    }
}