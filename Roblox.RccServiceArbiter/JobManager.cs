using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using Roblox.Grid.Arbiter.Common;

namespace Roblox.RccServiceArbiter
{
    class JobManager
    {
        private class RccServiceProcess
        {
            internal static string RCCServiceArgs = "/Console {0} {1} ";
            internal static string RCCServiceCrashUploaderArgs = "/CrashReporter {0} {1} ";

            private Process process;
            //internal PublicDomain.Win32.Job job;
            private int port;
            private DateTime expirationTime;
            public int numThreads = 1;

            internal bool HasExited
            {
                get { return process.HasExited; }
            }
            internal DateTime ExpirationTime
            {
                get { return expirationTime; }
                set { expirationTime = value; }
            }

            internal Roblox.Grid.Rcc.RCCServiceSoap SoapInterface
            {
                get
                {
                    Roblox.Grid.Rcc.RCCServiceSoap result = new Roblox.Grid.Rcc.RCCServiceSoap();
                    result.Url = "http://localhost:" + port.ToString();
                    result.Timeout = (int)Properties.Settings.Default.Timeout.TotalMilliseconds;
                    return result;
                }
            }

            internal RccServiceProcess(int port)
            {
                this.port = port;
                this.process = new Process();
            }

            internal void Start(string exe)
            {
                process.StartInfo = new ProcessStartInfo(exe, String.Format(RCCServiceArgs, Properties.Settings.Default.RccServiceLaunch,port));
                process.Start();
            }
            internal void StartCrashReporter(string exe)
            {
                process.StartInfo = new ProcessStartInfo(exe, String.Format(RCCServiceCrashUploaderArgs, Properties.Settings.Default.RccServiceLaunch, port));
                process.Start();
            }

            internal void Close()
            {
                process.Kill();
            }
        }


        private bool multiProcessMode;
        private bool recycleProcessMode;
        private int recycleJobCount;
        private string startScript;
        private string rccServiceExe;
        private System.Threading.ReaderWriterLock exceptionLock;
        private System.Collections.Generic.Dictionary<string, int> exceptionInformation;

        private System.Threading.ReaderWriterLock activeJobsLock;

        RccServiceProcess singleProcess;
        private System.Collections.Generic.Dictionary<string, RccServiceProcess> activeJobs;
        private System.Collections.Generic.List<string> lostJobs;
        private System.Collections.Generic.List<string> closedJobs;

        private System.Threading.Semaphore newProcessRequests;
        private System.Threading.Semaphore newProcessReady;
        private System.Threading.Thread newProcessThread;
        private System.Threading.Thread expiredJobThread;
        private System.Threading.Thread monitorThread;
        private System.Threading.Thread clientSettingsThread;

        private System.Collections.Generic.LinkedList<RccServiceProcess> recycledProcesses;
        private System.Collections.Generic.LinkedList<RccServiceProcess> readyProcesses;

        public enum ThreadPoolConfig { Threads1 = 1, Threads2 = 2, Threads3 = 3, Threads4 = 4, Threads8 = 8, Threads16 = 16, Auto = 101, PlayerCount = 102, JobCount = 103 }
        private Dictionary<string, ThreadPoolConfig> threadPoolConfigList;
        ThreadPoolConfig threadPoolConfig;

