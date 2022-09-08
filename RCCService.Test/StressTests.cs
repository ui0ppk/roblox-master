#define USE_REMOTE_CLIENTS

using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Roblox.Grid.Rcc;
using Roblox.Grid;
using System.IO;
using System.Threading;
using System.Diagnostics;

namespace RCCService.Test
{
    /// <summary>
    /// Summary description for StressTest
    /// </summary>
    [TestClass]
    public class StressTests
    {
        public StressTests()
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


        private class GameInfo
        {
            public GameInfo(int gamenum, string id, Uri gameUri, bool active)
            {
                this.gamenum = gamenum;
                this.id = id;
                this.active = active;
                this.gameUri = gameUri;
            }

            public int gamenum;
            public string id;
            public bool active;
            public Uri gameUri;
        };

        class ClientManager : IDisposable
        {
            int servicenum;
            private Uri uri;
            private RCCServiceSoap service;
            private List<GameInfo> games;
            public static int webRequestTimeout = 60* 1000;
            public static bool ignoreWebExceptions = false; // set this to true for tests where you expect occasionnal client downs.

            public static List<ClientManager> CreateNRemoteClients(int firstclientnum, int numclients)
            {
                List<ClientManager> result = new List<ClientManager>();
                List<Uri> clientUris = new List<Uri>();
#if USE_REMOTE_CLIENTS
                clientUris = RCCLauncher.FindNMoreRemoteRCCServices(numclients);
#else
                for (int i = 0; i < numclients; ++i)
                {
                    clientUris.Add(RCCLauncher.StartRCCService());
                }
#endif
                foreach (Uri uri in clientUris)
                {
                    result.Add(new ClientManager(firstclientnum++, uri));
                }
                return result;
            }

            public ClientManager(int servicenum, Uri uri)
            {
                this.servicenum = servicenum;
                this.uri = uri;
                this.games = new List<GameInfo>();
            }

            public void AquireService()
            {
                service = Roblox.Grid.Common.GridServiceUtils.GetService(uri.Host, uri.Port);
                service.Timeout = webRequestTimeout;
                service.CloseAllJobs(); // cleanup potential crumbs from failed previous run
            }

            public void ReAquireServiceAndGames()
            {
                this.uri = RCCLauncher.FindRemoteRCCService();
                AquireService();

                // try to re-join all the games that we used to be in
                foreach (GameInfo gi in games)
                {
                    Roblox.Grid.Rcc.Job job = new Roblox.Grid.Rcc.Job();
                    job.expirationInSeconds = Math.Max(2 * 60 * 60, webRequestTimeout / 1000);
                    job.category = 0;

                    job.id = gi.id;

                    service.OpenJob(job, LuaScripts.MakeJoinGameScript(gi.gameUri.Host/*Lists.routerAddress*/, gi.gameUri.Port));

                    gi.active = false;

                }

            }

            public void EnsureUp()
            {
                if (!RCCLauncher.IsServiceAvailable(uri.Host, uri.Port, 0))
                {
                    Stop();
                    ReAquireServiceAndGames();
                }
            }

            private void CloseGameWorker(GameInfo game)
            {
                try
                {
                    service.CloseJob(game.id);
                }
                catch (System.Net.WebException)
                {
                    if (!ignoreWebExceptions)
                    {
                        throw;
                    }
                }
            }

            // close job connected to game if exists (returns true if exists)
            public bool CloseGame(int gamenum)
            {
                bool b = false;
                for (int i = games.Count-1; i != -1; --i)
                {
                    if (games[i].gamenum == gamenum)
                    {
                        CloseGameWorker(games[i]);
                        games.RemoveAt(i);
                        b = true;
                    }
                }
                return b;
            }

            private void JoinGameWorker(GameInfo newgame)
            {
                Roblox.Grid.Rcc.Job job = new Roblox.Grid.Rcc.Job();
                job.expirationInSeconds = Math.Max(2 * 60 * 60, webRequestTimeout / 1000);
                job.category = 0;
                job.id = newgame.id;

                try
                {
                    service.OpenJob(job, LuaScripts.MakeJoinGameScript(newgame.gameUri.Host/*Lists.routerAddress*/, newgame.gameUri.Port));
                }
                catch (System.Net.WebException)
                {
                    if (!ignoreWebExceptions)
                    {
                        throw;
                    }
                }
            }

            public GameInfo JoinGame(GameInfo servergame, int clientnum)
            {
                GameInfo newgame = new GameInfo(
                    servergame.gamenum,
                    servergame.id + "_Client#" + clientnum.ToString(),
                    servergame.gameUri,
                    false); // we use active for wether or not the busy script is running.

                games.Add(newgame);

                JoinGameWorker(newgame);

                return newgame;
            }

