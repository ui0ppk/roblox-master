using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ServiceModel;
using System.IO;
using System.Diagnostics;


// generated client :
// Svcutil.exe /n:"http://www.roblox.com/Roblox.Test,Roblox.Test.Client" http://localhost:8000/Roblox.Test/RunService/mex /l:c# /out:generatedClient.cs
namespace Roblox.Test
{
    // Define a service contract.
    [ServiceContract(Namespace = "http://www.roblox.com/Roblox.Test")]
    public interface IRun
    {
        [OperationContract]
        string Ping();

        [OperationContract]
        string Execute(string binroot, string testname);

        // ask for a string to use as a header for these tests.
        [OperationContract]
        string GetHeader(string binroot, string testname);

        [OperationContract]
        string SpawnProcess(string filename, string args, out int pid);

        [OperationContract]
        string KillProcess(int pid);

        [OperationContract]
        string Cleanup(); // kill all spawned processes, abort running tests.
    }


    public class RunService : IRun
    {

        #region IRun Members

        public string Execute(string binroot, string testname)
        {
            string result = "OK";
            StringWriter sw = new StringWriter();
            try
            {
                Console.Write(DateTime.Now + " Executing " + testname + "...");

                if (testname == "Ping")
                {
                    sw.WriteLine(DateTime.Now.ToString() + "\t" + Environment.MachineName + "\t" + "Pong!" + "\t");
                }
                else
                {
                    Program.ShimExecutorDelegate runTest = delegate(IRemoteTestShim shim, string AssemblyName, string DeploymentDir, TextWriter tw)
                    {
                        tw.Write(DateTime.Now.ToString() + "\t" + Environment.MachineName + "\t" + testname + "\t");
                        shim.RunTest(AssemblyName, testname, DeploymentDir, tw);
                    };

                    Program.LoadAssemblyAndExecute(binroot, sw, runTest);
                }
                sw.Flush();
                result = sw.ToString();
            }
            catch(Exception e)
            {
                sw.Flush();
                result= sw.ToString() + Program.ExceptionToMessage(e) + "\n";
            }
            Console.WriteLine(result);
            return result;
        }

        public string GetHeader(string binroot, string testname)
        {
            StringWriter sw = new StringWriter();
            try
            {
                Program.ShimExecutorDelegate getHeader = delegate(IRemoteTestShim shim, string AssemblyName, string DeploymentDir, TextWriter tw)
                {
                    sw.WriteLine(shim.GetTestHeader(AssemblyName, testname));
                };

                Program.LoadAssemblyAndExecute(binroot, sw, getHeader);
            }
            catch (Exception e)
            {
                sw.WriteLine("Generating header: " + Program.ExceptionToMessage(e));
            }
            sw.Flush();

            return sw.ToString();
        }

        public string Ping()
        {
            string result = "OK";
            StringWriter sw = new StringWriter();
            try
            {
                Console.Write(DateTime.Now + " Executing Ping...");

                sw.Write(DateTime.Now.ToString() + "\t" + Environment.MachineName + "\t" + "Pong!" + "\t");
                sw.Flush();
                result = sw.ToString();
            }
            catch (Exception e)
            {
                sw.Flush();
                result = sw.ToString() + Program.ExceptionToMessage(e);
            }
            Console.WriteLine(result);
            return result;
        }

        public string SpawnProcess(string filename, string args, out int pid)
        {
            string result = "OK";
            pid = 0;
            StringWriter sw = new StringWriter();
            try
            {
                Console.Write(DateTime.Now + " Spawning " + filename + " " + args + "...");
                sw.Write(DateTime.Now.ToString() + "\t" + Environment.MachineName + "\t" + "Spawning " + filename + " " + args + "..." + "\t");

                Process proc = new Process();
                proc.StartInfo.FileName = filename;
                proc.StartInfo.Arguments = args;

                proc.Start();

                pid = proc.Id;

                Program.RegisterProcess(proc);


                sw.Flush();
                result = sw.ToString();
            }
            catch (Exception e)
            {
                sw.Flush();
                result = sw.ToString() + Program.ExceptionToMessage(e);
            }
            Console.WriteLine(result);
            return result;
        }

        public string KillProcess(int pid)
        {
            string result = "OK";
            pid = 0;
            StringWriter sw = new StringWriter();
            try
            {
                Console.Write(DateTime.Now + " Killing " + pid + "...");
                sw.Write(DateTime.Now.ToString() + "\t" + Environment.MachineName + "\t" + "Killing #" + pid + "..." + "\t");

                Process proc = Process.GetProcessById(pid);

                proc.Kill();

                sw.Flush();
                result = sw.ToString();
            }
            catch (Exception e)
            {
                sw.Flush();
                result = sw.ToString() + Program.ExceptionToMessage(e);
            }
            Console.WriteLine(result);
            return result;
        }

        public string Cleanup()
        {
            string result = "OK";
            StringWriter sw = new StringWriter();
            try
            {
                Console.Write(DateTime.Now + " Cleanup...");
                sw.Write(DateTime.Now.ToString() + "\t" + Environment.MachineName + "\t" + "Cleanup..." + "\t");

                Program.Cleanup();

                sw.Flush();
                result = sw.ToString();
            }
            catch (Exception e)
            {
                sw.Flush();
                result = sw.ToString() + Program.ExceptionToMessage(e);
            }
            Console.WriteLine(result);
            return result;
        }

        #endregion
    }

    public static class Service
    {
        public static void Listen()
        {
            using (ServiceHost serviceHost = new ServiceHost(typeof(RunService)))
            {
                // Open the ServiceHost to create listeners and start listening for messages.
                serviceHost.Open();

                // The service can now be accessed.
                Console.WriteLine("The service is listening.");
                Console.WriteLine("Press <ENTER> to terminate service.");
                Console.WriteLine();
                Console.ReadLine();
            }
        }
    }

}
