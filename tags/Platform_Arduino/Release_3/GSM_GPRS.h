/*
  GSM_GPRS.h - library for the GSM Playground - GSM Shield for Arduino
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
#ifndef __GSM_GPRS
#define __GSM_GPRS


#define GPRS_LIB_VERSION 102 // library version X.YY (e.g. 1.00)
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
    102       GPRS is a part of GSM class to avoid repeated inheritance in future
              (in case other module will be added, like GPS)
    --------------------------------------------------------------------------
*/

// type of the socket
#define  TCP_SOCKET 0
#define  UDP_SOCKET 1

// mode for the context activation
#define CHECK_AND_OPEN    0
#define CLOSE_AND_REOPEN  1



#endif