        public JobManager()
        {
            System.Net.ServicePointManager.DefaultConnectionLimit = Properties.Settings.Default.MaxConnections;

            this.multiProcessMode = !Properties.Settings.Default.SingleProcess;
            this.recycleProcessMode = Properties.Settings.Default.RecycleProcesses;
            this.recycleJobCount = 4;
            this.startScript = Properties.Settings.Default.StartScript;

            exceptionLock = new System.Threading.ReaderWriterLock();
            exceptionInformation = new Dictionary<string, int>();
            activeJobsLock = new System.Threading.ReaderWriterLock();
            newProcessRequests = new System.Threading.Semaphore(0, 1000);
            newProcessReady = new System.Threading.Semaphore(0, 1000);
            singleProcess = null;

            System.Reflection.Assembly assembly = System.Reflection.Assembly.GetExecutingAssembly();
            string fullProcessPath = assembly.Location;
            rccServiceExe = System.IO.Path.GetDirectoryName(fullProcessPath) + System.IO.Path.DirectorySeparatorChar + Properties.Settings.Default.RccServicePath;

            activeJobs = new Dictionary<string, RccServiceProcess>();
            lostJobs = new List<string>();
            closedJobs = new List<string>();
            recycledProcesses = new LinkedList<RccServiceProcess>();
            readyProcesses = new LinkedList<RccServiceProcess>();

            threadPoolConfig = ThreadPoolConfig.Threads1;

            threadPoolConfigList = new Dictionary<string, ThreadPoolConfig>();
            threadPoolConfigList.Add("Threads1", ThreadPoolConfig.Threads1);
            threadPoolConfigList.Add("Threads2", ThreadPoolConfig.Threads2);
            threadPoolConfigList.Add("Threads3", ThreadPoolConfig.Threads3);
            threadPoolConfigList.Add("Threads4", ThreadPoolConfig.Threads4);
            threadPoolConfigList.Add("Threads8", ThreadPoolConfig.Threads8);
            threadPoolConfigList.Add("Threads16", ThreadPoolConfig.Threads16);
            threadPoolConfigList.Add("PlayerCount", ThreadPoolConfig.PlayerCount);
            threadPoolConfigList.Add("JobCount", ThreadPoolConfig.JobCount);
        }
        public void initialize()
        {
            Console.WriteLine("JobManager::initialize");

            newProcessThread = new System.Threading.Thread(
                delegate()
                {
                    while (true)
                    {
                        //Someone wants a job (for whatever reason)
                        newProcessRequests.WaitOne();

                        lock (recycledProcesses)
                        {
                            if(recycledProcesses.Count > 0){
                                QueueProcess(recycledProcesses.First.Value);
                                recycledProcesses.RemoveFirst();
                                continue;
                            }
                        }

                        Console.WriteLine("Processor Count" + Environment.ProcessorCount.ToString());
                        //We need to make a new process
                        QueueProcess(CreateNewProcess());
                    }
                });
            newProcessThread.IsBackground = true; 
            newProcessThread.Start();
            newProcessRequests.Release(1);

            expiredJobThread = new System.Threading.Thread(
                delegate()
                {
                    while (true)
                    {
                        System.Threading.Thread.Sleep(60000);
                        ClearExpiredJobs();
                    }
                });
            expiredJobThread.IsBackground = true;
            expiredJobThread.Start();

            clientSettingsThread = new System.Threading.Thread(
                delegate()
                {
                    while (true)
                    {
                        string configString = ClientSettings.FetchThreadPoolConfig();

                        Console.WriteLine("FetchThreadPoolConfig got " + configString);

                        if (configString != null)
                        {
                            ThreadPoolConfig config;
                            if (threadPoolConfigList.TryGetValue(configString, out config))
                            {
                                SetTheadPoolConfig(config);
                            }
                            else
                            {
                                Console.WriteLine("Invalid ThreadPoolConfig");
                            }
                        }

                        // check for new setting every 10 mins
                        System.Threading.Thread.Sleep(10 * 60000);
                    }
                }
                );

            clientSettingsThread.IsBackground = true;
            clientSettingsThread.Start();
        }
        ~JobManager()
        {
            foreach(RccServiceProcess process in readyProcesses)
            {
                CloseProcess(process);
            }
            foreach (RccServiceProcess process in activeJobs.Values)
            {
                CloseProcess(process);
            }
        }

