using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.Remoting.Lifetime;
using System.Runtime.Remoting;
using System.Windows.Forms;
using System.Data.SqlClient;
using System.Net.Sockets;
using System.Threading;

namespace Roblox.Test
{
    public struct ExecStats
    {
        public int SuccessCount;
        public int FailureCount;
    };

    public class Program
    {
        private const string outputDir = "S:\\Automation\\Results\\";
        private const string outFileFormat = "{0}{1}_{2}.txt";
        private const string testCasesDll = "Roblox.Test.TestCases.dll";
        private const string testCasesAssemblyName = "Roblox.Test.TestCases, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null";
        private const string remoteReflectorAssemblyName = "Roblox.Test.RemoteTestShim, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null";

        public static string ExceptionToMessage(Exception e)
        {
            string exceptionmessage = e.Message;
            Exception inner = e;
            while ((inner = inner.InnerException) != null)
            {
                exceptionmessage += " --> " + inner.Message;
            }
            return exceptionmessage;
        }
 
        public static void Local(string[] args)
        {
            TextWriter sw;
            if (args.Length <= 2)
            {
                sw = new StringWriter();
            }
            else
            {
                sw = new StreamWriter(outputDir + args[2], true /*append*/);
            }

            string result = "OK";
            string testname = args[1];


            string binroot = null;

            PrintHeaderForTest(binroot, sw, testname);

            try
            {
                Console.WriteLine(DateTime.Now + " Executing " + testname + "...");

                Program.ShimExecutorDelegate runTest = delegate(IRemoteTestShim shim, string AssemblyName, string DeploymentDir, TextWriter tw)
                {
                    tw.Write(DateTime.Now.ToString() + "\t" + Environment.MachineName + "\t" + testname + "\t");

                    shim.RunTest(AssemblyName, testname, DeploymentDir, tw);
                };

                Program.LoadAssemblyAndExecute(binroot, sw, runTest);

                sw.Flush();
                result = sw.ToString();
            }
            catch (Exception e)
            {
                sw.Flush();
                result = sw.ToString() + ExceptionToMessage(e) + sw.NewLine;
            }
            Console.WriteLine(result);
 
            sw.Close();
        }

        public delegate void ShimExecutorDelegate(IRemoteTestShim shim, string AssemblyName, string DeploymentDir, TextWriter tw);


        public static string CalcBinRoot(string optbinroot)
        {
            if (string.IsNullOrEmpty(optbinroot))
            {
                if (Assembly.GetEntryAssembly() != null)
                {
                    return Path.GetDirectoryName(Assembly.GetEntryAssembly().Location);
                }
                else
                {
                    return Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                }
            }
            else
            {
                return optbinroot;
            }
        }

        public static void LoadAssemblyAndExecute(string binroot, TextWriter sw, ShimExecutorDelegate shimExec)
        {
            string execdir =  CalcBinRoot(binroot);;

            string dllfullpath = Path.Combine(execdir, testCasesDll);

            if (!File.Exists(dllfullpath))
            {
                throw new Exception(String.Format("Roblox.Test.TestCases.dll not found at {0}. Did you forget to deploy the tests with Roblox.Test.exe?", dllfullpath));
            }


            // Create an app domain for our test dll so that we can unload it
            AppDomain testCaseDomain = AppDomain.CreateDomain("TestCaseDomain");
 
            // expects the consturctor to take two arguments: (string DeploymentDir, TextWriter result)
            IRemoteTestShim remoteReflector = (IRemoteTestShim)testCaseDomain.CreateInstanceAndUnwrap(remoteReflectorAssemblyName, "Roblox.Test.RemoteTestShimImpl");

            try
            {
                shimExec(remoteReflector, testCasesAssemblyName, execdir, sw); 
            }
            finally
            {
                remoteReflector = null;
                // we are done with the assembly, unload it.
                AppDomain.Unload(testCaseDomain);

                GC.Collect();
            }
        }

        public delegate string AsyncRunDelegate(string address);

