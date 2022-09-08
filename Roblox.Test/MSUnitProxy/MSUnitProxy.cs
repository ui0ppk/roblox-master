using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.IO;

namespace Roblox.Test
{
    [TestClass]
    [Serializable]
    public class MSUnitProxy
    {
        private TestContext testContextInstance;

        private readonly string[] machines;
        private readonly string[] coremachines;

        public MSUnitProxy()
        {
            machines = new string[]
            {
                "xyz",
                "monica",
                "imagestaging",
                "homebrew",
                "nforce",
                "guantanamo",
                "cigar",
                "acer",
                "foster",
                "amnesty",
                "chad",
                "redskins",
                "arnold",
                "helen",
                "gateway-quad"
            };

            coremachines = new string[]
            {
                "xyz",
                "monica",
                "imagestaging",
                "guantanamo",
                "gateway-quad",
                "acer",
                "redskins",
                "chad"
            };

        }

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


        [TestInitialize()]
        public void TestInitialize()
        {
        }

        [TestCleanup()]
        public void TestCleanup()
        {
        }

        [TestMethod]
        public void TestPing()
        {
            TextWriter tw = new StringWriter();
            var result = Program.Ping(tw, machines);
            tw.Flush();
            TestContext.WriteLine("{0}", tw.ToString());
            TestContext.WriteLine("Success: {0} of {1}", result.SuccessCount, result.FailureCount + result.SuccessCount);
            Assert.IsTrue(result.SuccessCount > result.FailureCount, "Success ratio");
        }

        [TestMethod]
        public void RemoteBasicHouseOgreD3D()
        {
            TextWriter tw = new StringWriter();
            string binroot = Environment.GetEnvironmentVariable("PublishedTestBinRoot");
            var result = Program.Remote(binroot, tw, "TestBasicHouseOgreD3D", coremachines);
            tw.Flush();
            TestContext.WriteLine("{0}", tw.ToString());
            TestContext.WriteLine("Success: {0} of {1}", result.SuccessCount, result.FailureCount + result.SuccessCount);
            Assert.IsTrue(result.SuccessCount > result.FailureCount, "Success ratio");


            var sr = new StringReader(tw.ToString());
            Program.PublishResultsToDB(sr);
        }

        [TestMethod]
        public void RemoteRegressCrossroadsOgreD3D()
        {
            TextWriter tw = new StringWriter();
            string binroot = Environment.GetEnvironmentVariable("PublishedTestBinRoot");
            var result = Program.Remote(binroot, tw, "RegressCrossroadsOgreD3D", coremachines);
            tw.Flush();
            TestContext.WriteLine("{0}", tw.ToString());
            TestContext.WriteLine("Success: {0} of {1}", result.SuccessCount, result.FailureCount + result.SuccessCount);
            Assert.IsTrue(result.SuccessCount > result.FailureCount, "Success ratio");


            var sr = new StringReader(tw.ToString());
            Program.PublishResultsToDB(sr);
        }

        [TestMethod]
        public void RemoteRegressCrossroadsG3DGL()
        {
            TextWriter tw = new StringWriter();
            string binroot = Environment.GetEnvironmentVariable("PublishedTestBinRoot");
            var result = Program.Remote(binroot, tw, "RegressCrossroadsG3DGL", coremachines);
            tw.Flush();
            TestContext.WriteLine("{0}", tw.ToString());
            TestContext.WriteLine("Success: {0} of {1}", result.SuccessCount, result.FailureCount + result.SuccessCount);
            Assert.IsTrue(result.SuccessCount > result.FailureCount, "Success ratio");


            var sr = new StringReader(tw.ToString());
            Program.PublishResultsToDB(sr);
        }

    }
}
