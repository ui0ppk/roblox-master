//test

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
    /// Summary description for UnitTest1
    /// </summary>
    [TestClass]
    public class BasicScenarios
    {
        public BasicScenarios()
        {
            //
            // TODO: Add constructor logic here
            //
        }

        private TestContext testContextInstance;

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
            serviceUri = RCCLauncher.FindOrStartRCCService();
        }
        //
        // Use TestCleanup to run code after each test has run
        // [TestCleanup()]
        // public void MyTestCleanup() { }
        //
        #endregion

        [TestMethod]
        public void TestStartStopJob()
        {
            using (RCCServiceSoap service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port))
            {
                ScriptExecution script = new ScriptExecution();
                script.name = "Test";
                script.script = "return 'Hello World! ' .. ...";
                script.arguments = Lua.NewArgs("0");
                Roblox.Grid.Rcc.Job job = new Roblox.Grid.Rcc.Job(); // ScriptJob(0, timeout, script);
                job.id = "TestStartStopJob";
                job.expirationInSeconds = 11111;
                job.category = 0;
                try
                {
                    service.OpenJob(job, script);
                }
                finally
                {
                    service.CloseJob(job.id);
                }
            }
        }

        [TestMethod]
        public void TestLoadPlace()
        {
            using (LuaScripts.pushBaseUrl("http://gametest.roblox.com/"))
            {

                using (RCCServiceSoap service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port))
                {
                    Roblox.Grid.Rcc.Job job = new Roblox.Grid.Rcc.Job(); // ScriptJob(0, timeout, script);
                    job.id = "TestLoadPlace";
                    job.expirationInSeconds = 11111;
                    job.category = 0;
                    try
                    {
                        service.OpenJob(job, Lua.EmptyScript);

                        service.Execute(job.id, LuaScripts.MakeLoadPlaceScript(129632223));
                    }
                    finally
                    {
                        service.CloseJob(job.id);
                    }
                }
            }
        }

        [TestMethod]
        public void TestLoadModel()
        {
            using (RCCServiceSoap service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port))
            {
                Roblox.Grid.Rcc.Job job = new Roblox.Grid.Rcc.Job(); // ScriptJob(0, timeout, script);
                job.id = "TestLoadModel";
                job.expirationInSeconds = 11111;
                job.category = 0;
                try
                {
                    service.OpenJob(job, Lua.EmptyScript);

                    LuaValue[] result = service.Execute(job.id, Lua.NewScript("LoadUnload", LuaScripts.LoadUnload, "http://www.roblox.com/asset/?id=4446382"));

                }
                finally
                {
                    service.CloseJob(job.id);
                }
            }
        }

        [TestMethod]
        public void TestStartGame()
        {
            using (RCCServiceSoap service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port))
            {
                Roblox.Grid.Rcc.Job job = new Roblox.Grid.Rcc.Job(); // ScriptJob(0, timeout, script);
                job.id = "TestStartGame";
                job.expirationInSeconds = 11111;
                job.category = 0;
                try
                {
                    service.OpenJob(job, Lua.EmptyScript);

                    LuaValue[] result = service.Execute(job.id, Lua.NewScript("StartGameServer", LuaScripts.StartGameServer(), 53640));

                }
                finally
                {
                    service.CloseJob(job.id);
                }
            }

        }

        [TestMethod]
        public void TestJobTimeout()
        {
            //TestContext.AddResultFile("result.txt");
            using (RCCServiceSoap service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port))
            {
                Roblox.Grid.Rcc.Job job = new Roblox.Grid.Rcc.Job(); // ScriptJob(0, timeout, script);
                job.id = "TestGetDiagnostics";
                job.expirationInSeconds = 1; //expire in 1 second
                job.category = 0;
                service.OpenJob(job, Lua.EmptyScript);

                System.Threading.Thread.Sleep(2000); // wait 2 seconds.

                bool exthrown = false;
                try
                {
                    LuaValue[] result = service.Execute(job.id, Lua.EmptyScript);
                }
                catch (System.Web.Services.Protocols.SoapHeaderException)
                {
                    exthrown = true;
                }

                service.CloseJob(job.id);

                Assert.IsTrue(exthrown, "Expected job to not be available after timout");
            }

        }




        [TestMethod]
        public void TestGetDiagnostics()
        {
            //TestContext.AddResultFile("result.txt");
            using (RCCServiceSoap service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port))
            {
                Roblox.Grid.Rcc.Job job = new Roblox.Grid.Rcc.Job(); // ScriptJob(0, timeout, script);
                job.id = "TestGetDiagnostics";
                job.expirationInSeconds = 11111;
                job.category = 0;

                service.OpenJob(job, Lua.EmptyScript);

                LuaValue[] result = service.Execute(job.id, Lua.NewScript("GetPrivateWorkingSetBytes", "return settings().Diagnostics.PrivateWorkingSetBytes"));

                service.CloseJob(job.id);

                TestContext.WriteLine("PrivateWorkingSetBytes is:{0} bytes", result[0].value);
                Assert.IsTrue(Int32.Parse(result[0].value) < 32000000, "Baseline memory usage exceeded");

            }
        }


        [TestMethod]
        public void TestStressLoadModel()
        {
            using (RCCServiceSoap service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port))
            {
                Roblox.Grid.Rcc.Job job = new Roblox.Grid.Rcc.Job(); // ScriptJob(0, timeout, script);
                job.id = "TestStressLoadModel";
                job.expirationInSeconds = 11111;
                job.category = 0;

                service.OpenJob(job, Lua.EmptyScript);

                for (int i = 0; i < 100; i++)
                {
                    LuaValue[] result = service.Execute(job.id, Lua.NewScript("LoadUnload", LuaScripts.LoadUnload, "http://www.roblox.com/asset/?id=4446382"));
                }

                service.CloseJob(job.id);
            }
        }


    }
}
