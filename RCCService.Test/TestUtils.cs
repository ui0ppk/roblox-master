using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Roblox.Grid.Rcc;
using Roblox.Grid;
using System.Threading;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Net;

namespace RCCService.Test
{
    public static class TestUtils
    {
        public static void WaitForNClients(RCCServiceSoap serverService, string serverjobid, int numClients, int waitms)
        {
            const int pollsleep = 100;
            // poll for one second until all clients are available.
            int connectedclients = 0;
            int loopcount = 0;
            do
            {
                Thread.Sleep(pollsleep);
                LuaValue[] result = serverService.Execute(serverjobid, Lua.NewScript("GetClientCount", LuaScripts.GetClientCount));
                connectedclients = Int32.Parse(result[0].value);
            } while (connectedclients != numClients && loopcount++ < waitms / pollsleep);

            Assert.IsTrue(connectedclients >= numClients, "clients connected count");
        }

        public delegate void waitDelegate(object delegateParams);

        public static void PrintStatisticsAndWait(RCCServiceSoap serverService, string serverjobid, TestContext testcontext, int placerun, int numTimes, int periodms, waitDelegate wd, object delegateParams)
        {
            while (true)
            {
                LuaValue[] result;
                result = serverService.Execute(serverjobid, Lua.NewScript("GetPrivateWorkingSetBytes", "return settings().Diagnostics.PrivateWorkingSetBytes"));
                string privateWorkingSetBytes = result[0].value;

                result = serverService.Execute(serverjobid, Lua.NewScript("GetPrivateBytes", "return settings().Diagnostics.PrivateBytes"));
                string privateBytes = result[0].value;

                result = serverService.Execute(serverjobid, Lua.NewScript("GetProcessCores", "return settings().Diagnostics.ProcessCores"));
                string processCores = result[0].value;

                testcontext.WriteLine("{0}, {1}, {2}, {3}", placerun, privateWorkingSetBytes, privateBytes, processCores);

                if (wd != null)
                {
                    wd(delegateParams);
                }

                if (--numTimes <= 0)
                {
                    break;
                }
                Thread.Sleep(periodms);
            }

        }


        public static IPAddress GetLocalIpAddress()
        {
            System.Net.IPAddress[] a =
            System.Net.Dns.GetHostAddresses(System.Net.Dns.GetHostName());

            for (int i = 0; i < a.Length; i++)
            {
                if (a[i].AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
                {
                    return a[i];
                }
            }

            throw new Exception("Can't resolve machine's ip address");
        }

    }
}