        public string CreateThreadConfigScript(int numThreads)
        {
            // we can have 1, 2, 3, 4, 8, 16 threads
            int value = numThreads;
            if (value > 4)
            {
                if (value <= 8)
                    value = 8;
                else
                    value = 16;
            }
            return String.Format(@"settings()['Task Scheduler'].ThreadPoolConfig = Enum.ThreadPoolConfig.Threads{0};", value);
        }

        void StartPlayerCountMonitorThread()
        {
            if (monitorThread != null && monitorThread.IsAlive)
                return;

            // monitor player count and readjust task scheduler thread count
            monitorThread = new System.Threading.Thread(
                delegate()
                {
                    while (true)
                    {
                        // query player count
                        try
                        {
                            activeJobsLock.AcquireReaderLock(-1);

                            System.Collections.Generic.SynchronizedCollection<RccServiceProcess> set = new SynchronizedCollection<RccServiceProcess>();
                            foreach (RccServiceProcess process in activeJobs.Values)
                            {
                                if (!set.Contains(process))
                                {
                                    int maxCount = 0;
                                    using (var rccService = process.SoapInterface)
                                    {    
                                        Roblox.Grid.Rcc.Job[] jobs = rccService.GetAllJobs();
                                        foreach (Roblox.Grid.Rcc.Job job in jobs)
                                        {
                                            Roblox.Grid.Rcc.ScriptExecution script = new Roblox.Grid.Rcc.ScriptExecution();
                                            script.name = "GetPlayerCount";
                                            script.script = "return #game.Players:GetChildren()";
                                            Roblox.Grid.Rcc.LuaValue[] result = rccService.Execute(job.id, script);
                                            int playerCount = Int32.Parse(result[0].value);
                                            if (maxCount < playerCount)
                                                maxCount = playerCount;

                                            Console.WriteLine(String.Format("Job {0}, player count: {1}", job.id, playerCount));
                                        }
                                    }

                                    // set thread count based on number of players in game
                                    ConfigureProcess(process, "ConfigThread", CreateThreadConfigScript((maxCount / 100 * Environment.ProcessorCount) + 1));
                                    
                                    set.Add(process);
                                }
                            }
                        }
                        finally
                        {
                            activeJobsLock.ReleaseReaderLock();
                        }

                        // do this every 5 mins
                        System.Threading.Thread.Sleep(5 * 60000);

                    }
                }
                );

            monitorThread.IsBackground = true;
            monitorThread.Start();
        }

        void StopPlayerCountMonitorThread()
        {
            if (monitorThread != null)
                monitorThread.Abort();

            monitorThread = null;
        }

        public void SetTheadPoolConfig(ThreadPoolConfig config)
        {
            if (config == threadPoolConfig)
                return;

            Console.WriteLine("SetThreadPoolConfig " + config.ToString());

            if (threadPoolConfig == ThreadPoolConfig.PlayerCount)
                StopPlayerCountMonitorThread();

            switch (config)
            {
                case ThreadPoolConfig.JobCount:
                    AdjustThreadsPerProcessByJobCount();
                    break;
                case ThreadPoolConfig.PlayerCount:
                    StartPlayerCountMonitorThread();
                    break;
                case ThreadPoolConfig.Auto:
                    SetThreadsPerProcess(Environment.ProcessorCount, true);
                    break;
                default:
                    SetThreadsPerProcess((int)config, true);
                    break;

            }

            threadPoolConfig = config;
        }

        public void SetMultiProcess(bool value)
        {
            multiProcessMode = value;
        }

        public void SetRecycleProcess(bool value)
        {
            recycleProcessMode = value;
        }

        public void SetRecycleProcessCount(int count)
        {
            recycleJobCount = count;
        }