            public void ActivateInactiveGames(ScriptExecution scriptExecution)
            {
                foreach (GameInfo gi in games)
                {
                    if (gi.active)
                    {
                        continue;
                    }
                    try
                    {
                        service.Execute(gi.id, scriptExecution);
                        gi.active = true;
                    }
                    catch (System.Net.WebException)
                    {
                        if (!ignoreWebExceptions)
                        {
                            throw;
                        }
                    }
                    catch (Exception)
                    {
                        // not critical
                    }

                }
            }

            public void Stop()
            {
                if (uri != null)
                {
                    RCCLauncher.StopRCCService(uri);
                    uri = null;
                }
            }

            #region IDisposable Members

            public void Dispose()
            {
                Stop();
            }

            #endregion

            public void Execute(string jobid, ScriptExecution scriptExecution)
            {
                try
                {
                    service.Execute(jobid, scriptExecution);
                }
                catch (System.Net.WebException)
                {
                    if (!ignoreWebExceptions)
                    {
                        throw;
                    }
                }
            }

            public int GameCount()
            {
                return games.Count;
            }

            public void ToggleGame(int gamei)
            {
                CloseGameWorker(games[gamei]);
                games[gamei].active = false;
                JoinGameWorker(games[gamei]);
            }
        };

        private Uri serverUri;
        private List<ClientManager> rccclients = new List<ClientManager>();

        [TestInitialize()]
        public void MyTestInitialize()
        {
            // complex multi-rcc server test required clean slate.
            RCCLauncher.StopAllRCCServices();
            //serverUri = new Uri("");
            rccclients.Clear();
        }

        [TestCleanup()]
        public void MyTestCleanup()
        {
            RCCLauncher.StopRCCService(serverUri);
            foreach (ClientManager c in rccclients)
            {
                c.Stop();
            }
            rccclients.Clear();
        }



        private delegate void CloseJobDelegateType(int gamenum);

        public delegate ScriptExecution ServerScriptDelegateType(int placerun);

        public static void DefaultWaitDelegate(object delegateParams)
        {
            foreach (ClientManager cm in ((StressTests)delegateParams).rccclients)
            {
                cm.EnsureUp();
                cm.ActivateInactiveGames(LuaScripts.MakeBusyScript());
            }
        }

        public class StressClientServerParams
        {
            public int numGamesOnServer = 1;
            public int numClients = 8;
            public int numRCCClients = 4;
            public int numPlacesToLoad = 1;
            public int waitMSEachPlace = 60 * 1000;
            public int webRequestTimeout = defaultWebRequestTimeout;
            public bool allowFailingClients = false;
            public ServerScriptDelegateType scriptChooser = null;
            public TestUtils.waitDelegate waitDelegate = DefaultWaitDelegate;
            public int samplePeriodms = 60 * 1000; // also corresponds to frequency of call to waitDelegate.

            public StressClientServerParams()
            {
            }

            public StressClientServerParams(int numGamesOnServer, int numClients, int numRCCClients, int numPlacesToLoad, int waitMSEachPlace, int webRequestTimeout, bool allowFailingClients, ServerScriptDelegateType scriptChooser)
            {
                this.numGamesOnServer = numGamesOnServer;
                this.numClients = numClients;
                this.numRCCClients = numRCCClients;
                this.numPlacesToLoad = numPlacesToLoad;
                this.waitMSEachPlace = waitMSEachPlace;
                this.webRequestTimeout = webRequestTimeout;
                this.allowFailingClients = allowFailingClients;
                this.scriptChooser = scriptChooser;

            }
            public StressClientServerParams(int numGamesOnServer, int numClients, int numRCCClients, int numPlacesToLoad, int waitMSEachPlace, int webRequestTimeout, bool allowFailingClients, int[] placeAssetIds)
            {
                this.numGamesOnServer = numGamesOnServer;
                this.numClients = numClients;
                this.numRCCClients = numRCCClients;
                this.numPlacesToLoad = numPlacesToLoad;
                this.waitMSEachPlace = waitMSEachPlace;
                this.webRequestTimeout = webRequestTimeout;
                this.allowFailingClients = allowFailingClients;

                this.placeAssetIds = placeAssetIds;
            }

            public int[] placeAssetIds
            {
                set
                {
                    scriptChooser = delegate(int placerun)
                    {
                        return LuaScripts.MakeLoadPlaceScript(value[placerun % value.Length]);
                    };
                    
                }
            }


        }


