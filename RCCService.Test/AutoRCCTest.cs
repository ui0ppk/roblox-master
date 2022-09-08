using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Roblox.Grid.Rcc;
using Roblox.Grid;
using System.Diagnostics;
using System.IO;

namespace RCCService.Test
{
    /// <summary>
    /// Summary description for AutoRCCTest
    /// </summary>
    [TestClass]
    public class AutoRCCTest
    {

        public AutoRCCTest()
        {
            //
            // TODO: Add constructor logic here
            //
        }

        private TestContext testContextInstance;
        internal static bool autoBuild = false;

        /// <summary>
        ///Gets or sets the test context which provides
        ///information about and functionality for the current test run.
        ///</summary>
        public TestContext TestContext
        {
            get
            {
                return testContextInstance;
            }
            set
            {
                testContextInstance = value;
            }
        }
        #region Additional test attributes
        //
        // You can use the following additional attributes as you write your tests:
        //

        static Uri serviceUri;
        //        [ClassInitialize()]
        //        public static void MyClassInitialize(TestContext testContext) 
        //        {
        //            servicePort = RCCLauncher.StartRCCService();
        //        }
        //
        [ClassCleanup()]
        public static void MyClassCleanup()
        {
            RCCLauncher.StopAllRCCServices();
        }

        [TestInitialize()]
        public void MyTestInitialize()
        {

            const string buildTarget = "Debug";
            const string service_LOCAL = @"..\..\..\RCCService\bin\" + buildTarget + @"\RCCService.exe";
            const string service_AUTO = @"..\..\..\Binaries\Mixed Platforms\" + buildTarget + @"\RCCService.exe";
            const string args_LOCAL = "/console /content:..\\..\\..\\content\\ {0} ";
            const string args_AUTO = "/console /content:F:\\share\\content\\ {0} ";
            // directories where RBXL files are expected
            //const string dir_LOCAL = @"..\..\TestRBXLFiles";
            const string dir_LOCAL = @"..\..\..\RCCService.Test\TestRBXLFiles";
            //const string dir_AUTO = @"..\..\..\Sources\Trunk\Client\RCCService.Test\TestRBXLFiles";
            const string dir_AUTO = @"F:\ClientBuildSource\Trunk\Client\RCCService.Test\TestRBXLFiles";

            if (Directory.Exists(dir_LOCAL))
            {
                autoBuild = false;
                RCCLauncher.RCCServicePath = service_LOCAL;
                RCCLauncher.RCCServiceArgs = args_LOCAL;
                RCCLauncher.RemoteRCCServicePath = service_LOCAL;
                RCCLauncher.RemoteRCCServiceArgs = args_LOCAL;
            }
            else if (Directory.Exists(dir_AUTO))
            {
                autoBuild = true;
                RCCLauncher.RCCServicePath = service_AUTO;
                RCCLauncher.RCCServiceArgs = args_AUTO;
                RCCLauncher.RemoteRCCServicePath = service_AUTO;
                RCCLauncher.RemoteRCCServiceArgs = args_AUTO;
            }
            else
            {
                Assert.Fail("RBXL Directory was not copied to local machine in paths " + dir_LOCAL + " or " + dir_AUTO);
            }


            // if ( build dirctory exists ) {
            //      RCCLauncher.RCCServicePath = BUILDMACHINE PATH
            //      PlaceDir = BUILDMACHINE PATH
            // else {
            //      do nothing -- all defaults are for local
            // }



            serviceUri = RCCLauncher.FindOrStartRCCService();
        }
        //
        // Use TestCleanup to run code after each test has run
        // [TestCleanup()]
        // public void MyTestCleanup() { }
        //
        #endregion

