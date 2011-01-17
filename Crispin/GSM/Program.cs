using System;
using System.Threading;
using Microsoft.SPOT;
using Microsoft.SPOT.Hardware;
using SecretLabs.NETMF.Hardware;
using SecretLabs.NETMF.Hardware.Netduino;

namespace GSM
{
    public class Program
    {
        
        
        
        static InterruptPort swAnswer = new InterruptPort(Pins.GPIO_PIN_D10, true, Port.ResistorMode.PullUp, Port.InterruptMode.InterruptEdgeLow);
        static InterruptPort swHangUp = new InterruptPort(Pins.GPIO_PIN_D11, true, Port.ResistorMode.PullUp, Port.InterruptMode.InterruptEdgeLow);
        static InterruptPort swCall = new InterruptPort(Pins.GPIO_PIN_D12, true, Port.ResistorMode.PullUp, Port.InterruptMode.InterruptEdgeLow);
        static InterruptPort swMisc = new InterruptPort(Pins.GPIO_PIN_D13, true, Port.ResistorMode.PullUp, Port.InterruptMode.InterruptEdgeLow);
        //static OutputPort led = new OutputPort(Pins.ONBOARD_LED, false);
        static GSM gsm = new GSM();
        public static void Main()
        {
            swAnswer.OnInterrupt += new NativeEventHandler(swAnswer_OnInterrupt);
            swHangUp.OnInterrupt += new NativeEventHandler(swHangup_OnInterrupt);
            swCall.OnInterrupt += new NativeEventHandler(swCall_OnInterrupt);
            swMisc.OnInterrupt += new NativeEventHandler(swMisc_OnInterrupt);

            gsm.OnNetworkRegisterChange += new GSM.NetworkRegistered(gsm_OnNetworkRegisterChange);
            gsm.OnSignalStrengthChange += new GSM.SignalStrengthChange(gsm_OnSignalStrengthChange);
            //if (gsm.Status != GSM.GSM_Status.Registered)
            gsm.TurnOn();

//            gsm.testc();
            Thread.Sleep(Timeout.Infinite);

        }

        static void gsm_OnSignalStrengthChange(int SignalStrength)
        {
            Debug.Print("Signal Changed to: " + SignalStrength.ToString());
        }

        static void gsm_OnNetworkRegisterChange(GSM.GSM_Status Status)
        {
            if (Status == GSM.GSM_Status.NotRegisteredNotSearching)
                Debug.Print("Not Registered Not Searching");
            else if (Status == GSM.GSM_Status.NotRegisteredSearching)
                Debug.Print("Not Registered, Searching");
            else if (Status == GSM.GSM_Status.Registered)
                Debug.Print("Registered");
            else if (Status == GSM.GSM_Status.RegisteredRoaming)
                Debug.Print("Registered - Roaming");
            else if (Status == GSM.GSM_Status.RegistrationDenied)
                Debug.Print("Registration Denied.");
            else
                Debug.Print("unknonw..");
        }

        static void swMisc_OnInterrupt(uint data1, uint data2, DateTime time)
        {
            //gsm.AutoAnswerIncomingCalls(0);
            gsm.Test();
        }

        static void swCall_OnInterrupt(uint data1, uint data2, DateTime time)
        {
            //string xx = gsm.SendSerial("AT+E0");
            //Debug.Print(xx);
            gsm.Reset();
            gsm.TurnOn();
            //gsm.Call("+447891987932");
            //Debug.Print("Calling");
        }

        static void swAnswer_OnInterrupt(uint data1, uint data2, DateTime time)
        {
//            led.Write(true);
           gsm.Answer();
        }
        static void swHangup_OnInterrupt(uint data1, uint data2, DateTime time)
        {
  //          led.Write(false);
            gsm.Hangup();
        }

    }
}