        public void StressClientServer(StressClientServerParams p)
        {
            TestContext.WriteLine("Simultaneous games on server: {0}", p.numGamesOnServer);
            TestContext.WriteLine("Number of clients connected: {0}", p.numClients);
            TestContext.WriteLine("Number of RCCClients spawned: {0}", p.numRCCClients);
            TestContext.WriteLine("Number of places loaded: {0}", p.numPlacesToLoad);
            TestContext.WriteLine("Wait in running game in milliseconds: {0}", p.waitMSEachPlace);

            //Debug.Assert(numRCCClients >= numClients / numGamesOnServer, "number of spawend RCCClients must equal or exceed the number of clients per games. (so that the same RCCClient won't connect twice on the same game)");
            serverUri = RCCLauncher.StartRCCService();

            ClientManager.webRequestTimeout = p.webRequestTimeout;
            ClientManager.ignoreWebExceptions = p.allowFailingClients;
            rccclients = ClientManager.CreateNRemoteClients(0, p.numRCCClients);

            int gameport = 53640;
            List<GameInfo> serverjobids = new List<GameInfo>(p.numGamesOnServer);
            for (int i = 0; i < p.numGamesOnServer; ++i)
            {
                UriBuilder urib = new UriBuilder(serverUri);
                urib.Port = gameport++;

                serverjobids.Add(new GameInfo(i, "", urib.Uri, false));
            }

            int currentgame = 0; // cycle commands through each game server.

            RCCServiceSoap serverService = null;

            CloseJobDelegateType closeJobDelegate = delegate(int gamenum)
            {
                foreach (ClientManager c in rccclients)
                {
                    c.CloseGame(gamenum);
                }

                serverService.CloseJob(serverjobids[gamenum].id);
                serverjobids[gamenum].active = false;
            };

            try // try/finalluy block to make sure we dispose the serverService and the clientService
            {
                serverService = Roblox.Grid.Common.GridServiceUtils.GetService(serverUri.Host, serverUri.Port);
                serverService.Timeout = p.webRequestTimeout;
                serverService.CloseAllJobs(); // cleanup potential crumbs from failed previous run
                foreach (ClientManager c in rccclients)
                {
                    c.AquireService();
                }

                Roblox.Grid.Rcc.Job job = new Roblox.Grid.Rcc.Job();
                job.expirationInSeconds = Math.Max(2 * 60 * 60, p.webRequestTimeout / 1000);
                job.category = 0;

                TestContext.WriteLine("PrivateWorkingSetBytes: run#, workingset bytes, bytes, cores");

                for (int placerun = 0; placerun < p.numPlacesToLoad; placerun++)
                {
                    LuaValue[] result = null;

                    if (serverjobids[currentgame].active == true)
                    {
                        closeJobDelegate(currentgame);
                    }

                    // save job ID so we can use it now, and close it later.
                    serverjobids[currentgame].id = "TestSimpleClientServer_game" + currentgame.ToString() + "_placerun" + placerun.ToString();
                    job.id = serverjobids[currentgame].id;
                    result = serverService.OpenJob(job, Lua.EmptyScript);
                    serverjobids[currentgame].active = true;

                    result = serverService.Execute(serverjobids[currentgame].id, p.scriptChooser(placerun));

                    result = serverService.Execute(serverjobids[currentgame].id, Lua.NewScript("StartGameServer", LuaScripts.StartGameServer(), serverjobids[currentgame].gameUri.Port));

                    // 3568125
                    // empty place: 381342

                    //connect clients, partition client pool so that we don't reallocate _All_ the clients to the most recently opened game.
                    // on a 4 game server, with 16 clients:
                    // clients 0, 1, 2, 4 would go on game 0
                    // clients 5, 6, 7, 8 would go on game 1, etc...
                    int expectedClientCount = 0;
                    Dictionary<int, GameInfo> clientGames = new Dictionary<int, GameInfo>();
                    for (int clienti = (p.numClients * currentgame) / p.numGamesOnServer; clienti < (p.numClients * (currentgame + 1)) / p.numGamesOnServer; ++clienti)
                    {
                        int wrappedClienti = clienti % rccclients.Count();
                        clientGames[wrappedClienti] = rccclients[wrappedClienti].JoinGame(serverjobids[currentgame], clienti);
                    }

                    foreach (ClientManager cm in rccclients)
                    {
                        cm.EnsureUp();
                    }

                    TestUtils.WaitForNClients(serverService, serverjobids[currentgame].id, clientGames.Count, 15 * 1000);

                    // only do the full wait once the pipeline is full.
                    int waitms = (placerun + 1 >= p.numGamesOnServer) ? p.waitMSEachPlace : 0;
                    TestUtils.PrintStatisticsAndWait(serverService, serverjobids[currentgame].id, TestContext, placerun, waitms / p.samplePeriodms, p.samplePeriodms, p.waitDelegate, this);

                    foreach (ClientManager cm in rccclients)
                    {
                        cm.EnsureUp();
                    }
                    // check to make sure clients are still connected.
                    TestUtils.WaitForNClients(serverService, serverjobids[currentgame].id, expectedClientCount, 0);

                    // cycle through each game on the server.
                    currentgame = (currentgame + 1) % p.numGamesOnServer;
                } //for

                // cleanup
                for (int i = 0; i < p.numGamesOnServer; ++i)
                {
                    closeJobDelegate(i);
                } // for
            }
            finally
            {
                serverService.Dispose();
                for (int clienti = 0; clienti < rccclients.Count; ++clienti)
                {
                    rccclients[clienti].Dispose();
                }
            }
        } // func

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