        public void SetStartScript(string value, bool broadcast)
        {
            startScript = value;
            if (broadcast)
            {
                try{
                    activeJobsLock.AcquireReaderLock(-1);

                    System.Collections.Generic.SynchronizedCollection<RccServiceProcess> set = new SynchronizedCollection<RccServiceProcess>();
                    foreach (RccServiceProcess process in activeJobs.Values)
                    {
                        if (!set.Contains(process))
                        {
                            ConfigureProcess(process, "SetupThreads", startScript);
                            set.Add(process);
                        }
                    }
                }
                finally
                { 
                    activeJobsLock.ReleaseReaderLock(); 
                }
            }
        }

        public void SetThreadsPerProcess(int value, bool broadcast)
        {
            Console.WriteLine("SetThreadPerProcess");

            if (broadcast)
            {
                try
                {
                    activeJobsLock.AcquireReaderLock(-1);

                    string script = CreateThreadConfigScript(value);

                    Console.WriteLine("Created " + script);
                    
                    System.Collections.Generic.SynchronizedCollection<RccServiceProcess> set = new SynchronizedCollection<RccServiceProcess>();
                    foreach (RccServiceProcess process in activeJobs.Values)
                    {
                        if (!set.Contains(process) && process.numThreads != value)
                        {
                            ConfigureProcess(process, "SetupThreads", script);
                            process.numThreads = value; 
                            set.Add(process);
                        }
                    }
                }
                finally
                {
                    activeJobsLock.ReleaseReaderLock();
                }
            }
        }

        public void AdjustThreadsPerProcessByJobCount()
        {
            int count = activeJobs.Count;
            int numThreads = Environment.ProcessorCount;
            if ((count > 1))
            {
                if (count % 2 == 1)
                    count++;

                numThreads = Environment.ProcessorCount / count;
                if (numThreads < 1)
                    numThreads = 1;
            }

            SetThreadsPerProcess(numThreads, true);
        }

        public string GetStats(bool clearExceptions)
        {
            ArbiterStats stats = new ArbiterStats();

            activeJobsLock.AcquireReaderLock(-1);
            try
            {
                stats.AddStat("Active Jobs", activeJobs.Count);
            }
            finally
            {
                activeJobsLock.ReleaseReaderLock();
            }

            stats.AddStat("Pending Jobs", PendingProcessCount);
            stats.AddStat("Recycled Jobs", RecycleProcessCount);
            System.Diagnostics.Process[] processlist = System.Diagnostics.Process.GetProcesses();
            {
                int RccServiceCount = 0;
                long totalWorkingSet = 0;
                long totalVirtual = 0;
                long totalPrivateMemory = 0;
                TimeSpan totalProcessorTime = new TimeSpan();
                foreach (System.Diagnostics.Process theprocess in processlist)
                {
                    if (theprocess.ProcessName == "RCCService")
                    {
                        RccServiceCount++;

                        totalWorkingSet += theprocess.WorkingSet64;
                        totalVirtual += theprocess.VirtualMemorySize64;
                        totalProcessorTime += theprocess.TotalProcessorTime;
                        totalPrivateMemory += theprocess.PrivateMemorySize64;
                    }
                }
                stats.AddStat("RccService Count", RccServiceCount);
                stats.AddStat("Total WorkingSet", totalWorkingSet);
                stats.AddStat("Total VirtualMemory", totalVirtual);
                stats.AddStat("Total PrivateMemory", totalPrivateMemory);
                stats.AddStat("Total ProcessorTime", totalProcessorTime.TotalSeconds);
            }

            stats.AddStat("Setting-MultipleProcess", multiProcessMode);
            stats.AddStat("Setting-RecycleProcess", recycleProcessMode);
            stats.AddStat("Setting-RecycleQueueSize", recycleJobCount);
            stats.AddStat("Setting-RccServicePath", Properties.Settings.Default.RccServicePath);
            
            if(clearExceptions)
                exceptionLock.AcquireWriterLock(-1);
            else
                exceptionLock.AcquireReaderLock(-1);
            try
            {
                foreach (string message in exceptionInformation.Keys)
                {
                    stats.AddStat("Exception-" + message, exceptionInformation[message]);
                }

                if (clearExceptions)
                {
                    exceptionInformation.Clear();
                }
            }
            finally
            {
                if (clearExceptions)
                    exceptionLock.ReleaseWriterLock();
                else
                    exceptionLock.ReleaseReaderLock();
            }

            return stats.ToXml();
        }