        public static void PrintHeaderForTest(string binroot, TextWriter tw, string testname)
        {
            // print the header:
            tw.Write("Time\tMachine\tTestName\t");

            // get the header from the local test.
            try
            {
                Program.ShimExecutorDelegate getHeader = delegate(IRemoteTestShim shim, string AssemblyName, string DeploymentDir, TextWriter unsused)
                {
                    tw.WriteLine(shim.GetTestHeader(AssemblyName, testname));
                };

                Program.LoadAssemblyAndExecute(binroot, tw, getHeader);
            }
            catch (Exception e)
            {
             //   tw.WriteLine("Generating header: " + Program.ExceptionToMessage(e));
                // use default. todo: fix this loading thingy on tfs build machine.
                tw.WriteLine("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus");
            }
    
        }

        public static void RemoteMany(TextWriter tw, string[] machinenames, AsyncRunDelegate runone)
        {
            string addressFormat = "http://{0}:8000/Roblox.Test/RunService";

            IAsyncResult[] results = new IAsyncResult[machinenames.Length];


            for (int i = 0; i < results.Length; ++i)
            {
                Console.WriteLine("Launched " + machinenames[i] + "...");
                results[i] = runone.BeginInvoke(string.Format(addressFormat, machinenames[i]), null, null);
            }

            for (int i = 0; i < results.Length; ++i)
            {
                Console.WriteLine("Waiting for: " + machinenames[i] + "...");
                results[i].AsyncWaitHandle.WaitOne();
            }

            for (int i = 0; i < results.Length; ++i)
            {
                tw.Write(runone.EndInvoke(results[i]));
            }

        }

        private static void RemoteCmd(string[] args)
        {
            string testnamearg = args[1];
            string[] machinenames = new string[args.Length - 2];
            for (int i = 0; i < machinenames.Length; ++i)
            {
                machinenames[i] = args[i + 2];
            }

            TextWriter tw = new StringWriter();


            string binroot = null;

            Remote(binroot, tw, testnamearg, machinenames);

            tw.Flush();
            string now = DateTime.Now.ToString("s").Replace(':', '.');
            string outfile = string.Format(outFileFormat, outputDir, testnamearg, now);

            Console.WriteLine("Results (also written to {0}):", outfile);
            Console.Write(tw.ToString());

            using (StreamWriter sw = new StreamWriter(outfile))
            {
                sw.Write(tw.ToString());
                sw.Close();
            }
        }

        public static ExecStats Remote(string binroot, TextWriter tw, string testnamearg, string[] machinenames)
        {
            int s = 0;
            int f = 0;
            PrintHeaderForTest(null, tw, testnamearg);

            AsyncRunDelegate runone = delegate(string address)
            {
                string result;
                try
                {

                    Roblox.Test.Client.RunClient runclient = new Roblox.Test.Client.RunClient("BasicHttpBinding_IRun", address);

                    result = runclient.Execute(binroot, testnamearg) + tw.NewLine;
                    s++;
                }
                catch (Exception e)
                {
                    result = DateTime.Now.ToString() + "\t" + address + "\t" + testnamearg + ":" + "\t" + ExceptionToMessage(e) + tw.NewLine;
                    f++;
                }

                Console.WriteLine(result);
                return result;
            };

            RemoteMany(tw, machinenames, runone);

            return new ExecStats { FailureCount = f, SuccessCount = s };
        }

        public static void Spawn(string[] args)
        {
            string[] exeargs = args[1].Split(new char[]{'(',')'});
            if (exeargs.Length != 3)
            {
                PrintUsage();
                return;
            }
            string procname = exeargs[0];
            string procargs = exeargs[1];
            string[] machinenames = new string[args.Length - 2];
            for (int i = 0; i < machinenames.Length; ++i)
            {
                machinenames[i] = args[i + 2];
            }

            TextWriter tw = new StringWriter();

            AsyncRunDelegate runone = delegate(string address)
            {
                int pid;
                string result;
                try
                {

                    Roblox.Test.Client.RunClient runclient = new Roblox.Test.Client.RunClient("BasicHttpBinding_IRun", address);

                    result = runclient.SpawnProcess(out pid, procname, procargs) + tw.NewLine;
                }
                catch (Exception e)
                {
                    result = DateTime.Now.ToString() + "\t" + address + "\t" + procname + ":" + "\t" + ExceptionToMessage(e) + tw.NewLine;
                }

                Console.Write(result);
                return result;
            };

            RemoteMany(tw, machinenames, runone);

            tw.Flush();
            Console.Write(tw.ToString());
        }

