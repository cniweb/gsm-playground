/*
    This sketch demonstrates how to use GPRS connection

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

// return variable
signed char ret_val;
uint16_t num_of_rx_bytes;
byte* ptr_to_data;
byte buffer[COMM_BUF_LEN];


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
    gsm.DebugPrint(gsm.GSMLibVer(), 0);
    gsm.DebugPrint("DEBUG GPRS library version: ", 0);
    gsm.DebugPrint(gsm.GPRSLibVer(), 1);
  #endif

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
}


void loop()
{
  // try to read data with "GET request" - 3 times
  // between each attempt we will try to use standard AT command
  // to demonstrate a possibility to use AT command interface(call, SMS etc.) and GPRS connection "together"
  // -------------------------------------------------------------------------------------------------------
  for (byte i = 0; i < 3; i++) {
    // open the TCP socket
    ret_val = gsm.OpenSocket(TCP_SOCKET, 80, "www.google.com", 0, 0);
    if (ret_val == 1) {
      // socket was successfully opened
      // so we can exchange data
      // here we are trying GET request
      // GET request must be finished by sequence <CR><LF><CR><LF> == \r\n\r\n
      gsm.SendData("GET / HTTP/1.1\r\nHost: www.google.com\r\n\r\n");
      // and wait for first incomming data max. 20sec.
      // receiving will be finished either buffer is full 
      // or there is no other incomming byte 1000msec. from last receiving byte
      // !!! Please note that in case buffer is full we have to read again and again 
      // not to loose any other incoming data !!!
      do {
        // 20000 means: we will wait max. 20 sec. for first incomming character
        // 1000 means: receiving is finished if there is no other incomming character longer then 1sec.
        num_of_rx_bytes = gsm.RcvData(20000, 1000, &ptr_to_data);
        if (num_of_rx_bytes) {
          // we have received some data
          // we can analyze the data here etc.
          // we can e.g. find out if socket was not already closed from the host side
          // it means find the text <CR><LF>NO CARRIER<CR><LF> etc....


          // just now we only copy incomming data to the temporary buffer
          memcpy(buffer, ptr_to_data, num_of_rx_bytes);
        }
        else {
          // no data were received
        }
      } while (num_of_rx_bytes == COMM_BUF_LEN);
      

      // now close the socket for a moment 
      gsm.CloseSocket();

      // and now we can try standard AT command - e.g. try to check registration
      // or we can check new SMS etc.
      if (REG_REGISTERED == gsm.CheckRegistration()) {
        // we are registered => everything is OK
      }
      else {
        // we are not registered => make something
      }

    }
  }
  // and deactivate GPRS context
  gsm.DisableGPRS();


  // we are finished so stay here forever
  while (1);
}