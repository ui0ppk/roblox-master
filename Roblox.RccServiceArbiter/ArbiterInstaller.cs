using System;
using System.Collections.Generic;
using System.ComponentModel;

namespace Roblox.RccServiceArbiter
{
    [RunInstaller(true)]
    public class ArbiterInstaller : Roblox.ServiceProcess.ServiceHostInstaller
    {
        public override string ServiceName
        {
            get { return "Roblox.RccServiceArbiter"; }
        }

        public override string DisplayName
        {
            get { return "Roblox RccService Arbiter"; }
        }

        public override string Description
        {
            get { return "Manages a handful of processes running the RCC web service."; }
        }
    }
}
