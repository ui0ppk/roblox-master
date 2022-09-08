using System;
using System.Collections.Generic;
using System.Text;

namespace Roblox.RccServiceArbiter
{
    class Program
    {
        static void Main(string[] args)
        {
            Log.Event("Starting");
            //Cleanup old RCCService instances
            System.Diagnostics.Process[] processlist = System.Diagnostics.Process.GetProcesses();
            foreach(System.Diagnostics.Process theprocess in processlist)
            {
                if (theprocess.ProcessName == "RCCService")
                {
                    theprocess.Kill();
                }
            }

            RccService rccService = new RccService();
            RccServiceMonitor rccServiceMonitor = new RccServiceMonitor(rccService);
            Roblox.ServiceProcess.ServiceBasePublic[] otherHosts = new Roblox.ServiceProcess.ServiceBasePublic[1];
            otherHosts[0] = new Roblox.ServiceProcess.ServiceHostApp<RccServiceMonitor>(rccServiceMonitor);
            var app = new Roblox.ServiceProcess.ServiceHostApp<RccService>(rccService, otherHosts);
            app.HostOpened += new EventHandler(delegate(object a,EventArgs b) { rccService.initializeJobManager(); });
            //var app = new Roblox.ServiceProcess.ServiceHostApp<RccServiceMonitor>(rccServiceMonitor);
            app.Process(args, delegate() { Roblox.Grid.Arbiter.Common.ArbiterStats stats = new Roblox.Grid.Arbiter.Common.ArbiterStats(rccService.GetStats(false));
                                           Console.Out.Write(stats.ToString());
            });
        }
    }
}
