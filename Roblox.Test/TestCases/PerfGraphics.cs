using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.Threading;
using System.IO;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Reflection;


namespace Roblox.Test
{
    [TestClass]
    [Serializable]
    public class PerfGraphics
    {
        Process proc;

        public PerfGraphics()
        {
            proc = new Process();

        }

        /*public PerfGraphics(string DeploymentDir, TextWriter result)
        {

            TestDeploymentDir = DeploymentDir;

            Result = result;
            //
            // TODO: Add constructor logic here
            //

        }*/

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


        private const string TestOutputDir = "S:\\Automation\\Results\\";
        private const string RobloxAppBuildTypeConfigFile = "S:\\Automation\\Roblox\\bin\\dirs";
        private string RobloxAppFilename;
        private const string RobloxAppArgFormat = "/NonInteractive /Settings \"{0}\"";
        private const string RobloxAppDefaultFilenameFormat = "S:\\Automation\\Roblox\\bin\\{0}\\RobloxApp.exe";

        private string FindRobloxApp()
        {
            string publishbinrootenv = Environment.GetEnvironmentVariable("PublishedBinRoot");

            List<string> searchPaths = new List<string>();

            if (!String.IsNullOrEmpty(publishbinrootenv))
            {
                searchPaths.Add(Path.Combine(publishbinrootenv, "RobloxApp.exe"));
            }
            searchPaths.Add(Path.Combine(TestContext.TestDeploymentDir, "RobloxApp.exe"));
            searchPaths.Add(Path.Combine(TestContext.TestDeploymentDir.Replace("Mixed Platforms", "Release"), "RobloxApp.exe"));
            searchPaths.Add(Path.Combine(TestContext.TestDeploymentDir, "..\\..\\Win32\\Release\\RobloxApp.exe"));

            foreach (var p in searchPaths)
            {
                if (File.Exists(p))
                    return p;
            }

            // default.
            string flavor = "Release";
            try
            {
                using (var sr = new StreamReader(RobloxAppBuildTypeConfigFile))
                {
                    flavor = sr.ReadLine();
                }
            }
            catch (FileNotFoundException)
            {
            }
            return String.Format(RobloxAppDefaultFilenameFormat, flavor);
        }

        [TestInitialize()]
        public void TestInitialize() 
        {
            RobloxAppFilename = FindRobloxApp();

            proc.StartInfo.FileName = RobloxAppFilename;

            Process[] procs = System.Diagnostics.Process.GetProcessesByName("RobloxApp");
            foreach (Process p in procs)
            {
                try
                {
                    p.Kill();
                }
                catch
                {
                }
            }
            // register COM object.
            string testdir = TestContext.TestDeploymentDir;

            proc.StartInfo.Arguments = "/Register";
            proc.Start();

            proc.WaitForExit();

        }

        [TestCleanup()]
        public void TestCleanup() 
        {
            if (!proc.HasExited)
            {
                proc.Kill();
            }
        }

        const uint ID_VIEW_FULLSCREEN = 33012;
        const uint ID_VIEW_GAMELAYOUT = 33013;
        const uint ID_IDE_RUN = 32966;

        public enum RunMode
        {
            Static,
            Run,
            Visit
        };

        public class SecureWorkspace : RobloxLib.IWorkspace
        {
            RobloxLib.IWorkspace p;
            ScriptSigner scriptSigner;

            public SecureWorkspace(RobloxLib.IWorkspace ws)
            {
                p = ws;
                scriptSigner = new ScriptSigner();
            }
            #region IWorkspace Members

            public object[] ExecScript(string script, object arg1, object arg2, object arg3, object arg4)
            {
                return p.ExecScript(scriptSigner.SignScript(script), arg1, arg2, arg3, arg4);
            }


            public void Close()
            {
                p.Close();
            }

            public object[] ExecUrlScript(string url, object arg1, object arg2, object arg3, object arg4)
            {
                return p.ExecUrlScript(url, arg1, arg2, arg3, arg4);
            }

            public void Insert(string url)
            {
                p.Insert(url);
            }

            public void StartDrag(string url)
            {
                p.StartDrag(url);
            }

            public RobloxLib.Content Write()
            {
                return p.Write();
            }