        private const int defaultWebRequestTimeout = 60 * 1000;

        [TestMethod]
        public void TestStressLoadManyPopularPlaces()
        {
            StressClientServer( new StressClientServerParams(1, 0, 0,  200, 0, defaultWebRequestTimeout, false, Lists.popularPlaces));
        }

        [TestMethod]
        public void TestStress4Games16Clients()
        {
            StressClientServer( new StressClientServerParams(4, 16, 4, 4, 0, defaultWebRequestTimeout, false, Lists.popularPlaces));
        }

        [TestMethod]
        [Timeout(2*60*60*1000)] // 2 hours
        public void TestStress4Games16ClientsOverManyPopularPlaces()
        {
            StressClientServer( new StressClientServerParams(4, 16, 4, 200, 0, defaultWebRequestTimeout, false, Lists.popularPlaces));
        }

        [TestMethod]
        [Timeout(2 * 60 * 60 * 1000)] // 2 hours
        public void TestStress8Games8ClientsOverTop25()
        {
            StressClientServer( new StressClientServerParams(8, 8, 1, 100, 0, defaultWebRequestTimeout * 4, false, Lists.top25));
        }

        [TestMethod]
        [Timeout(2 * 60 * 60 * 1000)] // 2 hours
        public void TestStress8Games16ClientsOverTop25()
        {
            StressClientServer( new StressClientServerParams(8, 16, 4, 100, 0, defaultWebRequestTimeout * 4, false, Lists.top25));
        }

        [TestMethod]
        [Timeout(2 * 60 * 60 * 1000)] // 2 hours
        public void TestStress8Games32ClientsOverTop25()
        {
            StressClientServer( new StressClientServerParams(8, 32, 4, 100, 0, defaultWebRequestTimeout * 4, false, Lists.top25));
        }

        [TestMethod]
        [Timeout(2 * 60 * 60 * 1000)] // 2 hours
        public void TestStress8Games64ClientsOverTop25()
        {
            StressClientServer( new StressClientServerParams(8, 64, 8, 100, 0, defaultWebRequestTimeout * 4, false, Lists.top25));
        }

        [TestMethod]
        public void TestStressPhysicsBenchmarkShort()
        {
            StressClientServer( new StressClientServerParams(4, 8, 2, 4, 15 * 1000, defaultWebRequestTimeout * 2, false, new int[] { 4420793 }));
        }

        [TestMethod]
        public void TestStress1Games8ClientsMTBF()
        {
            StressClientServer(new StressClientServerParams(1, 8, 8, 1, 10 * 60 * 60 * 1000 /*10 hours*/, defaultWebRequestTimeout * 2, false, new int[] { 7074883 }));
        }

        [TestMethod]
        [Timeout(20 * 60 * 60 * 1000)] // 20 hours
        public void TestStress8Games128ClientsMTBF()
        {
            StressClientServer( new StressClientServerParams(8, 128, 16, 1, 10 * 60 * 60 * 1000 /*10 hours*/, defaultWebRequestTimeout * 2, true, new int[] { 7074883 }));
        }
        [TestMethod]
        [Timeout(20 * 60 * 60 * 1000)] // 20 hours
        public void TestStress8Games128ClientsConnectDisconnect()
        {
            Random rand = new Random();
            StressClientServerParams p = new StressClientServerParams();
            p.placeAssetIds = new int[] { 7074883 };

            p.waitMSEachPlace = 60 * 60 * 1000; /*1 hours*/

            p.numClients = 128;
            p.numRCCClients = 16;
            int minutesConnectedPerClient = 3;

            p.samplePeriodms = minutesConnectedPerClient * 60 * 1000 / p.numClients;
            p.waitDelegate = delegate(object waitParams)
            {
                // do the usual first
                StressTests pthis = (StressTests)waitParams;
                DefaultWaitDelegate(pthis);
                
                // randomly disconnect and reconnect a client every p.samplePreiodms.
                // equivalent to having every client be connected on average "minutesConnectedPerClient"
                int rccclientnum = rand.Next(pthis.rccclients.Count);
                int gamenum = rand.Next(pthis.rccclients[rccclientnum].GameCount());
                pthis.rccclients[rccclientnum].ToggleGame(gamenum);
                //pthis.rccclients[rccclientnum].

            };

            StressClientServer(p);
        }
    }
}