        /*
         *  CreateRBXLDir moves the .RBXL test files to be run by RCCService
         *  from the repository directory which contains these files to a specified path
         *  on the local build machine.
         *  @param placedir is the string of the repository directory
         *  @param dumpdir is the string of the local machine directory to create and copy files to
         */
        private void CreateRBXLDir(string placedir, string dumpdir)
        {
            // if the repository directory containing RBXL files cannot be found, quit
            if (!Directory.Exists(placedir))
                Assert.Fail("Cannot find test RBXL files in designated repository " + placedir);
            // otherwise, delete the existing directory and move repository RBXL files to local folder:
            if (Directory.Exists(dumpdir))
            {
                DirectoryInfo old_dir = new DirectoryInfo(dumpdir);

                FileInfo[] OldFiles = old_dir.GetFiles();
                foreach (FileInfo aFile in OldFiles)
                {
                    try
                    {
                        aFile.IsReadOnly = false;
                        //File.SetAttributes(aFile, FileAttributes.Normal);
                        aFile.Delete();
                    }
                    catch (System.Exception)
                    {
                        Assert.Fail("Cannot delete file");
                    }
                }
                Directory.Delete(dumpdir);
            }

            Directory.CreateDirectory(dumpdir);

            DirectoryInfo source_dir = new DirectoryInfo(placedir);
            DirectoryInfo dest_dir = new DirectoryInfo(dumpdir);

            FileInfo[] SourceFiles = source_dir.GetFiles();
            if (SourceFiles.Length > 0)
            {
                foreach (FileInfo aFile in SourceFiles)
                    aFile.CopyTo(dumpdir + aFile.Name);
            }
        }
        string run_arg = "";
        public string RunArg { set { run_arg = value; } }
        [TestMethod]
        public void TestRunRBXL()
        {
            int PASS = 1;
            // local RBXL directory:
            //string placedir = @"C:\\Documents and Settings\\ROBLOX\\Desktop\\RBXL\\RBX-Test.rbxl";
            // local path for Dan's machine:
            //string placedir = @"C:\\Documents and Settings\\ROBLOX\\Desktop\\RBXL\\";       // where the repository rbxl are grabbed from
            // path in repository for auto build machine:
            //string placedir = @"..\\..\\..\\..\\Trunk\\Client\\RCCService.Test\\Resources";
            //string placedir = @"C:\\Users\\Administrator\\AppData\\Local\\Temp\\Doblox2\\Trunk\\Client\\RCCService.Test\\TestRBXLFiles";
            //string placedir = @"C:\\Users\\Administrator\\AppData\\Local\\Temp\\Doblox2\\client_mixed\\Sources\\Trunk\\Client\\RCCService.Test\\TestRBXLFiles";

            // try local directory for RBXL files first:
            string placedir;            // used to store the correct path among local and auto
            string localdir = @"..\\..\\..\\RCCService.Test\\TestRBXLFiles";
            string autodir = @"F:\\ClientBuildSource\\Trunk\\Client\\RCCService.Test\\TestRBXLFiles";
            //string autodir = @"..\\..\\..\\Sources\\Trunk\\Client\\RCCService.Test\\TestRBXLFiles";
            if (!autoBuild)
            {
                placedir = localdir;
            }
            else if (autoBuild)
            {
                placedir = autodir;
            }
            else
            {
                placedir = "";
                Assert.Fail("RBXL Directory was not copied to local machine in paths " + localdir + " or " + autodir);
            }



            /*
            string dirr = @"C:\Users\Administrator\AppData\Local\Temp\Doblox2\Client\Sources";

            System.IO.StreamWriter file = new System.IO.StreamWriter("c:\\dirr.txt");
            string[] subFolders = Directory.GetDirectories(dirr);
            foreach(string aSubFolder in subFolders)
                file.WriteLine(aSubFolder); 
            file.Close();
            */
            string dumpdir = @"F:\\RBXL_AUTO\\";   // where the repository rbxl are dumped to on local machine

            CreateRBXLDir(placedir, dumpdir);

            // auto-build RBXL directory:
            //string placedir = @"C:\\RBXL_files\\RBX-Test.rbxl";

            string[] fileEntries = Directory.GetFiles(dumpdir);

            foreach (string fileName in fileEntries)
            {

                using (RCCServiceSoap service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port))
                {

                    ScriptExecution script = new ScriptExecution();
                    script.name = "OpenAndRunRBXL";
                    script.script = Resources.LoadAndRun;       // see resources(.resx) textfile LoadAndRun for test script
                    script.arguments = Lua.NewArgs(fileName);

                    Roblox.Grid.Rcc.Job job = new Roblox.Grid.Rcc.Job();
                    job.id = "OpenAndRunRBXLFile";
                    job.expirationInSeconds = 60;      // Sets the timeout of this test.  Will fail if result is not set by this time.
                    job.category = 0;
                    try
                    {
                        service.OpenJob(job, script);

                        ScriptExecution check_script = new ScriptExecution();
                        check_script.name = "CheckPassCondition";
                        check_script.script = "return workspace.RBX_Test.Result.Value";

                        while (true)
                        {

                            object[] ret = Lua.GetValues(service.Execute(job.id, check_script));

                            // assuming result is binary and only changes once, this condition makes sure result is correct value:
                            if ((double)ret[0] != 0 && (double)ret[0] != PASS)
                                Assert.Fail("Test returned unexpected return value");
                            // if correct result is returned, break from infinite loop and passes the test
                            else if ((double)ret[0] == PASS)
                                break;
                            System.Threading.Thread.Sleep(500);        // wait 1 second until next poll...
                        }
                    }
                    finally
                    {

                        service.CloseJob(job.id);
                    }
                }
            }
        }
    }
}
