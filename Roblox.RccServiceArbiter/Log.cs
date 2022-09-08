using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
namespace Roblox.RccServiceArbiter
{
    internal static class Log
    {
        static StreamWriter log;
        static Log()
        {
            if (Properties.Settings.Default.LogTransactions)
            {
                log = new StreamWriter(String.Format("C:\\arbiter-log{0}.txt", Guid.NewGuid().ToString()));
            }
        }

        static internal void Event(string message)
        {
            if (log != null)
            {
                lock (log)
                {
                    log.WriteLine(String.Format("{0} - {1}", DateTime.Now.ToString(), message));
                    log.Flush();
                }
            }
        }

    }
}
