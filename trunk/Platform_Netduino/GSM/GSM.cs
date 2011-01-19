using System;
using System.Threading;
using Microsoft.SPOT;
using Microsoft.SPOT.Hardware;
using System.IO.Ports;
using SecretLabs.NETMF.Hardware;
using SecretLabs.NETMF.Hardware.Netduino;

namespace GSM
{
    
    public class GSM
    {

        #region private variables
        public delegate void NetworkRegistered(GSM_Status Status);
        public delegate void SignalStrengthChange(int SignalStrength);

        static private SerialPort serialPort = new SerialPort("COM1", 115200);
        static private OutputPort pin_DTMF_ENABLE = new OutputPort(Pins.GPIO_PIN_D2, true);
        static private InputPort pin_DTMF_VALID = new InputPort(Pins.GPIO_PIN_D3, false, Port.ResistorMode.Disabled);
        static private OutputPort pin_RESET = new OutputPort(Pins.GPIO_PIN_D4, false);
        static private OutputPort pin_ONOFF = new OutputPort(Pins.GPIO_PIN_D5, false);
        private Timer _t_CheckNetwork = null;
        private Timer _t_CheckSignalStrength = null;
        private Timer _t_WatchDog = null;
        static int GSMWatchDogCount = 0;

        #endregion


        #region private properties
        
        #endregion

        #region enums

        private enum GSM_AT_Resp
        {
            AT_RESP_ERR_NO_RESP = -1,   // nothing received
            AT_RESP_ERR_DIF_RESP = 0,   // response_string is different from the response
            AT_RESP_OK = 1,             // response_string was included in the response
        }

        public enum GSM_Status
        {
            NotRegisteredNotSearching = 0,
            Registered = 1,
            NotRegisteredSearching = 2,
            RegistrationDenied=3,
            RegisteredRoaming = 5,
            Unknown = -1
        }

        public enum Call_Status
        {
            Ready = 0,
            Unavailable = 1,
            Status_Unknown = 2,
            Ringing = 3,
            Call_In_Progress = 4,
            Asleep = 5
        }

        #endregion

        #region public events

        public event NetworkRegistered OnNetworkRegisterChange;
        public event SignalStrengthChange OnSignalStrengthChange;
        
        #endregion

        #region public properties

        public Call_Status CallStatus
        {
            get;
            private set;
        }

        public int SignalStrength { get; private set; }

        public string NetworkName
        {
            get
            {
                string resp = SendSerial("AT+COPS?");
                if (resp.Length == 0)
                    return "";
                int start = resp.IndexOf('"');
                if (start < 0)
                    return "";
                int _end = resp.IndexOf('"', start+1);
                if (_end < 0)
                    return "";

                string ret = resp.Substring(start+1, _end-start-1);
                return ret;
            }
        }

        public double Temperature
        {
            get
            {
                string ret = SendSerial("AT#ADC=2,2,0");
                //"AT#ADC=2,2,0#ADC: 846OK"
                if (ret.IndexOf("#ADC:") < 0)
                    return 0;
                string t = ret.Substring(ret.IndexOf("#ADC:") + 5, ret.Length - ret.IndexOf("#ADC:") - 7);

                return (Convert.ToDouble(t) - 600) / 10.0;
            }
        }

        public GSM_Status Status
        {
            get;
            private set;
        }
        #endregion

        public GSM()
        {
            _t_CheckNetwork = new Timer(new TimerCallback(CheckStatus), null, 0, 1000);//every 5 seconds, check status of network.
            _t_CheckSignalStrength = new Timer(new TimerCallback(GetSignalStrength), null, 0, 1000);//every 5 seconds, check status of network.
            _t_WatchDog = new Timer(new TimerCallback(WatchDog), null, 2, 5000);//every 5 seconds, check status of network.

            InitModule();
        }

        public void TurnOn()
        {
            GSM_AT_Resp res = SendATCmdWaitResp("AT", "OK", 5);

            if (res == GSM_AT_Resp.AT_RESP_ERR_NO_RESP || res == GSM_AT_Resp.AT_RESP_ERR_DIF_RESP)
            {
                // there is no response => turn on the module
                pin_ONOFF.Write(true);
                System.Threading.Thread.Sleep(1200);
                pin_ONOFF.Write(false);
            }
        }