        private static int GetPort()
        {
            return TcpPort.FindNextAvailablePort(64000);
        }
        private RccServiceProcess GetSingleProcess()
        {
            lock(readyProcesses)
            {
                if(singleProcess != null && !singleProcess.HasExited)
                {
                    return singleProcess;
                }
                else{
                    singleProcess = null;
                }
            }

            while (true)
            {
                bool requestNewJob = false;
                if (PendingProcessCount == 0)
                    requestNewJob = true;

                if (requestNewJob)
                {
                    newProcessRequests.Release(1);
                    newProcessReady.WaitOne();
                }

                lock (readyProcesses)
                {
                    if (singleProcess != null && !singleProcess.HasExited)
                        return singleProcess;

                    Debug.Assert(readyProcesses.Count > 0);
                    singleProcess = readyProcesses.First.Value;
                    readyProcesses.RemoveFirst();

                    if (!singleProcess.HasExited)
                    {
                        return singleProcess;
                    }
                }
            }
        }

        private RccServiceProcess GetNewProcess(double expirationInSeconds)
        {
            if (!multiProcessMode)
            {
                return GetSingleProcess();
            }
            else
            {
                while (true)
                {
                    newProcessRequests.Release(1);
                    newProcessReady.WaitOne();
                    
                    RccServiceProcess result;
                    lock (readyProcesses)
                    {
                        Debug.Assert(readyProcesses.Count > 0);
                        result = readyProcesses.First.Value;
                        readyProcesses.RemoveFirst();
                    }

                    if (result.HasExited)
                    {
                        //The pending job died, so try again
                        continue;
                    }

                    result.ExpirationTime = DateTime.Now.AddSeconds(expirationInSeconds);
                    return result;
                }
            }
        }
        private void RecycleProcess(RccServiceProcess process)
        {
            lock (recycledProcesses)
            {
                recycledProcesses.AddLast(process);
            }
        }
        private void QueueProcess(RccServiceProcess process)
        {
            lock (readyProcesses)
            {
                readyProcesses.AddLast(process);
            }
            newProcessReady.Release(1);
        }
        private int RecycleProcessCount
        {
            get
            {
                lock (recycledProcesses)
                {
                    return recycledProcesses.Count;
                }
            }
        }
        private int PendingProcessCount
        {
            get
            {
                lock (readyProcesses)
                {
                    return readyProcesses.Count;
                }
            }
        }
        private string RecordException(string message)
        {
            try
            {
                exceptionLock.AcquireWriterLock(-1);
                if (!exceptionInformation.ContainsKey(message))
                {
                    exceptionInformation[message] = 0;
                }
                exceptionInformation[message]++;
            }
            finally
            {
                exceptionLock.ReleaseWriterLock();
            }
            return message;
        }

        private bool ConfigureProcess(RccServiceProcess process, string scriptName, string script)
        {
            try
            {
                using (var rccService = process.SoapInterface)
                {
                    Roblox.Grid.Rcc.Job configJob = new Roblox.Grid.Rcc.Job();
                    configJob.id = "Config";
                    configJob.category = 0;
                    configJob.cores = 1;
                    configJob.expirationInSeconds = 10;
                    Roblox.Grid.Rcc.ScriptExecution scriptExcution = new Roblox.Grid.Rcc.ScriptExecution();
                    scriptExcution.name = scriptName;
                    scriptExcution.script = script;
                    scriptExcution.arguments = null;

                    Console.WriteLine("ConfigureProcess, script: " + script);

                    rccService.OpenJob(configJob, scriptExcution);
                    rccService.CloseJob(configJob.id);
                    return true;
                }
            }
            catch (Exception)
            {
                RecordException("SetupScriptFailed");
                return false;
            }
        }
        private RccServiceProcess CreateCrashUploaderProcess()
        {
            RccServiceProcess result = new RccServiceProcess(GetPort());
            result.StartCrashReporter(rccServiceExe);
            return result;
        }

