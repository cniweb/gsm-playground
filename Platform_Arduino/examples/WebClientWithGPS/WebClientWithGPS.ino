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
//byte buffer[COMM_BUF_LEN+30];
//byte buffer[30];
byte user_button_last_state = 0;
unsigned short last_temperature;
byte user_LED_last_request = 0;
byte GPIO10_last_state = 0;
byte GPIO11_last_state = 0;
byte GPIO12_last_request = 0;
byte GPIO13_last_request = 0;
char* ptr_char;
PROGMEM prog_uchar str_0[]   = {"GET http://www.hwkitchen.4fan.cz/example1/Client2WebData.php?"};
PROGMEM prog_uchar str_1[]   = {" HTTP/1.1\r\n"}; 
PROGMEM prog_uchar str_2[]   = {"Host:hwkitchen.cz\r\n"}; 
PROGMEM prog_uchar str_3[]   = {"Connection: close\r\n\r\n"};
char buffer[70];

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
     
      //gsm.SendData("GET http://www.hwkitchen.4fan.cz/example1/Client2WebData.php?"); 
      strcpy_P(buffer, (char*)pgm_read_word(&(str_0)));
      gsm.SendData(buffer);

      gsm.SendData("id=ID_123");  // change this ID for identification of your module

      gsm.SendData("&temp=");
      gsm.Print(last_temperature/10);

      gsm.SendData("&user_button=");
      if (user_button_last_state == 1) gsm.Print("ACTIVATED");
      else gsm.Print("NOT_ACTIVATED");

      gsm.SendData("&GPIO10=");
      if (GPIO10_last_state == 1) gsm.Print("HIGH");
      else gsm.Print("LOW");

      gsm.SendData("&GPIO11=");
      if (GPIO11_last_state == 1) gsm.Print("HIGH");
      else gsm.Print("LOW");

      gsm.SendData("&GPS_valid=");
      gsm.Print(gps_data_valid);
      if (gps_data_valid) {
        gsm.SendData("&GPS_latitude=");
        gps.ConvertPosition2String(&position, PART_LATITUDE, GPS_POS_FORMAT_3, string);
        gsm.Print(string);

        gsm.SendData("&GPS_longitude=");
        gps.ConvertPosition2String(&position, PART_LONGITUDE, GPS_POS_FORMAT_3, string);
        gsm.Print(string);
      }
      else {
        gsm.SendData("&GPS_latitude=");
        gsm.Print("0.000000");
        gsm.SendData("&GPS_longitude=");
        gsm.Print("0.000000");
      }
      
      // termination of HTML request
      //gsm.SendData(" HTTP/1.1\r\n"); 
      strcpy_P(buffer, (char*)pgm_read_word(&(str_1)));
      gsm.SendData(buffer);

      //gsm.SendData("Host:hwkitchen.cz\r\n");
      strcpy_P(buffer, (char*)pgm_read_word(&(str_2)));
      gsm.SendData(buffer);

      //gsm.SendData("Connection: close\r\n\r\n");
      strcpy_P(buffer, (char*)pgm_read_word(&(str_3)));
      gsm.SendData(buffer);

      // and wait for first incomming data max. 20sec.
      // receiving will be finished either buffer is full 
      // or there is no other incomming byte 1000msec. from last receiving byte
      // !!! Please note that in case buffer is full we have to read again and again 
      // not to loose any other incoming data !!!
/*
      do {
        // 20000 means: we will wait max. 20 sec. for first incomming character
        // 2000 means: receiving is finished if there is no other incomming character longer then 2sec.
        num_of_rx_bytes = gsm.RcvData(20000, 2000, &ptr_to_data);
        if (num_of_rx_bytes) {
          // we have received some data
          // we can analyze the data here etc.
          // we can e.g. find out if socket was not already closed from the host side
          // it means find the text <CR><LF>NO CARRIER<CR><LF> etc....


          // just now we only copy incomming data to the temporary buffer
          memcpy(buffer, ptr_to_data, num_of_rx_bytes);
          buffer[num_of_rx_bytes] = 0x00;
          ptr_char = strstr((const char *)buffer, "RET");
          if (ptr_char[7] == '1') user_LED_last_request = 1;
          else user_LED_last_request = 0;
          if (ptr_char[9] == '1') GPIO12_last_request = 1;
          else GPIO12_last_request = 0;
          if (ptr_char[11] == '1') GPIO13_last_request = 1;
          else GPIO13_last_request = 0;
        }
        else {
          // no data were received
        }
      } while (num_of_rx_bytes == COMM_BUF_LEN);
      */    
      Serial.setTimeout(20000);
      if (Serial.findUntil("RET_S;OK;", "NO CARRIER")) {
        // RET;OK; has been found
        Serial.readBytes((char *)buffer, 5);
        if (buffer[0] == '1') user_LED_last_request = 1;
        else user_LED_last_request = 0;
        if (buffer[2] == '1') GPIO12_last_request = 1;
        else GPIO12_last_request = 0;
        if (buffer[4] == '1') GPIO13_last_request = 1;
        else GPIO13_last_request = 0;
      }
      else {
        // timeout occured
      }

/*
      char nalez;
      Serial.flush();
      while (gsm.Available()) {
        //...do promìné c zapiš bajt odeslaný ze serveru 
        char c = gsm.Read(); 
        // když objevíš náš poèáteèný kontrolní znak 
        // tak nález bude true - pravda 
        // když koneèný tak false 
        if (c == 'N') nalez = true; 
        if (c == 'R') nalez = false; 

        // když je nález tak ukládej znaky do promìné data 
        // protože by se nám ukládal i první øídící znak tak, 
        // jen ukládej když je znak rozdílný od '<' 
        if (nalez && (c =='N')) buffer[0] = c;
      }
*/

 
      
      //Serial.setTimeout(2000);
      //if (!Serial.find("NO CARRIER")) {
        // now close the socket for a moment 
        gsm.CloseSocket();
      //}

      // activate or deactivate LED
      // --------------------------
      if (user_LED_last_request == 1) gsm.TurnOnLED();
      else gsm.TurnOffLED();

      // activate or deactivate GPIO12 and GPIO13
      // ----------------------------------------
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