        public void Test()
        {
            string ret = SendSerial("AT+CPAS");
//            Call_Status cs = Call_Status.Status_Unknown;

            if (ret.IndexOf("CPAS:") < 0)
            {
                CallStatus = Call_Status.Status_Unknown;
                return;
            }
            //need to sort out the serial buffer - still has old data in it....
            //"+CSQ: 12,0OKAT+CPASAT+CREG?+CPAS: 0OK"
            Debug.Print(ret);

        }

        /// <summary>
        /// Set to zero for off, any number between 1 and 255 rings for auto answer.
        /// </summary>
        /// <param name="rings">Set to zero for off, any number between 1 and 255 rings for auto answer.</param>
        public void AutoAnswerIncomingCalls(int rings)
        {
            SendSerial("ATS0=" + rings.ToString());
        }

        public void TurnOff()
        {
            if (SendATCmdWaitResp("AT", "OK", 5) == GSM_AT_Resp.AT_RESP_ERR_NO_RESP)
                return;
            
                //there was a valid response. Module is on. Turn it off.
                pin_ONOFF.Write(true);
                System.Threading.Thread.Sleep(1200);
                pin_ONOFF.Write(false);
        }

        public void Reset()
        {
            Debug.Print("Resetting...");
            pin_RESET.Write(true);
            Thread.Sleep(500);
            pin_RESET.Write(false);
        }

        public bool Call(String number)
        {
            if (number.Substring(0, 1) != "+")
                throw new Exception("Number not correct format");

            SendSerial("ATD" + number + ";");

            return true;
        }

        #region private methods
        private void GetSignalStrength(object state)
        {
            string ret = SendSerial("AT+CSQ", 1000);
            if (ret.IndexOf("+CSQ:") < 0)
                return;
            string[] x = ret.Substring(ret.IndexOf(":") + 1).Trim().Split(',');
            int y = 0;

            if (x[0] == "" || x.Length == 0)
                y = 0;
            else
                y = 10 - map(Convert.ToInt32(x[0]), 0, 31, 0, 10);

            if (y != SignalStrength)
            {
                SignalStrength = y;
                OnSignalStrengthChange.Invoke(SignalStrength);
            }
        }

        private void CheckStatus(object state) {
            string ret = SendSerial("AT+CREG?");

            if (ret.IndexOf("+CREG:") < 0)
            {
                Status = GSM_Status.Unknown;
                return;
            }
            
            GSM_Status s = GSM_Status.Unknown;
            //"AT+CREG?+CREG: 0,1OK"
            if (ret.IndexOf("+CREG: 0,0") >= 0)
                s = GSM_Status.NotRegisteredNotSearching;
            else if (ret.IndexOf("+CREG: 0,1") >= 0)
                s = GSM_Status.Registered;
            else if (ret.IndexOf("+CREG: 0,2") >= 0)
                s = GSM_Status.NotRegisteredSearching;
            else if (ret.IndexOf("+CREG: 0,3") >= 0)
                s = GSM_Status.RegistrationDenied;
            else if (ret.IndexOf("+CREG: 0,4") >= 0)
                s = Status; //seeing as 0,4 is unknown repsonse, leave the status as it is.
            else if (ret.IndexOf("+CREG: 0,5") >= 0)
                s = GSM_Status.RegisteredRoaming;
            else
            {
                Debug.Print("Registration: " + ret);
                s = GSM_Status.Unknown;
            }
            if (s != Status)
            {
                Status = s;
                OnNetworkRegisterChange.Invoke(s);
            }
        }

        private int map(int x, int in_min, int in_max, int out_min, int out_max)
        {
            return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
        }

        static private void OpenSerial()
        {
            if (!serialPort.IsOpen)
                serialPort.Open();

            if (serialPort.BytesToRead > 0)
            {
                Debug.Print("Still have data in output buffer");

                byte[] buffOut = new byte[1];
                string ret = "";
                while (serialPort.BytesToRead > 0)
                {
                    serialPort.Read(buffOut, 0, 1);
                    //if (buffOut[0] < 32 || buffOut[0] > 127)
                    //    continue;
                    ret += new string(System.Text.UTF8Encoding.UTF8.GetChars(buffOut));
                }
                Debug.Print(ret);
            }
            serialPort.DiscardInBuffer();
            serialPort.DiscardOutBuffer();
        }

