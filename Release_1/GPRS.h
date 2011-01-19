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

#define GPRS_LIB_VERSION 101 // library version X.YY (e.g. 1.00)
/*
    Version
    --------------------------------------------------------------------------
    100       Initial version
    --------------------------------------------------------------------------
    101       OpenSocket() modified: remote and local port are now 2 bytes wide
              RcvData() modified: if "NO CARRIER" string is detected between 
              received data(= socket has been closed), status of communication 
              line is automatically changed from DATA to FREE state
    --------------------------------------------------------------------------
*/

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
    char OpenSocket(byte socket_type, uint16_t remote_port, char* remote_addr,
                    byte closure_type, uint16_t local_port);
    char CloseSocket(void);
    void SendData(char* str_data);
    void SendData(byte* data_buffer, unsigned short size);
    uint16_t RcvData(uint16_t start_comm_tmout, uint16_t max_interchar_tmout, byte** ptr_to_rcv_data);
    signed short StrInBin(byte* p_bin_data, char* p_string_to_search, unsigned short size);

  private:

};

#endif
