/*
 GPRS.h - library for the GSM Playground - GSM Shield for Arduino
  www.hwkitchen.com

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
#ifndef __GPRS
#define __GPRS


#include "WProgram.h"
#include "AT.h"

#define GPRS_LIB_VERSION 100 // library version X.YY (e.g. 1.00)

// type of the socket
#define  TCP_SOCKET 0
#define  UDP_SOCKET 1

// mode for the context activation
#define CHECK_AND_OPEN    0
#define CLOSE_AND_REOPEN  1

class GPRS : public AT 
{
  public:

    // library version
    int LibVer(void);
    // constructor
    GPRS(void);

    char InitGPRS(char* apn, char* login, char* password);
    char EnableGPRS(byte open_mode);
    char DisableGPRS(void);
    char OpenSocket(byte socket_type, byte remote_port, char* remote_addr,
                    byte closure_type, byte local_port);
    char CloseSocket(void);
    void SendData(char* str_data);
    void SendData(byte* data_buffer, unsigned short size);
    uint16_t RcvData(uint16_t start_comm_tmout, uint16_t max_interchar_tmout, byte** ptr_to_rcv_data);

  private:

};

#endif
