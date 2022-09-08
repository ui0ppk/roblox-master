#define USE_REMOTE_CLIENTS

using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Roblox.Grid.Rcc;
using Roblox.Grid;
using System.Threading;

namespace RCCService.Test
{
    /// <summary>
    /// Summary description for StressTests
    /// </summary>
    [TestClass]
    public class ServerClientTests
    {
        public ServerClientTests()
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

        private const int numRCCClients = 1;
        private const int numClients = 1;
        private Uri serverUri;
        private List<Uri> clientUris = new List<Uri>();

        // Use TestInitialize to run code before running each test 
        [TestInitialize()]
        public void MyTestInitialize() 
        {
            // complex multi-rcc server test required clean slate.
            RCCLauncher.StopAllRCCServices();

            serverUri = RCCLauncher.StartRCCService();

#if USE_REMOTE_CLIENTS
            clientUris = RCCLauncher.FindNMoreRemoteRCCServices(numRCCClients);
#else
            for (int i = 0; i < numRCCClients; ++i)
            {
                clientUris.Add(RCCLauncher.StartRCCService());
            }
#endif

        }
        //
        // Use TestCleanup to run code after each test has run
        [TestCleanup()]
        public void MyTestCleanup() 
        {
            RCCLauncher.StopRCCService(serverUri);
            foreach (Uri uri in clientUris)
            {
                RCCLauncher.StopRCCService(uri);
            }
            clientUris.Clear();
        }

        #region Additional test attributes
        //
        // You can use the following additional attributes as you write your tests:
        //
        // Use ClassInitialize to run code before running the first test in the class
        // [ClassInitialize()]
        // public static void MyClassInitialize(TestContext testContext) { }
        //
        // Use ClassCleanup to run code after all tests in a class have run
        // [ClassCleanup()]
        // public static void MyClassCleanup() { }
        //
        // Use TestInitialize to run code before running each test 
        // [TestInitialize()]
        // public void MyTestInitialize() { }
        //
        // Use TestCleanup to run code after each test has run
        // [TestCleanup()]
        // public void MyTestCleanup() { }
        //
        #endregion


        [TestMethod]
        public void TestSimpleClientServer()
        {

            int gameport = 53640;
            using (RCCServiceSoap serverService = Roblox.Grid.Common.GridServiceUtils.GetService(serverUri.Host, serverUri.Port))
            {
                Roblox.Grid.Rcc.Job serverjob = new Roblox.Grid.Rcc.Job(); // ScriptJob(0, timeout, script);
                serverjob.id = "TestSimpleClientServer";
                serverjob.expirationInSeconds = 60;
                serverjob.category = 0;
                //try
                {
                    LuaValue[] result = null;

                    result = serverService.OpenJob(serverjob, Lua.EmptyScript);

                    // 3568125
                    // empty place: 381342
                    result = serverService.Execute(serverjob.id, LuaScripts.MakeLoadPlaceScript(381342));
                        
                    result = serverService.Execute(serverjob.id, Lua.NewScript("StartGameServer", LuaScripts.StartGameServer(), gameport));


                    List<RCCServiceSoap> clientservices = new List<RCCServiceSoap>();
                    Roblox.Grid.Rcc.Job clientjob = new Roblox.Grid.Rcc.Job(); // ScriptJob(0, timeout, script);
                    string clientjobbaseid = "TestSimpleClientServer_client";
                    clientjob.expirationInSeconds = 60;
                    clientjob.category = 0;
                    try
                    {

                        foreach (Uri clienturi in clientUris)
                        {
                            clientservices.Add(Roblox.Grid.Common.GridServiceUtils.GetService(clienturi.Host, clienturi.Port));
                        }

                        for (int clientnum = 0; clientnum < numClients; ++clientnum)
                        {
                            clientjob.id = clientjobbaseid + clientnum.ToString();
                            clientservices[clientnum % clientservices.Count()].OpenJob(clientjob, LuaScripts.MakeJoinGameScript(serverUri.Host/*Lists.routerAddress*/, gameport));
                        }

                        TestUtils.WaitForNClients(serverService, serverjob.id, numClients, 1000);

                        for (int clientnum = 0; clientnum < numClients; ++clientnum)
                        {
                            clientjob.id = clientjobbaseid + clientnum.ToString();
                            clientservices[clientnum % clientservices.Count()].CloseJob(clientjob.id);
                        }
                    }
                    finally
                    {
                        foreach (RCCServiceSoap client in clientservices)
                        {
                            client.Dispose();
                        }
                    }

                }

                serverService.CloseJob(serverjob.id);
            }
        }



    }
}
