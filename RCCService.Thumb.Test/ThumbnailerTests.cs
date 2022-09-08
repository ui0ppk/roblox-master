using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Roblox.Grid.Rcc;
using Roblox.Grid;
using System.Diagnostics;
using System.IO;
using RCCService.Test;
using Microsoft.Ccr.Core;
using Roblox;

namespace RCCService.Test.Thumb
{
    /// <summary>
    /// Summary description for UnitTest1
    /// </summary>
    [TestClass]
    public class ThumbnailerTests
    {
        static public double THUMBNAIL_JOB_TIMEOUT = 5.0;
        static public double THUMBNAIL_SERVICE_TIMEOUT = 7.0;

        public ThumbnailerTests()
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
            //serviceUri = RCCLauncher.FindRemoteRCCService();
        }
        //
        // Use TestCleanup to run code after each test has run
        // [TestCleanup()]
        // public void MyTestCleanup() { }
        //
        #endregion


        public IEnumerator<ITask> RunScriptAsync(ScriptExecution script, int key)
        {
            try
            {
                var result = new PortSet<LuaValue[], Exception>();
                TimeSpan timeout = TimeSpan.FromMinutes(THUMBNAIL_JOB_TIMEOUT);
                Roblox.Grid.Rcc.Job job = new Roblox.Grid.Rcc.Job(); // ScriptJob(0, timeout, script);
                job.id = "TestToolThumJob " + Guid.NewGuid();
                job.expirationInSeconds = (int)timeout.TotalSeconds;
                job.category = 0;
                RCCServiceSoap service = null;
                try
                {
                    service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port);
                    Roblox.Common.AsyncHelper.Call(service.BeginBatchJobEx, job, script, service.EndBatchJobEx, result, timeout, service.Dispose );
                }
                catch (Exception)
                {
                    System.Threading.Interlocked.Increment(ref failure);
                    yield break;
                }

                yield return (Choice)result;

                Exception ex = result.Test<Exception>();
                if (ex != null)
                {
                    System.Threading.Interlocked.Increment(ref failure);
                    yield break;
                }

                var luaResult = result.Test<LuaValue[]>();

                if (luaResult == null || luaResult[0].value == "")
                {
                    System.Threading.Interlocked.Increment(ref failure);
                    yield break;
                }

                byte[] todecode_byte = Convert.FromBase64String(luaResult[0].value);

                using (FileStream fs = new FileStream(Path.Combine(TestContext.TestDir, "ThumbOutput" + key + ".png"), FileMode.Create))
                {
                    fs.Write(todecode_byte, 0, todecode_byte.Length);
                    TestContext.WriteLine("Output thumb written to: " + fs.Name);
                }
                yield break;
            }
            finally
            {
                System.Threading.Interlocked.Increment(ref done);
            }

        }
        public void RunScript(ScriptExecution script)
        {
            using (RCCServiceSoap service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port))
            {
                Roblox.Grid.Rcc.Job job = new Roblox.Grid.Rcc.Job(); // ScriptJob(0, timeout, script);
                job.id = "TestToolThumJob" + DateTime.Now.ToString() + "_" + DateTime.Now.Millisecond.ToString();
                job.expirationInSeconds = (int)TimeSpan.FromMinutes(THUMBNAIL_JOB_TIMEOUT).TotalSeconds;
                job.category = 0;
                LuaValue[] result = null;

                String exp = String.Empty;
                try
                {
                    // 3 min timeout
                    service.Timeout = (Int32)(1000 * 60 * THUMBNAIL_SERVICE_TIMEOUT);
                    result = service.BatchJobEx(job, script);
                }
                catch (System.Net.WebException e)
                {
                    exp = e.Message;
                    Console.Write(e.Message);
                }
                Assert.IsTrue(result != null, exp);
                Assert.IsTrue(result[0].value != "");

                byte[] todecode_byte = Convert.FromBase64String(result[0].value);

                using (FileStream fs = new FileStream(Path.Combine(TestContext.TestDir, "ThumbOutput.png"), FileMode.Create))
                {
                    fs.Write(todecode_byte, 0, todecode_byte.Length);
                    TestContext.WriteLine("Output thumb written to: " + fs.Name);
                }

            }

        }
        
        [TestMethod]
        public void TestToolThumb()
        {
            RunScript(LuaScripts.MakeToolThumbScript(47613, 256, 256));
        }

        [TestMethod]
        public void TestPlaceThumbBaseplateWithWood()
        {
            RunScript(LuaScripts.MakePlaceThumbScript( 4604875, 0, 512, 512)); 
        }

        [TestMethod]
        public void TestPlaceThumbCrossroads()
        {
            RunScript(LuaScripts.MakePlaceThumbScript(1818, 0, 512, 512));
        }

        [TestMethod]
        public void TestPlaceThumbChaosCanyon()
        {
            RunScript(LuaScripts.MakePlaceThumbScript(14403, 0, 512, 512));
        }

        [TestMethod]
        public void TestPlaceThumbWTRB()
        {
            RunScript(LuaScripts.MakePlaceThumbScript(41324860, 0, 512, 512));
        }

        [TestMethod]
        public void TestPlaceThumbSFOH()
        {
            RunScript(LuaScripts.MakePlaceThumbScript(47324, 0, 512, 512));
        }

        [TestMethod]
        public void TestPlaceThumbHappyHome()
        {
            RunScript(LuaScripts.MakePlaceThumbScript(65033, 0, 512, 512));
        }
        
        int done;
        int failure;

        
        [TestMethod]
        public void TestPlaceRoGymTycoon()
        {
            RunScript(LuaScripts.MakePlaceThumbScript(27420024, 0, 512, 512));
        }

        [TestMethod]
        public void TestPlaceMariusHQ()
        {
            RunScript(LuaScripts.MakePlaceThumbScript(24518738, 0, 512, 512));
        }        

        [TestMethod]
        public void TestNewHeadThumbOnGT1()
        {
            using (LuaScripts.pushBaseUrl("http://www.gametest1.robloxlabs.com/"))
            {
                RunScript(LuaScripts.MakeHeadScript(8330578, 1024, 1024));  // Peabody head
            }
        }

        [TestMethod]
        public void TestShirtThumbOnGT1()
        {
            using (LuaScripts.pushBaseUrl("http://www.gametest1.robloxlabs.com/"))
            {
                RunScript(LuaScripts.MakeCustomThumbScript(LuaScripts.ShirtScript,
                    "http://www.gametest1.robloxlabs.com/asset/?id=81522820",
                    "PNG",
                    1024, 1024,
                    LuaScripts.getBaseUrl(),
                    "http://www.roblox.com/asset/?id=242902171"));  //Roblox'S avatar

            }
        }

        [TestMethod]
        public void TestPlaceThumbCosmic()
        {
            RunScript(LuaScripts.MakePlaceThumbScript(277989, 0, 1024, 1024));  // Cosmic Ride by senro
        }

        [TestMethod]
        public void TestPlaceThumbWhosKillingNintendo()
        {
            RunScript(LuaScripts.MakePlaceThumbScript(12088157, 0, 1024, 1024));  
        }
        
                
        [TestMethod]
        public void TestRendermanAvatarThumb()
        {
            {
                RunScript(LuaScripts.MakeCustomThumbScript(LuaScripts.AvatarThumbScript,
                    "http://www.roblox.com/Asset/AvatarAccoutrements.ashx?userId=1127502",
                    LuaScripts.getBaseUrl(),
                    "PNG",
                    1024, 1024));  //RENDERMAN'S avatar
            }
        }

        [TestMethod]
        public void TestMeshModelOnGT1()
        {
            using (LuaScripts.pushBaseUrl("http://www.gametest1.roblox.com/"))
            {
                RunScript(LuaScripts.MakeCustomThumbScript(LuaScripts.BodyPartScript,
                    "http://www.gametest1.roblox.com/Bear-Torso-item?id=25308616",
                    "http://www.gametest1.roblox.com/",
                    "PNG",
                    1024, 1024,
                    "http://www.gametest1.roblox.com/Asset/?id=1785197"));
            }
        }
        [TestMethod]
        public void TestRobloxAvatarThumbOnGT1()
        {
            using (LuaScripts.pushBaseUrl("http://www.gametest1.robloxlabs.com/"))
            {
                RunScript(LuaScripts.MakeCustomThumbScript(LuaScripts.AvatarThumbScript,
                    "http://www.gametest1.robloxlabs.com/Asset/AvatarAccoutrements.ashx?userId=1",
                    LuaScripts.getBaseUrl(),
                    "PNG",
                    1024, 1024,
                    "http://www.roblox.com/Asset/AvatarAccoutrements.ashx?userId=504316"));  //Roblox'S avatar
            }
        }

        [TestMethod]
        public void TestTorsoThumb()
        {
            {
                RunScript(LuaScripts.MakeCustomThumbScript(LuaScripts.BodyPartScript,
                    "http://www.roblox.com/Bear-Torso-item?id=25004305",
                    "http://www.roblox.com/",
                    "PNG",
                    1024, 1024,
                    "http://www.roblox.com/Asset/?id=1785197"));  //RENDERMAN'S avatar
            }
        }

        [TestMethod]
        public void TestBrightEyesAvatarThumb()
        {
            RunScript(LuaScripts.MakeCustomThumbScript(LuaScripts.AvatarThumbScript,
                "http://www.roblox.com/Asset/AvatarAccoutrements.ashx?userId=504316",
                    LuaScripts.getBaseUrl(),
                    "PNG",
                    1024, 1024));  
        }

        [TestMethod]
        public void TestTheLastBunnyAvatarThumb()
        {
            RunScript(LuaScripts.MakeCustomThumbScript(LuaScripts.AvatarThumbScript,
                "http://www.roblox.com/Asset/AvatarAccoutrements.ashx?userId=4052914",
                    LuaScripts.getBaseUrl(),
                    "PNG",
                    1024, 1024));
        }

        [TestMethod]    
        public void TestEnigmaTest()
        {
            RunScript(LuaScripts.MakePlaceThumbScript(0, 532780, 1024, 1024));  
        }

        [TestMethod]
        public void TestBall5Model()
        {
            RunScript(LuaScripts.MakeModelThumbScript(7571380, 0, 1024, 1024));  
        }

        [TestMethod]
        public void TestGearWithCameraOnGT1()
        {
            using (LuaScripts.pushBaseUrl("http://www.gametest1.roblox.com/"))
            {
                RunScript(LuaScripts.MakeGearThumbScript(73089239, 0, 1024, 1024));
            }
        }
 
        [TestMethod]
        public void TestWedgeAndDecal()
        {
            RunScript(LuaScripts.MakePlaceThumbScript(8995054, 0, 1024, 1024));
        }
        

    }
}
