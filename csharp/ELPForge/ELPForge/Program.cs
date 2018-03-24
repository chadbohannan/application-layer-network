using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Threading.Tasks;

namespace ELPForge
{
    class Program
    {
        class PacketHandler : ELP.IPacketHandler
        {
            public void OnPacket(ELP.Packet packet)
            {
                Console.Out.WriteLine(String.Format("Packet recieved; {0} data bytes", packet.Data.Length));
            }
        }

        static void Main(string[] args)
        {
            ELP.Parser parser = new ELP.Parser(new PacketHandler());
            using (FileStream input = File.OpenRead("packetstream.input"))
            {
                byte[] byteArray = new byte[1];
                for (int i = 0; i < input.Length; i++)
                {
                    int value = input.ReadByte();
                    byteArray[0] = (byte)value;
                    parser.readBytes(byteArray);
                }
                
            }
            Console.Out.WriteLine("press Enter to quit");
            string r  = Console.In.ReadLine();
        }
    }
}