        private static void PingCmd(string[] args)
        {
            string[] machinenames = new string[args.Length - 1];
            for (int i = 0; i < machinenames.Length; ++i)
            {
                machinenames[i] = args[i + 1];
            }

            TextWriter tw = new StringWriter();

            var r = Ping(tw, machinenames);

            tw.Flush();
            Console.Write(tw.ToString());
        }

        public static ExecStats Ping(TextWriter tw, string[] machinenames)
        {
            int s = 0;
            int f = 0;
            AsyncRunDelegate runone = delegate(string address)
            {
                string result;
                try
                {

                    Roblox.Test.Client.RunClient runclient = new Roblox.Test.Client.RunClient("BasicHttpBinding_IRun", address);

                    result = runclient.Ping() + tw.NewLine;
                    s++;
                }
                catch (Exception e)
                {
                    result = DateTime.Now.ToString() + "\t" + address + "\t" + "ping" + ":" + "\t" + ExceptionToMessage(e) + tw.NewLine;
                    f++;
                }

                Console.Write(result);
                return result;
            };

            RemoteMany(tw, machinenames, runone);

            return new ExecStats { FailureCount = f, SuccessCount = s };
        }

        public static void RemoteCleanup(string[] args)
        {
            string[] machinenames = new string[args.Length - 1];
            for (int i = 0; i < machinenames.Length; ++i)
            {
                machinenames[i] = args[i + 1];
            }

            TextWriter tw = new StringWriter();

            AsyncRunDelegate runone = delegate(string address)
            {
                string result;
                try
                {

                    Roblox.Test.Client.RunClient runclient = new Roblox.Test.Client.RunClient("BasicHttpBinding_IRun", address);

                    result = runclient.Cleanup() + tw.NewLine;
                }
                catch (Exception e)
                {
                    result = DateTime.Now.ToString() + "\t" + address + "\t" + "cleanup" + ":" + "\t" + ExceptionToMessage(e) + tw.NewLine;
                }

                Console.WriteLine(result);
                return result;
            };

            RemoteMany(tw, machinenames, runone);

            tw.Flush();
            Console.Write(tw.ToString());
        }

        public static bool Connect(string[] args)
        {
            // register ourself with the test hook

            Console.WriteLine("Connecting to {0} on port {1}", args[1], args[2]);

            while (true)
            {
                System.Net.Sockets.TcpClient tcp = new TcpClient();
                try
                {
                    tcp.Connect(args[1], int.Parse(args[2]));
                    tcp.Close();
                    return true;
                }
                catch (System.Net.Sockets.SocketException e)
                {
                    Console.WriteLine(e.Message);
                    Thread.Sleep(1000);
                }
            }
        }

        static void PrintUsage()
        {
            Console.WriteLine("Usage:");
            Console.WriteLine("local:");
            Console.WriteLine("\tRoblox.Test.exe local [testname] | listen");
            Console.WriteLine("remote:");
            Console.WriteLine("\tRoblox.Test.exe connect [ip addresss] [port]");
            Console.WriteLine("\tRoblox.Test.exe remote [testname]      [machine names...]");
            Console.WriteLine("\tRoblox.Test.exe ping                   [machine names...]");
            Console.WriteLine("\tRoblox.Test.exe spawn [exepath(args)]  [machine names...]");
            Console.WriteLine("\tRoblox.Test.exe cleanup                [machine names...]");
        }

