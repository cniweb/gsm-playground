/*
 *  This sketch demonstrates how to send AT command directly from the sketch 
 *  and how to receive a response
 *  Released under the Creative Commons Attribution-Share Alike 3.0 License
 *  http://www.creativecommons.org/licenses/by-sa/3.0/
 *  www.hwkitchen.com
 */

#include "GSM.h"  

#ifndef DEBUG_PRINT
  #error "!!! It is necessary to enable DEBUG_PRINT macro in the GSM.h !!!"
#endif


// definition of instance of GSM class
GSM gsm;

// return variable
signed char ret_val;


void setup()
{
  // initialization of serial line
  gsm.InitSerLine(115200);
  // turn on GSM module
  gsm.TurnOn();
  
  #ifdef DEBUG_PRINT
    // print library version
    gsm.DebugPrint("DEBUG Library version: ", 0);
    gsm.DebugPrint(gsm.LibVer(), 1);
  #endif
}

void loop()
{
  // now we are sending some AT command
  // and waiting for the response
  // 
  // the GSM module must start with sending response in 1000 msec.
  // and the response is taken as finished when there is no other
  // incomming character longer then 100 msec. 
  // ""   means that we are not expecting certain response string
  //      so we will accept any response
  // 1    means that we will sed AT command just one time
  // --------------------------------------------------
  ret_val = gsm.SendATCmdWaitResp("AT+CCLK?", 1000, 100, "", 1);

  // now we are waiting for response from the GSM module
  // ---------------------------------------------------
  if (ret_val != AT_RESP_ERR_NO_RESP) {
    // finaly we have received something - print out the internal
    // buffer
    // ----------------------------------------------------------
#ifdef DEBUG_PRINT
    // print out some explaining tring
    // -------------------------------
    gsm.DebugPrint("Response on previous AT command is:\r\n", 0);
    gsm.DebugPrint("===================================\r\n", 0);
    // and now finaly print out the internal communication buffer content
    // intrenal communication buffer is automatically finished by the 0x00
    // inside the library function SendATCmdWaitResp() so it is possible
    // to prints it out as the string

    // as the com_buf is defined as byte it is necessary to retype it
    // to (const char*) to be compatible with library function DebugPrint()
    // ------------------------------------------------------------------
    gsm.DebugPrint((const char*)gsm.comm_buf, 1);
#endif
  }
  else {
    // this should not happen - we have received nothing
    // so maybe previous AT command was not recognized
    // becuase we have made some syntax error
    // or GSM module is not working or......
    // ---------------------------------------------------
#ifdef DEBUG_PRINT
    gsm.DebugPrint("We have received nothing, something is wrong...", 1);
#endif
  }

  // now we are finish so stay here for ever
  // if you want stay here following while(1) must not be commented by //
  // ---------------------------------------------------------------------
  //while (1);

  // or we can repeat previous AT command after some delay
  // -----------------------------------------------------
  delay(1000);
}
