#define UseLocallyBuiltRCCService

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using Roblox.Common;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.IO;

namespace RCCService.Test
{
    public static class RCCLauncher
    {
        private const string buildType = "Mixed Platforms";
        private const string buildTarget = "Release";
#if UseLocallyBuiltRCCService
        // use for local tests:
        internal static string RCCServicePath = @"..\..\..\RccService\bin\" + buildTarget + @"\RccService.exe";
        // use for automatic tfs build tests:
        //internal static string RCCServicePath = @"..\..\..\Binaries\Mixed Platforms\" + buildTarget + @"\RCCService.exe";

        // use for local tests:
        internal static string RCCServiceArgs = "/console /content:..\\..\\..\\content\\ {0} ";
        // note: content for testing resides in:
        //       "C:\Users\Administrator\AppData\Local\Temp\Doblox2\client_mixed\Binaries\Mixed Platforms\Debug"
        // test invoked from:
        //        C:\Users\Administrator\AppData\Local\Temp\Doblox2\client_mixed\TestResults\Administrator_VOSTRO420 2009-07-01 11_45_00_Mixed Platforms_Debug\Out
        //internal static string RCCServiceArgs = "/console /content:..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\share\\content\\ {0} ";
#else
        private const string RCCServicePath = @"S:\Automation\RCCService\bin\" + buildTarget + @"\RCCService.exe";
        private const string RCCServiceArgs = "/console {0} ";
#endif
        internal static string RemoteRCCServicePath = RCCServicePath;
        internal static string RemoteRCCServiceArgs = RCCServiceArgs;

        private const int rigPort = 8000;
        private const int portBase = 64989;
        private const int portIncrement = -1;
        private static int nextAvailablePort = portBase;

        static private HashSet<Uri> DeadServicesSet = new HashSet<Uri>();
        static private HashSet<Uri> PendingServicesSet = new HashSet<Uri>();
        static private Dictionary<Uri, Process> RCCServicesMap = new Dictionary<Uri, Process>();

        static public bool IsServiceAvailable(string host, int port, int retrycount)
        {
            // busy wait untill our service is ready to accept connections
            bool bAvailable = false;
            int waitcount = 0;
            while (!bAvailable) 
            {
                System.Net.Sockets.TcpClient tcp = new TcpClient();
                try
                {
                    tcp.Connect(host, port);
                    tcp.Close();
                    bAvailable = true;
                }
                catch (System.Net.Sockets.SocketException)
                {
                    if (++waitcount > retrycount)
                    {
                        bAvailable = false;
                        break;
                    }
                    Thread.Sleep(1000);
                }
            }

            return bAvailable;
        }

        static public Uri StartRCCService()
        {
            int port = nextAvailablePort;
            nextAvailablePort += portIncrement;

            /*
             *  FIND THE CORRECT SERVICE AND ARGS PATH FOR LOCAL OR BUILD MACHINE RUN
             */
            const string buildTarget = "Release";
            string[] RCCSearchPaths = new string[] {
                System.Environment.GetEnvironmentVariable("RCCsHappyHome", EnvironmentVariableTarget.Machine) + @"\RccService\Win32\" + buildTarget + @"\RccService.exe",
                @"..\..\..\..\RccService\Win32\" + buildTarget + @"\RccService.exe",
                @"..\..\..\RccService\Win32\" + buildTarget + @"\RccService.exe",
                @"..\..\..\Binaries\Mixed Platforms\" + buildTarget + @"\RccService.exe"
            };

            const string args_LOCAL = "/console /content:..\\..\\..\\content\\ {0} ";

            bool found = false;
            foreach (string path in RCCSearchPaths)
            {
                if (File.Exists(path))
                {
                    RCCLauncher.RCCServicePath = path;
                    RCCLauncher.RCCServiceArgs = args_LOCAL;
                    RCCLauncher.RemoteRCCServicePath = path;
                    RCCLauncher.RemoteRCCServiceArgs = args_LOCAL;
                    found = true;
                    break;
                }
            }

            if (found)
            {


                Process proc = new Process();
                proc.StartInfo.FileName = RCCServicePath;  // AutoRCCTest.RCCServicePath;
                proc.StartInfo.Arguments = String.Format(RCCServiceArgs, port);


                string dirr = Directory.GetCurrentDirectory();
                /*
                // write to a file the directory location
                System.IO.StreamWriter file = new System.IO.StreamWriter("c:\\dirr.txt");
                file.WriteLine(dirr);
                file.Close();
                */

                proc.Start();


                Uri result = new Uri("http://" + TestUtils.GetLocalIpAddress().ToString() + ":" + port.ToString());

                if (!IsServiceAvailable(result.Host, result.Port, 40))
                {
                    throw new Exception("Unable to connect to Launched RCCService");
                }

                RCCServicesMap.Add(result, proc);

                return result;
            }
            throw new Exception("Cannot find RccService");
        }

