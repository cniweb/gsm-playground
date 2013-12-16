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
#include <avr/pgmspace.h>
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
  strcpy_P(cmd, PSTR("AT+CGDCONT=1,\"IP\",\""));
  strcat(cmd, apn);
  strcat_P(cmd, PSTR("\"")); // add character "
  ret_val = SendATCmdWaitResp(cmd, 1000, 100, "OK", 2);
  if (ret_val == AT_RESP_OK) {
    // prepare command:  AT#USERID="login"
    strcpy_P(cmd, PSTR("AT#USERID=\""));
    strcat(cmd, login);
    strcat_P(cmd, PSTR("\"")); // add character "
    ret_val = SendATCmdWaitResp(cmd, 1000, 100, "OK", 2);
    if (ret_val == AT_RESP_OK) {
      // prepare command:  AT#PASSW="password"
      strcpy_P(cmd, PSTR("AT#PASSW=\""));
      strcat(cmd, password);
      strcat_P(cmd, PSTR("\"")); // add character "
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
    ret_val = SendATCmdWaitRespF(PSTR("AT#GPRS?"), 1000, 100, "#GPRS: 0", 2);
    if (ret_val == AT_RESP_OK) {
      // context is not initialized => init the context
      //Enable GPRS
      ret_val = SendATCmdWaitRespF(PSTR("AT#GPRS=1"), 10000, 1000, "OK", 1);
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
    ret_val = SendATCmdWaitRespF(PSTR("AT#GPRS=0"), 10000, 1000, "OK", 3);
    if (ret_val == AT_RESP_OK) {
      // context is dactivated
      // => activate GPRS context again
      ret_val = SendATCmdWaitRespF(PSTR("AT#GPRS=1"), 10000, 1000, "OK", 1);
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
  ret_val = SendATCmdWaitRespF(PSTR("AT#GPRS=0"), 1000, 100, "OK", 2);
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
  // AT#SKTD=0,80,"www.telit.net", 0, 0
  strcpy_P(cmd, PSTR("AT#SKTD="));
  // add socket type
  strcat(cmd, itoa(socket_type, tmp_str, 10));
  strcat_P(cmd, PSTR(",")); // add character ,
  // add remote_port
  strcat(cmd, itoa(remote_port, tmp_str, 10));
  strcat_P(cmd, PSTR(",\"")); // add characters ,"
  // add remote addr
  strcat(cmd, remote_addr);
  strcat_P(cmd, PSTR("\",")); // add characters ",
  // add closure type
  strcat(cmd, itoa(closure_type, tmp_str, 10));
  strcat_P(cmd, PSTR(",")); // add character ,
  // add local port
  strcat(cmd, itoa(local_port, tmp_str, 10));

  // send AT command and waits for the response "CONNECT" - max. 3 times
  ret_val = SendATCmdWaitResp(cmd, 20000, 200, "CONNECT", 3);
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
Method initializes GPRS in IP Easy Extended mode

PDP_contect_identifier: (PDP Context Identifier) numeric parameter which specifies a particular PDP context definition.
                        Values: 1..5
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
        gsm.IPEasyExt_InitGPRS(1, "internet", "", ""); 
**********************************************************/
char GSM::IPEasyExt_InitGPRS(byte PDP_contect_identifier, char* apn, char* login, char* password)
{
  char ret_val = -1;
  char tmp_str[10];
  char cmd[100];

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // prepare command:  AT+CGDCONT=1,"IP","apn"
  strcpy_P(cmd, PSTR("AT+CGDCONT="));
  // connection ID
  strcat(cmd, itoa(PDP_contect_identifier, tmp_str, 10));
  strcat_P(cmd, PSTR(",")); // add character ,
  strcat_P(cmd, PSTR("\"IP\""));
  strcat_P(cmd, PSTR(",\"")); // add character ,
  strcat(cmd, apn);
  strcat_P(cmd, PSTR("\"")); // add character "
  ret_val = SendATCmdWaitResp(cmd, 1000, 100, "OK", 2);
  if (ret_val == AT_RESP_OK) {
    // prepare command:  AT#USERID="login"
    strcpy_P(cmd, PSTR("AT#USERID=\""));
    strcat(cmd, login);
    strcat_P(cmd, PSTR("\"")); // add character "
    ret_val = SendATCmdWaitResp(cmd, 1000, 100, "OK", 2);
    if (ret_val == AT_RESP_OK) {
      // prepare command:  AT#PASSW="password"
      strcpy_P(cmd, PSTR("AT#PASSW=\""));
      strcat(cmd, password);
      strcat_P(cmd, PSTR("\"")); // add character "
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
Method enables/disables (= activates/deactivates) GPRS context in IP Easy Extended mode

PDP_contect_identifier: is the context that we want to activate/deactivate 1..5(previously opened by IPEasyExt_InitGPRS)
enable_disable: is the context status (0 means deactivation, 1 activation).

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free

        OK ret val:
        -----------
        0 - GPRS context was deactivated
        1 - GPRS context was activated


an example of usage:

        GSM gsm;
        if (gsm.IPEasyExt_EnableOrDisableGPRS(1, 1) == 1) {
          // GPRS context was enabled, so we have IP address
          // and we can communicate if necessary
        }
**********************************************************/
char GSM::IPEasyExt_EnableOrDisableGPRS(byte PDP_contect_identifier, byte enable_disable)
{
  char ret_val = -1;
  char cmd[100];
  char tmp_str[10];
  char num_of_bytes;
  byte* rx_data;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);

  // AT#SD=0,80,"remote_addr(e.g. www.telit.net)", 0, 0
  strcpy_P(cmd, PSTR("AT#SGACT="));
  // context ID
  strcat(cmd, itoa(PDP_contect_identifier, tmp_str, 10));
  strcat_P(cmd, PSTR(",")); // add character ,
  // anable/disable
  strcat(cmd, itoa(enable_disable, tmp_str, 10));
  Println(cmd);
  
  //wait for response
  /*
  Answer:
  #SGACT: 212.195.45.65
  OK if activation success.
  ERROR if activation fails.
  */
  if (FindUntil("#SGACT: ", "ERROR", 20000)) {
    // #SGACT: "    string was found
    num_of_bytes = ReadBytesUntil('\r', IP_address, 17/*max XXX.XXX.XXX.XXX + CR LF */);
    if (num_of_bytes) {
      // finish IP string
      IP_address[num_of_bytes] = 0x00; // finish string
      // make delay to receive last byte '\n' from serial line
      RcvData(200, 100, &rx_data); // trick - function is used for generation a delay
      ret_val = 1;
    }
    else {
      // ERROR has been received
      ret_val = 0;        
    }
  }
  else {
    // ERROR has been received
    ret_val = 0;
    RcvData(200, 100, &rx_data); // trick - function is used for generation a delay
  }

  
  if (ret_val == 0) {
    // ERROR response => try to reopen connection => close and open again
    strcpy_P(cmd, PSTR("AT#SGACT="));
    // context ID
    strcat(cmd, itoa(PDP_contect_identifier, tmp_str, 10));
    strcat_P(cmd, PSTR(",0")); // add character , and 0 = DISABLE
    ret_val = SendATCmdWaitResp(cmd, 20000, 200, "OK", 3);

    // and now try anyway 
    strcpy_P(cmd, PSTR("AT#SGACT="));
    // context ID
    strcat(cmd, itoa(PDP_contect_identifier, tmp_str, 10));
    strcat_P(cmd, PSTR(",")); // add character ,
    // anable/disable
    strcat(cmd, itoa(enable_disable, tmp_str, 10));
    Println(cmd);
    if (FindUntil("#SGACT: ", "ERROR", 20000)) {
      // #SGACT: "    string was found
      num_of_bytes = ReadBytesUntil('\r', IP_address, 17/*max XXX.XXX.XXX.XXX + CR LF */);
      if (num_of_bytes) {
        // finish IP string
        IP_address[num_of_bytes] = 0x00; // finish string
        // make delay to receive last byte '\n' from serial line
        RcvData(200, 100, &rx_data); // trick - function is used for generation a delay
        ret_val = 1;
      }
      else {
        // ERROR has been received
        ret_val = 0;        
      }
    }
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}


/**********************************************************
Method returns actuall IP address in string

!!!! IP addresses is parsed in the method IPEasyExt_EnableOrDisableGPRS() !!!!
so before using this function the function IPEasyExt_EnableOrDisableGPRS() must be called

return: pointer to string in the format "XXX.XXX.XXX.XXX"
        

an example of usage:

        GSM gsm;
        p_IP_addr *char; 
        gsm.IPEasyExt_EnableOrDisableGPRS(1, 1); // try to open socket
        p_IP_addr = gsm.IPEasyExt_GetLocalIPAddress();
**********************************************************/
char* GSM::IPEasyExt_GetLocalIPAddress(void)
{
  return (IP_address);
}

/**********************************************************
Method open the socket in IP Easy Extended mode

<connection_id> - socket id: 1..6

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
        gsm.IPEasyExt_OpenSocket(TCP, 80, "www.google.com", 0, 0); 
**********************************************************/
char GSM::IPEasyExt_OpenSocket(byte connection_id, byte socket_type, uint16_t remote_port, char* remote_addr,
                     byte closure_type, uint16_t local_port)
{
  char ret_val = -1;
  char cmd[100];
  char tmp_str[10];

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // set Escape Prompt Delay = minimum time before "+++" to 20*1/50sec. = 20*20msec. = 400msec.
  SendATCmdWaitRespF(PSTR("ATS12=20"), 500, 20, "OK", 3);

  // AT#SD=0,80,"remote_addr(e.g. www.telit.net)", 0, 0
  strcpy_P(cmd, PSTR("AT#SD="));
  // connection ID
  strcat(cmd, itoa(connection_id, tmp_str, 10));
  strcat_P(cmd, PSTR(",")); // add character ,
  // add socket type
  strcat(cmd, itoa(socket_type, tmp_str, 10));
  strcat_P(cmd, PSTR(",")); // add character ,
  // add remote_port
  strcat(cmd, itoa(remote_port, tmp_str, 10));
  strcat_P(cmd, PSTR(",\"")); // add characters ,"
  // add remote addr
  strcat(cmd, remote_addr);
  strcat_P(cmd, PSTR("\",")); // add characters ",
  // add closure type
  strcat(cmd, itoa(closure_type, tmp_str, 10));
  strcat_P(cmd, PSTR(",")); // add character ,
  // add local port
  strcat(cmd, itoa(local_port, tmp_str, 10));

  // send AT command and waits for the response "CONNECT" - max. 3 times
  ret_val = SendATCmdWaitResp(cmd, 20000, 200, "CONNECT", 3);
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
Method open the TCP socket in listening mode (in IP Easy Extended mode)

<connection_id> - socket id: 1..6

<listen_state> -  0 - closes socket listening
                  1 - starts socket listening
                
<listen_port> - local host port to be used on UDP socket

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free

        OK ret val:
        -----------
        0 - socket was not opened
        1 - socket was opened in listening mode

        Now the module is listening for incoming connection on port 6543 with Connection Id number 2, 
        if a remote host is trying to connect we’ll receive a SRING unsolicited indication with the listening Connection Id:
        SRING: 1


an example of usage:

        GSM gsm;
        gsm.IPEasyExt_OpenSocketInListenMode(1, 1, 6543);   // start listening


        gsm.IPEasyExt_OpenSocketInListenMode(1, 0, 6543);   // stop listening
**********************************************************/
char GSM::IPEasyExt_OpenSocketInListenMode(byte connection_id, byte listen_state, uint16_t listen_port)
{
  char ret_val = -1;
  char cmd[50];
  char tmp_str[10];

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // AT#SL = 1, 1, 6543
  strcpy_P(cmd, PSTR("AT#SL="));
  // connection ID
  strcat(cmd, itoa(connection_id, tmp_str, 10));
  strcat_P(cmd, PSTR(",")); // add character ,
  // add listen state
  strcat(cmd, itoa(listen_state, tmp_str, 10));
  strcat_P(cmd, PSTR(",")); // add character ,
  // add listen port
  strcat(cmd, itoa(listen_port, tmp_str, 10));

  // send AT command and waits for the response "CONNECT" - max. 3 times
  ret_val = SendATCmdWaitResp(cmd, 20000, 200, "OK", 3);
  if (ret_val == AT_RESP_OK) {
    ret_val = 1;
  }
  else {
    ret_val = 0;
  }
  
  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method accept incomming connection on specified socket (in IP Easy Extended mode)

<connection_id> - socket id: 1..6


        ERROR ret. val:
        ---------------
        -1 - comm. line is not free

        OK ret val:
        -----------
        0 - socket was accepted and now we are in DATA online state
            We pass in online mode and the connection is established. 
            With the escape sequence we suspend the socket and the module is back to command mode. 
            To resume the suspended connection we can use the #SO

        1 - CONNECT status was not received



an example of usage:

        GSM gsm;
        gsm.IPEasyExt_AcceptSocket(1);   // accept incoming connection in socket 1
**********************************************************/
char GSM::IPEasyExt_AcceptSocket(byte connection_id)
{
  char ret_val = -1;
  char cmd[50];
  char tmp_str[10];

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // AT#SL = 1, 1, 6543
  strcpy_P(cmd, PSTR("AT#SA="));
  // connection ID
  strcat(cmd, itoa(connection_id, tmp_str, 10));
  
  // send AT command and waits for the response "CONNECT" - max. 3 times
  ret_val = SendATCmdWaitResp(cmd, 20000, 200, "CONNECT", 3);
  if (ret_val == AT_RESP_OK) {
    SetCommLineStatus(CLS_DATA);
    ret_val = 1;
  }
  else {
    SetCommLineStatus(CLS_FREE);
    ret_val = 0;
  }
  
  return (ret_val);
}

/**********************************************************
Method config socket in IP Easy Extended mode

connection_id - socket id: 1..6

context id    - -the context identifier

min_pkt_size        -   the minimum data packet sent to the net
                        (default 300 bytes)    // in bytes  

inactivity_tmout    - inactivity timeout (default 90 sec.)

connection_tmout    - connection timeout (default 60 sec, expressed in
                      tenths of second)

data_sending_tmout  -  data sending timeout (default 5 sec, expressed in
                        tenths of second)

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free

        OK ret val:
        -----------
        0 - socket was configured OK
        1 - error


an example of usage:

        GSM gsm;

        We want to associate the Connection Id number 1 to the context number 1 
        with a minimum packet size of 300 bytes, 
        global timeout of 90 sec, 
        connection timeout of 60 sec 
        and transmission timeout of 5 sec.

        gsm.IPEasyExt_ConfigSocket(1, 1, 300, 90, 600, 50); 
**********************************************************/
char GSM::IPEasyExt_ConfigSocket(byte socket_id, 
                                 byte context_id,  
                                 uint16_t min_pkt_size,       // in bytes  
                                 uint16_t inactivity_tmout,   // in sec.
                                 uint16_t connection_tmout,   // expressed in tenths of second  
                                 uint16_t data_sending_tmout  // expressed in tenths of second
)
{
  char ret_val = -1;
  char cmd[100];
  char tmp_str[10];

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // set Escape Prompt Delay = minimum time before "+++" to 20*1/50sec. = 20*20msec. = 400msec.
  SendATCmdWaitRespF(PSTR("ATS12=20"), 500, 20, "OK", 3);

  // AT#SCFG = 2, 3, 512, 30, 300,100
  strcpy_P(cmd, PSTR("AT#SCFG="));
  // connection ID  = socket id in rANGE 1..6
  strcat(cmd, itoa(socket_id, tmp_str, 10));
  strcat_P(cmd, PSTR(",")); // add character ,

  // add CONTEXT id
  strcat(cmd, itoa(context_id, tmp_str, 10));
  strcat_P(cmd, PSTR(",")); // add character ,

  // add Pkt sz -the minimum data packet sent to the net
  strcat(cmd, itoa(min_pkt_size, tmp_str, 10));
  strcat_P(cmd, PSTR(",")); // add characters ,"

  // add inactivity tmout
  strcat(cmd, itoa(inactivity_tmout, tmp_str, 10));
  strcat_P(cmd, PSTR(",")); // add characters ,"

  // add connection_tmout
  strcat(cmd, itoa(connection_tmout, tmp_str, 10));
  strcat_P(cmd, PSTR(",")); // add characters ,"

  // add data_sending_tmout
  strcat(cmd, itoa(data_sending_tmout, tmp_str, 10));
 

  // send AT command and waits for the response "CONNECT" - max. 3 times
  ret_val = SendATCmdWaitResp(cmd, 20000, 200, "OK", 3);
  if (ret_val == AT_RESP_OK) {
    ret_val = 1;
  }
  else {
    ret_val = 0;
  }
  
  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method suspend the socket in IP Easy Extended mode

<connection_id> - socket id: 1..6


return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free

        OK ret val:
        -----------
        0 - socket was not suspended
        1 - socket was successfully suspended


an example of usage:

        GSM gsm;
        gsm.IPEasyExt_ResumeSocket(1);
**********************************************************/
char GSM::IPEasyExt_SuspendSocket(byte connection_id)
{
  char ret_val = -1;
  byte* rx_data;

  // sequence "+++" will be sent but this sequence must be sent with some delay before "+++"
  // 
  // make dalay 500msec. before escape seq. "+++"
  RcvData(500, 100, &rx_data); // trick - function is used for generation a delay
  // send escape sequence +++ and wait for "NO CARRIER"
  SendData("+++");
  if (FindUntil("OK", "NO CARRIER", 1000)) {
    // OK was found => socket was suspended and now we are in command mode
    ret_val = 1;
  }
  else {
    // timeout occured or "NO CARRIER" was found
    // now we dont knw if it was NO CARRIER or it was timeout
    ret_val = 1;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method resumes the socket in IP Easy Extended mode

<connection_id> - socket id: 1..6


return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line is not free

        OK ret val:
        -----------
        0 - socket was not resumed
        1 - socket was successfully resumed


an example of usage:

        GSM gsm;
        gsm.IPEasyExt_ResumeSocket(1);
**********************************************************/
char GSM::IPEasyExt_ResumeSocket(byte connection_id)
{
  char ret_val = -1;
  char cmd[10];
  char tmp_str[5];

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // AT#SO=1 (1 here is connection_id = 1)
  strcpy_P(cmd, PSTR("AT#SO="));
  // connection ID
  strcat(cmd, itoa(connection_id, tmp_str, 10));
  // send the string
  Println(cmd);
  
  //wait for response
  // here we can not use SendATCmdWaitResp becuase we need finish immediatelly CONNECT
  // is received
  if (FindUntil("CONNECT", "NO CARRIER", 5000)) {
    // CONNECT was found
    SetCommLineStatus(CLS_DATA);
    ret_val = 1;
  }
  else {
    // timeout occured or "NO CARRIER" was found
    // so CONNECT string has not been find
    SetCommLineStatus(CLS_FREE);
    ret_val = 0;
  }

  return (ret_val);
}


/**********************************************************
Method closes previously opened socket in IP Easy Extended mode

<connection_id> - socket id: 1..6
<send_ESC_seq_before> - 0 nothing is sent before close AT command
                      - 1 ESC sequence "+++" is sent before close AT command

return: 
        ERROR ret. val:
        ---------------

        OK ret val:
        -----------
        1 - socket was successfully closed


an example of usage:

        GSM gsm;
        gsm.IPEasyExt_CloseSocket();
**********************************************************/
char GSM::IPEasyExt_CloseSocket(byte connection_id, byte send_ESC_seq_before)
{
  char ret_val = -1;
  byte* rx_data;
  char cmd[10];
  char tmp_str[5];


  // socket can be closed any time so we can not check the state of the line
  // if it is in DATA or COMMAND state

  if (send_ESC_seq_before) {
    // sequence "+++" will be sent but this sequence must be sent with some delay before "+++"
    // 
    // make dalay 500msec. before escape seq. "+++"
    RcvData(1500, 100, &rx_data); // trick - function is used for generation a delay
    // send escape sequence +++ and wait for "NO CARRIER"
    SendData("+++");
    if (FindUntil("OK", "NO CARRIER", 2000)) {
      // OK was found => socket was suspended and now we are in command mode

    }
    else {
      // timeout occured or "NO CARRIER" was found
      // now we dont knw if it was NO CARRIER or it was timeout
      // but anyway we can try send just AT to be sure we are in command mode
      ret_val = SendATCmdWaitRespF(PSTR("AT"), 500, 20, "OK", 3);
      
      switch (ret_val) {
        AT_RESP_ERR_NO_RESP:
          // it is strange
          break;

        AT_RESP_ERR_DIF_RESP:
        AT_RESP_OK:
          // we are in command mode for sure
          break;
      }
    }
  }

  // AT#SH=1 1 (1 here is connection_id = 1)
  strcpy_P(cmd, PSTR("AT#SH="));
  // connection ID
  strcat(cmd, itoa(connection_id, tmp_str, 10));
  ret_val = SendATCmdWaitResp(cmd, 500, 20, "OK", 3);

  SetCommLineStatus(CLS_FREE);
  return (1);
}

/**********************************************************
Method returns socket status in IP Easy Extended mode

<connection_id> - socket id: 1..6

return: 
        ERROR ret. val:
        ---------------
        -1 - not in command mode

        OK ret val:
        -----------
        0 – Socket Closed.
        1 – Socket with an active data transfer connection.
        2 – Socket suspended.
        3 – Socket suspended with pending data.
        4 – Socket listening.
        5 – Socket with an incoming connection. Waiting for the user accept or shutdown command
        10 - ERROR response

an example of usage:

        GSM gsm;

        char socket_status;
        socket_status = gsm.IPEasyExt_GetSocketStatus();
**********************************************************/
char GSM::IPEasyExt_GetSocketStatus(byte connection_id)
{
  signed short ret_val = -1;
  char cmd[10];
  char tmp_str[5];
  char *p_resp;
  char *p_buf = (char*)&gsm.comm_buf[0];


  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);

  // AT#SI=1 (1 here is connection_id = 1)

  // better is AT#SS=1
  strcpy_P(cmd, PSTR("AT#SS="));
  // connection ID
  strcat(cmd, itoa(connection_id, tmp_str, 10));

  ret_val = SendATCmdWaitResp(cmd, 500, 20, "OK", 3);

  if (ret_val == AT_RESP_OK) {
    // response example to SI: #SI: 1,123,400,10,50 OK
    // where items are:
    //• Data sent on the socket.
    //• Data extracted from the socket buffer.
    //• Data pending on the socket buffer.
    //• Data not acknowledged by the remote.

    // but we are using SS: #SS: 1,2,217.201.131.110,1033,194.185.15.73,10510
    //                      #SS: 2,3,217.201.131.110,1034,194.185.15.73,10510
    // #SS: <ConnId>,<Status>,<Local IP>,<Local Port>,<Remote IP>,<Remote Port>
    /*
    The Status field represents the socket status:
    0 – Socket Closed.
    1 – Socket with an active data transfer connection.
    2 – Socket suspended.
    3 – Socket suspended with pending data.
    4 – Socket listening.
    5 – Socket with an incoming connection. Waiting for the user accept or shutdown command.
    */

    // 

    p_resp = gsm.Skip(p_buf, ',');                // first ","
    // we are on the <Status>
    ret_val = strtol(p_resp, NULL, 10);
  }
  else {
    ret_val = 10; // an error response
  }

  SetCommLineStatus(CLS_FREE);
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
  Print(str_data);
}


void GSM::SendData(const char* str_data)
{
  Print(str_data);
}


void GSM::SendDataF(PGM_P str_data)
{
  char c;
  
  while ((c = pgm_read_byte(str_data++)) != 0)
    PrintChar(c);
}


void GSM::SendData(byte* data_buffer, unsigned short size)
{
  Write(data_buffer, size);
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
      ret_val = SendATCmdWaitRespF(PSTR("AT"), 1000, 100, "OK", 1);
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