        private RccServiceProcess CreateNewProcess()
        {
            Console.WriteLine("CreateNewProcess");
            RccServiceProcess result = new RccServiceProcess(GetPort());
            result.Start(rccServiceExe);

            if (ConfigureProcess(result, "SetupThreads", startScript))
            {
                return result;
            }
            else
            {
                //return it non-configured
                return result;
            }

        }
        private void ClearExpiredJobs()
        {
            List<string> jobsToRemove = new List<string>();
            {
                activeJobsLock.AcquireReaderLock(-1);
                try
                {
                    DateTime currentTime = DateTime.Now;
                    foreach (string jobId in activeJobs.Keys)
                    {
                        if (activeJobs[jobId].HasExited || activeJobs[jobId].ExpirationTime < currentTime)
                        {
                            jobsToRemove.Add(jobId);
                        }
                    }
                }
                finally
                {
                    activeJobsLock.ReleaseReaderLock();
                }
            }
            foreach (string jobId in jobsToRemove)
            {
                RecordException("JobLeaseExpired");
                CloseJob(jobId);
            }
        }
        public void RenewLease(string jobId, double expirationInSeconds)
        {
            activeJobsLock.AcquireWriterLock(-1);
            try
            {
                if (activeJobs.ContainsKey(jobId))
                {
                    if (!activeJobs[jobId].HasExited)
                    {
                        DateTime newTime = DateTime.Now.AddSeconds(expirationInSeconds);
                        if (activeJobs[jobId].ExpirationTime < newTime)
                        {
                            activeJobs[jobId].ExpirationTime = newTime;
                        }
                    }
                }
            }
            finally
            {
                activeJobsLock.ReleaseWriterLock();
            }
        }
        public Roblox.Grid.Rcc.RCCServiceSoap NewJob(string jobId, double expirationInSeconds, string task)
        {
            RccServiceProcess process = GetNewProcess(expirationInSeconds);

            activeJobsLock.AcquireWriterLock(-1);
            try
            {
                //Launch a process on a new port
                activeJobs[jobId] = process;
            }
            finally
            {
                activeJobsLock.ReleaseWriterLock();
            }

            Console.WriteLine("NewJob, total: " + activeJobs.Count.ToString());

            if (threadPoolConfig == ThreadPoolConfig.JobCount)
                AdjustThreadsPerProcessByJobCount();

            return GetJob(jobId, task, "");
        }
        public Roblox.Grid.Rcc.RCCServiceSoap GetJob(string jobId, string task, string extraInfo)
        {
            string message ="UnknownGetJobError";
            activeJobsLock.AcquireReaderLock(-1);
            try
            {
                if (activeJobs.ContainsKey(jobId))
                {
                    if (!activeJobs[jobId].HasExited)
                    {
                        return activeJobs[jobId].SoapInterface;
                    }
                    else
                    {
                        message = RecordException("JobDied");
                        //A crash occurred, we need to spawn a process to deal with this.
                        CreateCrashUploaderProcess();
                        
                        System.Threading.LockCookie cookie = activeJobsLock.UpgradeToWriterLock(-1);
                        try
                        {

                            if (activeJobs.ContainsKey(jobId))
                            {
                                activeJobs.Remove(jobId);
                            }
                        }
                        finally
                        {
                            activeJobsLock.DowngradeFromWriterLock(ref cookie);
                        }
                    }
                }
                else
                {
                    if (extraInfo != "")
                    {
                        message = RecordException("JobLost('" + extraInfo + "')");
                    }
                    if (!lostJobs.Contains(jobId))
                    {
                        lostJobs.Add(jobId);
                        if (lostJobs.Count > 100)
                        {
                            lostJobs.RemoveAt(0);
                        }

                        if (closedJobs.Contains(jobId))
                        {
                            message = RecordException("JobLost-Unique" + task + "AfterClose");
                        }
                        else
                        {
                            message = RecordException("JobLost-Unique" + task);
                        }
                    }
                    else
                    {
                        if (closedJobs.Contains(jobId))
                        {
                            message = RecordException("JobLost-" + task + "AfterClose");
                        }
                        else
                        {
                            message = RecordException("JobLost-" + task);
                        }
                    }
                }
            }
            finally
            {
                activeJobsLock.ReleaseReaderLock();
            }
            throw new Exception(message);
        }
        public Roblox.Grid.Rcc.RCCServiceSoap AnyJob(double expirationInSeconds)
        {
            activeJobsLock.AcquireReaderLock(-1);
            try
            {
                foreach (RccServiceProcess p in activeJobs.Values)
                {
                    if (!p.HasExited)
                    {
                        return p.SoapInterface;
                    }
                }
            }
            finally
            {
                activeJobsLock.ReleaseReaderLock();
            }

            //We have no jobs, grab a new one
            RccServiceProcess newProcess = GetNewProcess(expirationInSeconds);

            //Queue it back up since we won't be using it for long
            RecycleProcess(newProcess);

            return newProcess.SoapInterface;
        }
        private void CloseProcess(RccServiceProcess process)
        {
            if (!process.HasExited)
            {
                process.Close();
            }
        }

