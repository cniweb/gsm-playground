using System;
using Microsoft.SPOT;
using System.IO.Ports;

namespace GSM
{
    public class LCD
    {
        static SerialPort lcd = new SerialPort("COM2", 9600);

        public LCD()
        {
            lcd.Open();
        }


        public void write(string message)
        {
            byte[] buff;
            buff = System.Text.Encoding.UTF8.GetBytes(message);
            lcd.Write(buff, 0, buff.Length);
        }
    }
}