        private GSM_AT_Resp SendATCmdWaitResp(string ATCommand, string ExpectedResponse, int responeTimeout)
        {
            string resp = SendSerial(ATCommand, responeTimeout);
            if (resp == ExpectedResponse)
                return GSM_AT_Resp.AT_RESP_OK;

            return GSM_AT_Resp.AT_RESP_ERR_DIF_RESP;

        }

        private void InitModule()
        {
            // Audio codec - Full Rate (for DTMF usage)
            SendSerial("AT#CODEC=1");
            // Hands free audio path
            SendSerial("AT#CAP=1");
            // Echo canceller enabled 
            SendSerial("AT#SHFEC=1");
            // Ringer tone select (0 to 32)
            SendSerial("AT#SRS=26,0");
            // Microphone gain (0 to 7) - response here sometimes takes 
            // more than 500msec. so 1000msec. is more safety
            SendSerial("AT#HFMICG=7");
            // set the SMS mode to text 
            SendSerial("AT+CMGF=1");
            // Auto answer after first ring enabled
            // auto answer is not used
            //SendATCmdWaitResp("ATS0=1", 500, 20, "OK", 5);

            // select ringer path to handsfree
            SendSerial("AT#SRP=1");
            // select ringer sound level
            SendSerial("AT+CRSL=2");
            // we must release comm line because SetSpeakerVolume()
            // checks comm line if it is free
            // select speaker volume (0 to 14)
//            SetSpeakerVolume(9);
            // init SMS storage
//            InitSMSMemory();
            // select phonebook memory storage
            SendSerial("AT+CPBS=\"SM\"");
        }

        static public string SendSerial(string ATCommand)
        {
            return SendSerial(ATCommand, 500);
        }
        static string SendSerial(string ATCommand, int responeTimeout)
        {
            Debug.Print("Serial Start " + ATCommand);
            //ATCommand;
            byte[] buff;
            buff = System.Text.Encoding.UTF8.GetBytes(ATCommand + (char)13);

            OpenSerial();

            serialPort.Write(buff, 0, buff.Length);

            DateTime dtFinishBy = DateTime.Now.AddMilliseconds(responeTimeout);

            DateTime dtStart = DateTime.Now;
            //wait until there is a response or timeout
            while (serialPort.BytesToRead == 0 && DateTime.Now < dtFinishBy)
            {
                //Debug.Print("Waiting....");
                Thread.Sleep(10);
            }
            Debug.Print("Done Waiting.... " + (DateTime.Now - dtStart).Seconds.ToString() + ":" + (DateTime.Now - dtStart).Milliseconds.ToString());
            byte[] buffOut = new byte[1];
            string ret = "";
            while (serialPort.BytesToRead > 0)
            {
                serialPort.Read(buffOut, 0, 1);
                if ((buffOut[0] < 32 || buffOut[0] > 127) && !(buffOut[0] == 13 || buffOut[0] == 10))
                    continue;
                ret += new string(System.Text.UTF8Encoding.UTF8.GetChars(buffOut));
                if (serialPort.BytesToRead == 0)
                    Thread.Sleep(100);

            }
            Debug.Print("Serial End");
            return ret;
        }

        static void WatchDog(object state)
        {

            if (serialPort.BytesToRead == 0)
            {
                GSMWatchDogCount++;
                //Debug.Print("Timeout for command: " + ATCommand);
                if (GSMWatchDogCount == 10)
                {
                    GSMWatchDogCount = 0;
                    //turn off then on.
                    pin_ONOFF.Write(true);
                    System.Threading.Thread.Sleep(1200);
                    pin_ONOFF.Write(false);

                    System.Threading.Thread.Sleep(1200);

                    pin_ONOFF.Write(true);
                    System.Threading.Thread.Sleep(1200);
                    pin_ONOFF.Write(false);
                    Debug.Print("Turning on!");
                    Thread.Sleep(5000);
                }
                Debug.Print("Serial End");
                return;
            }

        }

        #endregion

        #region public methods
        public void Answer()
        {
            string resp = SendSerial("ATA");
            Debug.Print(resp);
        }

        public void Hangup()
        {
            SendSerial("ATH");
        }

        /*
        private void blink(int count){
            for (int i = 0; i <= count; i++)
            {
                led.Write(!led.Read());
                Thread.Sleep(250);
            }
            led.Write(false);
        }
        */
        #endregion

    }
}