        public void CloseJob(string jobId)
        {
            activeJobsLock.AcquireWriterLock(-1);
            try
            {
                if (activeJobs.ContainsKey(jobId))
                {
                    
                    if (!multiProcessMode)
                    {
                        if (activeJobs[jobId] != singleProcess)
                        {
                            //Our process is a hanger on, not the core singleProcess
                            //go ahead and close it down
                            CloseProcess(activeJobs[jobId]);
                        }
                    }
                    else if (recycleProcessMode)
                    {
                        //Requeue it for use later, unless its exited already
                        if (!activeJobs[jobId].HasExited)
                        {
                            if (RecycleProcessCount < recycleJobCount)
                            {
                                RecycleProcess(activeJobs[jobId]);
                            }
                            else
                            {
                                //Close it, we probably have too many jobs already
                                CloseProcess(activeJobs[jobId]);
                            }
                        }
                    }
                    else
                    {
                        //Close it, we're creating a new process for every job
                        CloseProcess(activeJobs[jobId]);
                    }

                    //Kick it out of the running job list
                    activeJobs.Remove(jobId);

                    closedJobs.Add(jobId);
                    if (closedJobs.Count > 100)
                    {
                        closedJobs.RemoveAt(0);
                    }

                    Console.WriteLine("CloseJob, total: " + activeJobs.Count.ToString());

                    if (threadPoolConfig == ThreadPoolConfig.JobCount)
                        AdjustThreadsPerProcessByJobCount();

                }
                else
                {
                    RecordException("JobLost-Close");
                    System.Console.Out.WriteLine("Job[" + jobId + "] was lost");
                }
            }
            finally
            {
                activeJobsLock.ReleaseWriterLock();
            }
        }

        public delegate void JobDelegate(Roblox.Grid.Rcc.RCCServiceSoap soapProcess);
        public void DispatchRequest(JobDelegate jobDelegate)
        {
            activeJobsLock.AcquireReaderLock(-1);
            try
            {
                foreach (RccServiceProcess process in activeJobs.Values)
                {
                    if (!process.HasExited)
                    {
                        using (var rccSoap = process.SoapInterface)
                        {
                            jobDelegate(rccSoap);
                        }
                    }
                }
            }
            finally
            {
                activeJobsLock.ReleaseReaderLock();
            }
        }
    }
}