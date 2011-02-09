/*
  GSM_GPRS.c - GPRS library for the GSM Playground - GSM Shield for Arduino
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

#include "GSM_GPRS.h"
#include "GSM.h"


extern "C" {
  #include <string.h>
}


/**********************************************************
Method returns GPRS library version

return val: 010 means library version 0.10
            101 means library version 1.01
**********************************************************/
int GSM::GPRSLibVer(void)
{
  return (GPRS_LIB_VERSION);
}


/**********************************************************
Method initializes GPRS

apn:      APN string
login:    user id string
password: password string

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free


        OK ret val:
        -----------
        0 - GPRS was not initialized
        1 - GPRS was initialized


an example of usage:
        APN si called internet
        user id and password are not used

        GSM gsm;
        gsm.InitGPRS("internet", "", ""); 
**********************************************************/
char GSM::InitGPRS(char* apn, char* login, char* password)
{
  char ret_val = -1;
  char cmd[100];

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // prepare command:  AT+CGDCONT=1,"IP","apn"
  strcpy(cmd, "AT+CGDCONT=1,\"IP\",\"");
  strcat(cmd, apn);
  strcat(cmd, "\""); // add character "
  ret_val = SendATCmdWaitResp(cmd, 1000, 100, "OK", 2);
  if (ret_val == AT_RESP_OK) {
    // prepare command:  AT#USERID="login"
    strcpy(cmd, "AT#USERID=\"");
    strcat(cmd, login);
    strcat(cmd, "\""); // add character "
    ret_val = SendATCmdWaitResp(cmd, 1000, 100, "OK", 2);
    if (ret_val == AT_RESP_OK) {
      // prepare command:  AT#PASSW="password"
      strcpy(cmd, "AT#PASSW=\"");
      strcat(cmd, password);
      strcat(cmd, "\""); // add character "
      ret_val = SendATCmdWaitResp(cmd, 1000, 100, "OK", 2);
      if (ret_val == AT_RESP_OK) ret_val = 1;
      else ret_val = 0;
    }
    else ret_val = 0;
  }
  else ret_val = 0;

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method enables GPRS context

open_mode: 
        0 (= CHECK_AND_OPEN) - checks the current state of context
                               and in case context has been already activated
                               nothing else in made 

        1 (= CLOSE_AND_REOPEN) - context is deactivated anyway and then activated again
                               it was found during testing, that you may need to reset the module etc., 
                               and in these cases, you may not be able to activate the GPRS context 
                               unless you deactivate it first

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free

        OK ret val:
        -----------
        0 - GPRS context was disabled
        1 - GPRS context was enabled


an example of usage:

        GSM gsm;
        if (gsm.EnableGPRS(CHECK_AND_OPEN) == 1) {
          // GPRS context was enabled, so we have IP address
          // and we can communicate if necessary
        }
**********************************************************/
char GSM::EnableGPRS(byte open_mode)
{
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);

  if (open_mode == CHECK_AND_OPEN) {
    // first try if the GPRS context has not been already initialized
    ret_val = SendATCmdWaitResp("AT#GPRS?", 1000, 100, "#GPRS: 0", 2);
    if (ret_val == AT_RESP_OK) {
      // context is not initialized => init the context
      //Enable GPRS
      ret_val = SendATCmdWaitResp("AT#GPRS=1", 10000, 1000, "OK", 1);
      if (ret_val == AT_RESP_OK) {
        // context was activated
        ret_val = 1;
      }
      else ret_val = 0; // not activated
    }
    else ret_val = 1; // context has been already activated
  }
  else {
    // CLOSE_AND_REOPEN mode
    //disable GPRS context
    ret_val = SendATCmdWaitResp("AT#GPRS=0", 10000, 1000, "OK", 3);
    if (ret_val == AT_RESP_OK) {
      // context is dactivated
      // => activate GPRS context again
      ret_val = SendATCmdWaitResp("AT#GPRS=1", 10000, 1000, "OK", 1);
      if (ret_val == AT_RESP_OK) {
        // context was activated
        ret_val = 1;
      }
      else ret_val = 0; // not activated
    }
    else ret_val = 0; // not activated
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method disables GPRS context

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free

        OK ret val:
        -----------
        0 - GPRS context was not disabled
        1 - GPRS context was disabled


an example of usage:
        ANP si called internet
        user id and password are not used

        GSM gsm;
        gsm.DisableGPRS(); 
**********************************************************/
char GSM::DisableGPRS(void)
{
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = SendATCmdWaitResp("AT#GPRS=0", 1000, 100, "OK", 2);
  if (ret_val == AT_RESP_OK) {
    // context was disabled
    ret_val = 1;
  }
  else ret_val = 0; // context was not disabled

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method opens the socket

<socket type> - socket protocol type
                0 - TCP
                1 - UDP
<remote port> - remote host port to be opened
                0..65535 - port number
<remote addr> - address of the remote host, string type. 
              This parameter can be either:
              - any valid IP address in the format: xxx.xxx.xxx.xxx
              - any host name to be solved with a DNS query in the format: <host
              name>
<closure type> - socket closure behaviour for TCP
              0 - local host closes immediately when remote host has closed (default)
              255 - local host closes after an escape sequence (+++) or after an abortive
                    disconnect from remote.
<local port> - local host port to be used on UDP socket

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free

        OK ret val:
        -----------
        0 - socket was not opened
        1 - socket was successfully opened


an example of usage:

        GSM gsm;
        gsm.OpenSocket(TCP, 80, "www.google.com", 0, 0); 
**********************************************************/
char GSM::OpenSocket(byte socket_type, uint16_t remote_port, char* remote_addr,
                     byte closure_type, uint16_t local_port)
{
  char ret_val = -1;
  char cmd[100];
  char tmp_str[10];

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // prepare command:  AT+CGDCONT=1,"IP","apn"
  strcpy(cmd, "AT#SKTD=");
  // add socket type
  strcat(cmd, itoa(socket_type, tmp_str, 10));
  strcat(cmd, ","); // add character ,
  // add remote_port
  strcat(cmd, itoa(remote_port, tmp_str, 10));
  strcat(cmd, ",\""); // add characters ,"
  // add remote addr
  strcat(cmd, remote_addr);
  strcat(cmd, "\","); // add characters ",
  // add closure type
  strcat(cmd, itoa(closure_type, tmp_str, 10));
  strcat(cmd, ","); // add character ,
  // add local port
  strcat(cmd, itoa(local_port, tmp_str, 10));

  // send AT command and waits for the response "CONNECT" - max. 3 times
  ret_val = SendATCmdWaitResp(cmd, 20000, 100, "CONNECT", 3);
  if (ret_val == AT_RESP_OK) {
    ret_val = 1;
    SetCommLineStatus(CLS_DATA);
  }
  else {
    ret_val = 0;
    SetCommLineStatus(CLS_FREE);
  }
  
  return (ret_val);
}


/**********************************************************
Methods send data to the serial port
There are 2 modification with possibility to send:
- string (finished by the standard end character 0x00)
- certain size of binary data buffer


return: 
        none


an example of usage:

        GSM gsm;
        byte buffer[20];

        gsm.SendData("Some text"); 
        or
        gsm.SendData(buffer, 20); 

**********************************************************/
void GSM::SendData(char* str_data)
{
  Serial.print(str_data);
}

void GSM::SendData(byte* data_buffer, unsigned short size)
{
  Serial.write(data_buffer, size);
}

/**********************************************************
Methods receives data from the serial port

return: 
        number of received bytes


an example of usage:

        GSM   gsm;
        byte  num_of_bytes;

        num_of_bytes = gsm.RcvData(5000, 100); 
        if (num_of_bytes) {
          // some data were received
        }

**********************************************************/
uint16_t  GSM::RcvData(uint16_t start_comm_tmout, uint16_t max_interchar_tmout, byte** ptr_to_rcv_data)
{
  byte status;

  RxInit(start_comm_tmout, max_interchar_tmout, 0, 0); 
  // wait until response is not finished

  do {
    status = IsRxFinished();
  } while (status == RX_NOT_FINISHED);

  if (comm_buf_len) *ptr_to_rcv_data = comm_buf;
  else *ptr_to_rcv_data = NULL;

  // check <CR><LF>NO CARRIER<CR><LF>
  // in case this string was received => socked is closed
  if (comm_buf_len) { 
    if (StrInBin(comm_buf, "\r\nNO CARRIER\r\n", comm_buf_len) != -1) {
      // NO CARRIER was received => socket was closed from the host side
      // we can set the communication line to the FREE state
      SetCommLineStatus(CLS_FREE);
    }
  }

  return (comm_buf_len);
}

/**********************************************************
Method closes previously opened socket

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not in the data(GPRS) state

        OK ret val:
        -----------
        0 - socket was not closed
        1 - socket was successfully closed


an example of usage:

        GSM gsm;
        gsm.CloseSocket(); 
**********************************************************/
char GSM::CloseSocket(void)
{
  char ret_val = -1;
  byte i;
  byte* rx_data;

  if (CLS_FREE == GetCommLineStatus()) {
    ret_val = 1; // socket was already closed
    return (ret_val);
  }

  // we are in the DATA state so try to close the socket
  // ---------------------------------------------------
  for (i = 0; i < 3; i++) {
    // make dalay 500msec. before escape seq. "+++"
    RcvData(1500, 100, &rx_data); // trick - function is used for generation a delay
    // send escape sequence +++ and wait for "NO CARRIER"
    SendData("+++");
    if (RX_FINISHED_STR_RECV == WaitResp(5000, 100, "NO CARRIER")) {
      // socket was successfully closed
      ret_val = 1;
      SetCommLineStatus(CLS_FREE);
      break;
    }
    else {
      // try common AT command just to be sure that the socket
      // has not been already closed
      ret_val = SendATCmdWaitResp("AT", 1000, 100, "OK", 1);
      if (ret_val == AT_RESP_OK) {
        // we are in the standard AT command mode so socket
        // has been already disabled
        ret_val = 1;
        SetCommLineStatus(CLS_FREE);
        break;
      }
      else {
        ret_val = 0;
      }
    }
  }

  return (ret_val);
}

/**********************************************************
Method used for finding string in the binary data buffer

p_bin_data: pointer to the binary "buffer" where a string should be find
p_string_to_search: pointer to the string which is supposed to be find
size: size of the binary "buffer"

return: 
        -1    - string was not found
        > -1  - first position in the buffer where string which was found started
**********************************************************/
signed short GSM::StrInBin(byte* p_bin_data, char* p_string_to_search, unsigned short size)
{
  uint16_t pos_1, pos_2, pos_before_match;

  pos_1 = 0;
  pos_2 = 0;
  pos_before_match = 0;
  if (size) {
    while (pos_1 < size) {
      if (p_string_to_search[pos_2] == p_bin_data[pos_1]) {
        pos_2++;
        pos_1++;
        if (p_string_to_search[pos_2] == 0) return (pos_before_match);
      }
      else {
        pos_2 = 0; // from the start of p_string_to_search
        pos_before_match++;
        pos_1 = pos_before_match;
      }
    }
    return (-1);
  }
  else return (-1);
}

