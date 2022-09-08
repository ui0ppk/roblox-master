using System;
using System.Collections.Generic;
using System.Text;

namespace Roblox.RccServiceArbiter
{
    /// <summary>
    /// Taken from http://www.mattbrindley.com/developing/windows/net/detecting-the-next-available-free-tcp-port/
    /// </summary>
    public static class TcpPort
    {
        private const string PortReleaseGuid = "8875BD8E-4D5B-11DE-B2F4-691756D89593";

        /// <summary>
        /// Check if startPort is available, incrementing and
        /// checking again if it's in use until a free port is found
        /// </summary>
        /// <param name="startPort">The first port to check</param>
        /// <returns>The first available port</returns>
        public static int FindNextAvailablePort(int startPort)
        {
            int port = startPort;
            bool isAvailable = true;

            var mutex = new System.Threading.Mutex(false,
                string.Concat("Global/", PortReleaseGuid));
            mutex.WaitOne();
            try
            {
                System.Net.NetworkInformation.IPGlobalProperties ipGlobalProperties =
                    System.Net.NetworkInformation.IPGlobalProperties.GetIPGlobalProperties();
                System.Net.IPEndPoint[] endPoints =
                    ipGlobalProperties.GetActiveTcpListeners();

                do
                {
                    if (!isAvailable)
                    {
                        port++;
                        isAvailable = true;
                    }

                    foreach (System.Net.IPEndPoint endPoint in endPoints)
                    {
                        if (endPoint.Port != port) continue;
                        isAvailable = false;
                        break;
                    }

                } while (!isAvailable && port < System.Net.IPEndPoint.MaxPort);

                if (!isAvailable)
                    throw new Exception("NoAvailablePortsInRangeException");

                return port;
            }
            finally
            {
                mutex.ReleaseMutex();
            }
        }
    }
}