        // for tests that don't care if they reuse a service.
        // (they should not close the service either, for this to be useful)
        static public Uri FindOrStartRCCService()
        {
            Uri result2 = new Uri("http://" + TestUtils.GetLocalIpAddress().ToString() + ":64989");
            //Uri result2 = new Uri("http://localhost.:64989");

            if (IsServiceAvailable(result2.Host, result2.Port, 20))
            {
                return result2;
            }

            if (RCCServicesMap.Count == 0)
            {
                return StartRCCService();
            }
            else
            {
                return RCCServicesMap.First().Key;
            }
        }
        delegate Uri getOneDelegate();

        static public List<Uri> FindNMoreRemoteRCCServices(int n)
        {
            List<IAsyncResult> results = new List<IAsyncResult>();

            List<Uri> urilist = new List<Uri>();

            getOneDelegate getOne = delegate()
            {
                return FindRemoteRCCService();
            };

            int pendingDelegates = 0;

            int bNeedMore = n - (urilist.Count + pendingDelegates);
            while (bNeedMore > 0)
            {
                // make sure we always have enough pending calls to cover our dept.
                while ((bNeedMore - pendingDelegates) > 0)
                {
                    results.Add(getOne.BeginInvoke(null, null));
                    pendingDelegates++;
                }

                // process results.
                for (int i = results.Count - 1; i != -1; --i)
                {
                    if (results[i].IsCompleted)
                    {
                        urilist.Add(getOne.EndInvoke(results[i]));
                        results.RemoveAt(i);
                        pendingDelegates--;
                    }
                }

                lock (Lists.remoteServiceUris)
                {
                    bNeedMore = n - urilist.Count;
                }

            }

            return urilist;
        }

        static public Uri FindRemoteRCCService()
        {
            for (int i = 0; i < Lists.remoteServiceUris.Count(); ++i)
            {
                Uri testinguri;
                lock (Lists.remoteServiceUris)
                {
                    if (!RCCServicesMap.ContainsKey(Lists.remoteServiceUris[i]) &&
                        !DeadServicesSet.Contains(Lists.remoteServiceUris[i]) &&
                        !PendingServicesSet.Contains(Lists.remoteServiceUris[i]))
                    {
                        testinguri = Lists.remoteServiceUris[i];
                        PendingServicesSet.Add(testinguri);
                    }
                    else
                    {
                        continue;
                    }
                }

                {
                    if (IsServiceAvailable(testinguri.Host, testinguri.Port, 0))
                    {
                        lock (Lists.remoteServiceUris)
                        {
                            RCCServicesMap.Add(testinguri, null);
                            PendingServicesSet.Remove(testinguri);
                        }
                        return testinguri;
                    }
                    else if (IsServiceAvailable(testinguri.Host, rigPort, 0))
                    {
                        // service is not up, but test rig is available.
                        // spawn a rccservice.
                        StartRemoteRCCService(testinguri);
                        lock (Lists.remoteServiceUris)
                        {
                            RCCServicesMap.Add(testinguri, null);
                            PendingServicesSet.Remove(testinguri);
                        }
                        return testinguri;
                    }
                    else
                    {
                        Console.WriteLine(testinguri + " is dead.");
                        // skip pinging this one next time.
                        lock (Lists.remoteServiceUris)
                        {
                            DeadServicesSet.Add(testinguri);
                            PendingServicesSet.Remove(testinguri);
                        }
                    }
                }
            }

            throw new Exception("Not enough available services to fulfull request. Try adding/correcting the remoteServiceUris list");
        }

        static public void StartRemoteRCCService(Uri uri)
        {
            string addressFormat = "http://{0}:8000/Roblox.Test/RunService";
            string result;
            int pid = 0;
            Roblox.Test.Client.RunClient runclient = new Roblox.Test.Client.RunClient("BasicHttpBinding_IRun", String.Format(addressFormat, uri.Host));
            result = runclient.SpawnProcess(out pid, RemoteRCCServicePath, String.Format(RemoteRCCServiceArgs, uri.Port));
            Console.WriteLine(result);

            if (!IsServiceAvailable(uri.Host, uri.Port, 20))
            {
                throw new Exception("Unable to connect to Launched RCCService");
            }
        }

        static public void StopRCCService(Uri uri)
        {
            Process proc = RCCServicesMap[uri];
            if (proc != null && !proc.HasExited)
            {
                proc.Kill();
            }
            RCCServicesMap.Remove(uri);
        }

        static public void StopAllRCCServices()
        {
            foreach (System.Collections.Generic.KeyValuePair<Uri, Process> item in RCCServicesMap)
            {
                if (item.Value != null)
                {
                    item.Value.Kill();
                }
            }
            RCCServicesMap.Clear();

            nextAvailablePort = portBase;
        }


    }
}
