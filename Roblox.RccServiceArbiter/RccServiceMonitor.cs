using System;
using System.Collections.Generic;
using System.Text;
using System.ServiceModel;

namespace Roblox.RccServiceArbiter
{
    
    [ServiceBehavior(InstanceContextMode = InstanceContextMode.Single, ConcurrencyMode = ConcurrencyMode.Multiple)]
    class RccServiceMonitor :
        IArbiter
    {
        RccService rccService;

        public RccServiceMonitor(RccService rccService)
        {
            this.rccService = rccService;
        }

        public string GetStats()
        {
            return rccService.GetStats(false);
        }

        public string GetStatsEx(bool clearExceptions)
        {
            return rccService.GetStats(clearExceptions);
        }

        public void SetMultiProcess(bool value)
        {
            rccService.SetMultiProcess(value);
        }

        public void SetRecycleProcess(bool value)
        {
            rccService.SetRecycleProcess(value);
        }

        public void SetRecycleQueueSize(int value)
        {
            rccService.SetRecycleProcessCount(value);
        }

        public void SetThreadConfigScript(string script, bool broadcast)
        {
            rccService.SetThreadConfigScript(script, broadcast);
        }
    }
}