        static void Main(string[] args)
        {

            Console.WriteLine("Version: {0}", "2.0");

            if (args.Length == 0)
            {
                PrintUsage();
                Service.Listen();
                return;
            }

            if (args[0] == "local")
            {
                if (args.Length < 2)
                {
                    PrintUsage();
                    return;
                }
                Local(args);
                return;
            }
            else if (args[0] == "remote")
            {
                if (args.Length < 2)
                {
                    PrintUsage();
                    return;
                }
                RemoteCmd(args);
                return;
            }
            else if (args[0] == "ping")
            {
                if (args.Length < 1)
                {
                    PrintUsage();
                    return;
                }
                PingCmd(args);
                return;
            }
            else if (args[0] == "spawn")
            {
                if (args.Length < 2)
                {
                    PrintUsage();
                    return;
                }
                Spawn(args);
                return;
            }
            else if (args[0] == "cleanup")
            {
                if (args.Length < 1)
                {
                    PrintUsage();
                    return;
                }
                RemoteCleanup(args);
                return;
            }
            else if (args[0] == "listen")
            {
                Service.Listen();
                return;
            }
            else if (args[0] == "publish")
            {
                PublishResultsToDBCmd(args);
                return;
            }
            else if (args[0] == "connect")
            {
                if (args.Length < 3)
                {
                    PrintUsage();
                    return;
                }

                if (Connect(args))
                    Service.Listen();

                return;
            }
            else
            {
                PrintUsage();
            }

        }

        static Dictionary<int, Process> spawnedProcesses = new Dictionary<int, Process>();

        public static void RegisterProcess(Process proc)
        {
            spawnedProcesses[proc.Id] = proc;
        }

        public static void Cleanup()
        {
            foreach (KeyValuePair<int, Process> kv in spawnedProcesses)
            {
                if (!kv.Value.HasExited)
                {
                    kv.Value.Kill();
                }
            }
            spawnedProcesses.Clear();
        }


        private static void PublishResultsToDBCmd(string[] args)
        {
            var sr = new StreamReader(args[1]);
            PublishResultsToDB(sr);
        }

        public static void PublishResultsToDB(TextReader tr)
        {
            var schemaline = tr.ReadLine();
            var rgschema = schemaline.Split(new char[] { '\t' });
            var columnnames = schemaline.Replace('\t', ',');

            var csb = new SqlConnectionStringBuilder();
            csb.InitialCatalog = "LabPerf";
            csb.DataSource = "BUILD0\\SQLEXPRESS";
            csb.IntegratedSecurity = true;

            var cn = new SqlConnection(csb.ConnectionString);

            cn.Open();

            using (var transaction = cn.BeginTransaction())
            {
                string line = null;
                while ((line = tr.ReadLine()) != null)
                {
                    var rgvalues = line.Split(new char[] { '\t' });
                    var values = "'" + line.Replace("\t", "','") + "'";

                    values = values.Replace("'NaN'", "NULL");


                    using (var command = cn.CreateCommand())
                    {
                        command.Transaction = transaction;

                        if (rgvalues.Length == rgschema.Length) // non-exceptional input. just feed in to db.
                        {
                            command.CommandText = "INSERT INTO ClientPerf(" + columnnames + ") VALUES (" + values + ")";
                            Console.WriteLine("Execyting Sql Command:" + command.CommandText);
                            command.ExecuteNonQuery();
                        }
                        else if (rgvalues.Length >= 3 && rgschema[0] == "Time" && rgschema[1] == "Machine" && rgschema[2] == "TestName") // exception likely occured. need some processing.
                        {
                            var sb = new StringBuilder();

                            sb.Append("'");
                            sb.Append(rgvalues[0]); sb.Append("','");
                            sb.Append(rgvalues[1]); sb.Append("','");
                            sb.Append(rgvalues[2]); sb.Append("','");
                            for (int i = 3; i < rgvalues.Length; ++i)
                            {
                                var scrubbedvalue = rgvalues[i].Replace("'", "''");
                                sb.Append(scrubbedvalue);
                                if (i != rgvalues.Length - 1)
                                {
                                    sb.Append("\t");
                                }
                            }
                            sb.Append("'");

                            command.CommandText = "INSERT INTO ClientPerf(Time, Machine, TestName, Exception) VALUES (" + sb.ToString() + ")";
                            Console.WriteLine("Execyting Sql Command:" + command.CommandText);
                            command.ExecuteNonQuery();
                        }
                        else if(!string.IsNullOrEmpty(line))
                        {
                            throw new Exception(string.Format("Unknown line format : {0}", line));
                        }
                       
                    }
                }

                transaction.Commit();
            }

            cn.Close();
        }
    }
}