            public RobloxLib.Content WriteSelection()
            {
                return p.WriteSelection();
            }

            #endregion

            #region IWorkspace Members


            public object[] GetPlayers()
            {
                throw new NotImplementedException();
            }

            public void JoinGame(string server, string port, string gameTicket)
            {
                throw new NotImplementedException();
            }

            public void ReportAbuse(int abuserId, string comment)
            {
                throw new NotImplementedException();
            }

            public void Save()
            {
                throw new NotImplementedException();
            }

            public void SaveUrl(string url)
            {
                throw new NotImplementedException();
            }

            #endregion
        };

        public void TestBasic(string settings, string place, RunMode runmode)
        {
            string testdir = TestContext.TestDeploymentDir;
            string settingsdir = Path.Combine(Path.Combine(testdir,"TestFiles\\"),settings);
            string placedir = Path.Combine(Path.Combine(testdir, "TestFiles\\"), place);
            placedir = placedir.Replace("\\", "\\\\"); // double escape, for LUA scripting.

            proc.StartInfo.Arguments = String.Format(RobloxAppArgFormat, settingsdir);
            proc.Start();

            proc.WaitForInputIdle();

            HandleRef hr = new HandleRef(proc, proc.MainWindowHandle);

            PlatformInvokeUSER32.PostMessage(hr, PlatformInvokeUSER32.WM_COMMAND, new IntPtr(ID_VIEW_GAMELAYOUT), IntPtr.Zero);
            PlatformInvokeUSER32.ShowWindow(hr, PlatformInvokeUSER32.SW_SHOW);
            PlatformInvokeUSER32.PostMessage(hr, PlatformInvokeUSER32.WM_COMMAND, new IntPtr(ID_VIEW_FULLSCREEN), IntPtr.Zero);
            //PlatformInvokeUSER32.PostMessage(hr, PlatformInvokeUSER32.WM_ACTIVATE, new IntPtr(PlatformInvokeUSER32.WA_ACTIVE), IntPtr.Zero);

            RobloxLib.IApp app = new RobloxLib.AppClass();

            string statusmessage = "";

            RobloxLib.IWorkspace workspace = null;
            try
            {
                workspace = new SecureWorkspace(app.CreateGame("44340105256"));
            }
            catch (COMException comex)
            {
                if (comex.ErrorCode == unchecked ((int)0x80010105) /*RPC_E_SERVERFAULT*/)
                {
                    //this is what you get when "Graphics fail to initialize".
                    //Unfortunately, it is rather difficult to know exactly at this point. there is too much catching/re-throwing.
                    TestContext.WriteLine("Graphics failed to initialize (maybe)");
                    return;
                }
                else
                {
                    throw;
                }
            }



            object[] ret = (object[])workspace.ExecScript("return settings().Diagnostics.RobloxVersion", null, null, null, null);

            string rbxversion = (string)(ret[0]);
            string testversion = Assembly.GetExecutingAssembly().GetName().Version.ToString();


            ret = (object[])workspace.ExecScript("game:load('" + placedir + "')", null, null, null, null);

            if (runmode == RunMode.Visit)
            {
                // throttling only turns on with a character present.
                workspace.ExecUrlScript("http://www.roblox.com/game/visit.ashx", null, null, null, null);
            }
            else if (runmode == RunMode.Run)
            {
                PlatformInvokeUSER32.PostMessage(hr, PlatformInvokeUSER32.WM_COMMAND, new IntPtr(ID_IDE_RUN), IntPtr.Zero);
            }

            bool bHasFrameRateManager = (bool)(((object[])workspace.ExecScript("if stats():FindFirstChild(\"FrameRateManager\") then return true else return false end", null, null, null, null))[0]) ;
            int frameRateManagerBuckets = 0;  
            int secondsElapsed = 0;
            double numFramesElapsed = 0;
            do
            {
                ret = (object[])workspace.ExecScript("return stats().Render:FindFirstChild(\"3D CPU Total\"):getTimesForFrames(200)", null, null, null, null);
                numFramesElapsed = (double)ret[2];
                Thread.Sleep(1000);

                //walk back and forth every 30 seconds.
                if ((secondsElapsed++ % 10) == 0 && runmode == RunMode.Visit)
                {
                    
                    int posz = ((secondsElapsed / 10) % 3 == 0) ? 100 : 40;
                    int posx = ((secondsElapsed / 10) % 3 == 1) ? 40 : 0;
                    workspace.ExecScript("game.Workspace.Player.Humanoid:MoveTo(Vector3.new(" + posx.ToString() + ",0," + posz.ToString() + "), game.Workspace:FindFirstChild('Base'))", null, null, null, null); // throttling only turns on with a character present.
                }

                // framerate manager will turn on when # buckets exceeds 30.
                if (bHasFrameRateManager && runmode == RunMode.Visit)
                {
                    ret = (object[])workspace.ExecScript(
                        "stats().FrameRateManager:getValue()" +
                        "return stats().FrameRateManager.Buckets:getValue()" 
                        , null, null, null, null);
                    frameRateManagerBuckets = (int)((double)ret[0]);
                }
                else
                {
                    frameRateManagerBuckets = 30;
                }

            } while ( (frameRateManagerBuckets < 30) ||
                (numFramesElapsed < 200 && secondsElapsed < 30));

            if (runmode == RunMode.Visit)
            {
                // get the viewpoint we want to measure.
                workspace.ExecScript("game.Workspace.CurrentCamera.CameraSubject = game.Workspace:FindFirstChild('Roof')", null, null, null, null);
            }

            // idle untill we get 100 frames of stable blockCounts from the FrameRateManager
            secondsElapsed = 0;
            ret[0] = 0.0;
            while(bHasFrameRateManager && ((double)ret[0] < 100))
            {
                ret = (object[])workspace.ExecScript("return stats().FrameRateManager.StableFramesCounter:getValue()", null, null, null, null);
                Thread.Sleep(1000);
                if (secondsElapsed++ >= 120)
                {
                    statusmessage += "Unable to reach FrameRateManager stability. ";
                    break;
                }
            };

            // now do actual test.
            int numFrames = 100;
            ret = (object[])workspace.ExecScript("return stats().Render:FindFirstChild(\"3D CPU Total\"):getTimesForFrames(" + numFrames.ToString() + ")", null, null, null, null);

            double walltime = (double)ret[0];
            double sampletime = (double)ret[1];
            double frames = (double)ret[2];

            int visibleBlockCount = -1;
            double constant = 0;
            double visibleBlockCountEffect = 0;
            //double estimatedFrameTime = 0;

            if (bHasFrameRateManager)
            {
                // refresh values.
                ret = (object[])workspace.ExecScript(
                    "stats().FrameRateManager:getValue()"
                    , null, null, null, null);

                ret = (object[])workspace.ExecScript("return stats().FrameRateManager.X.visibleBlockCount:getValue()", null, null, null, null);
                visibleBlockCount = (int)((double)ret[0]);

                ret = (object[])workspace.ExecScript("return stats().FrameRateManager.EstimatedFrameTime:getValue() - stats().FrameRateManager.X.visibleBlockCount:getValue() * stats().FrameRateManager.Beta.visibleBlockCount:getValue()", null, null, null, null);
                constant = (double)ret[0];
                ret = (object[])workspace.ExecScript("return stats().FrameRateManager.Beta.visibleBlockCount:getValue()", null, null, null, null);
                visibleBlockCountEffect = (double)ret[0];

            }

            //take screenshot.

            Bitmap screen = CaptureScreen.GetDesktopImage();
            screen.Save(Path.Combine(TestOutputDir, Environment.MachineName + ".PNG"));

            TestContext.WriteLine("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\t{7}", testversion, rbxversion, frames / sampletime, walltime / frames, constant, visibleBlockCountEffect, visibleBlockCount, statusmessage);

            //app.Quit();
            bool result = proc.CloseMainWindow();

            proc.WaitForExit(30000);

        }

        [TestMethod]
        [DeploymentItem("TestFiles\\StarterHappyHome.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestStarterHappyHome()
        {
            TestBasic("SettingsOgreD3DRenderer.xml", "StarterHappyHome.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\house.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicHouseOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer.xml", "house.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\house_slate.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicHouseSlateOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "house_slate.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\house.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicHousePlasticOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "house.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\house_wood.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicHouseWoodOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "house_wood.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\house_rust.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicHouseRustOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "house_rust.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\house_concrete.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicHouseConcreteOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "house_concrete.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\walltester1.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void WallTester1OgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "walltester1.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\walltester2.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void WallTester2OgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "walltester2.rbxl", RunMode.Static);
        }


        [TestMethod]
        [DeploymentItem("TestFiles\\Village.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void VillageOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "Village.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\Town2.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void Town2OgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "Town2.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\LAHouse.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void LAHouseOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "LAHouse.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\Place1.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void Place1OgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "Place1.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\ShipHotel.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void ShipHotelOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "ShipHotel.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\house_dplate.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicHouseDPlateOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "house_dplate.rbxl", RunMode.Static);
        }



        [TestMethod]
        [DeploymentItem("TestFiles\\house_aluminum.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicHouseAluminumOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "house_aluminum.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\house_grass.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicHouseGrassOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "house_grass.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\house_ice.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer_NoFrameLimit.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicHouseIceOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer_NoFrameLimit.xml", "house_ice.rbxl", RunMode.Static);
        }


        
        
        
        
        
        
        
        [TestMethod]
        [DeploymentItem("TestFiles\\house.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreGLRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicHouseOgreGL()
        {
            TestBasic("SettingsOgreGLRenderer.xml", "house.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\house.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsG3DGLRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicHouseG3DGL()
        {
            TestBasic("SettingsG3DGLRenderer.xml", "house.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\house.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsDefaultRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicHouseDefault()
        {
            TestBasic("SettingsDefaultRenderer.xml", "house.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\Hotels.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestFRMHotelsOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer.xml", "Hotels.rbxl", RunMode.Visit);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\Hotels.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicHotelsOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer.xml", "Hotels.rbxl", RunMode.Static);
        }
        [TestMethod]
        [DeploymentItem("TestFiles\\CrossroadsPlastic.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\LowOgreD3DBench.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void RegressCrossroadsOgreD3D()
        {
            TestBasic("LowOgreD3DBench.xml", "CrossroadsPlastic.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\CrossroadsPlastic.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\LowG3DGLBench.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void RegressCrossroadsG3DGL()
        {
            TestBasic("LowG3DGLBench.xml", "CrossroadsPlastic.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\CrossroadsPlastic.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestCrossroadsPlasticOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer.xml", "CrossroadsPlastic.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\CrossroadsWood.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestCrossroadsWoodOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer.xml", "CrossroadsWood.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\CrossroadsSlate.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestCrossroadsSlateOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer.xml", "CrossroadsSlate.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\CrossroadsMixed.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestCrossroadsMixedOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer.xml", "CrossroadsMixed.rbxl", RunMode.Run);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\CrossroadsMixed.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreGLRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestCrossroadsMixedOgreGL()
        {
            TestBasic("SettingsOgreGLRenderer.xml", "CrossroadsMixed.rbxl", RunMode.Run);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\WoodenHouse.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicWoodenHouseOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer.xml", "WoodenHouse.rbxl", RunMode.Static);
        }


        [TestMethod]
        [DeploymentItem("TestFiles\\TrussStress.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsOgreD3DRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicTrussOgreD3D()
        {
            TestBasic("SettingsOgreD3DRenderer.xml", "TrussStress.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\TrussStress.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\SettingsG3DGLRenderer.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void TestBasicTrussG3DGL()
        {
            TestBasic("SettingsG3DGLRenderer.xml", "TrussStress.rbxl", RunMode.Static);
        }

        [TestMethod]
        [DeploymentItem("TestFiles\\8TowersHardMixed.rbxl", "TestFiles\\")]
        [DeploymentItem("TestFiles\\OgreD3DXBricks.xml", "TestFiles\\")]
        [HeaderString("testver\trbxver\tfps\tgfxtime_ms\tfrm_constant\tfrm_blockcost\tvisibleBlocks\tstatus")]
        public void Test8TowersHardMixed()
        {
            TestBasic("OgreD3DXBricks.xml", "8TowersHardMixed.rbxl", RunMode.Run);
        }
    }
}

