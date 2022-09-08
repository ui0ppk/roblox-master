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
using Microsoft.Ccr.Core;
using Roblox.Common;

namespace RCCService.Test
{
    [TestClass]
    public class StressSoapTest
    {

        static Uri serviceUri;
        static Job job;

        [TestInitialize()]
        public void MyTestInitialize()
        {
            System.Net.ServicePointManager.DefaultConnectionLimit = 8;

            serviceUri = RCCLauncher.FindOrStartRCCService();

            using (RCCServiceSoap service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port))
            {
                job = new Job();
                job.category = 1;
                job.cores = 1;
                job.expirationInSeconds = TimeSpan.FromSeconds(60).TotalSeconds;
                job.id = Guid.NewGuid().ToString();

                ScriptExecution script = Lua.NewScript("Madrak's Place",
                    "local ns = game:GetService('NetworkServer') \r\n ns:Start(6000, 5) \r\n game:GetService(\"RunService\"):Run()");

                service.OpenJobEx(job, script);
            }
        }

        [TestCleanup()]
        public void MyTestCleanup()
        {
            using (RCCServiceSoap service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port))
            {
                service.CloseJob(job.id);
            }
        }
        [TestMethod]
        public void TestAsyncExecuteExRenew()
        {
            done = 0;
            failure = 0;
            for (int i = 0; i < 25; i++)
            {
                Roblox.CcrService.Singleton.SpawnIterator(AsyncExecuteEx);
            }
            while (done != 25)
            {
                Assert.IsTrue(failure == 0);
                System.Threading.Thread.Sleep(10);
            }
        }
        [TestMethod]
        public void TestAsyncExecuteExWithOpenJobRenew()
        {
            done = 0;
            failure = 0;
            for (int i = 0; i < 100; i++)
            {
                Roblox.CcrService.Singleton.SpawnIterator(AsyncExecuteEx);
                if(i%5 == 0)
                    Roblox.CcrService.Singleton.SpawnIterator(AsyncLongOpenJob);
            }
            while (done != 100)
            {
                Assert.IsTrue(failure == 0,String.Format("{0} Exceptions", failure));
                System.Threading.Thread.Sleep(10);
            }
        }
        [TestMethod]
        public void TestSyncExecuteEx()
        {
            for (int i = 0; i < 1000; i++)
            {
                using (RCCServiceSoap service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port))
                {
                    ScriptExecution script = Lua.NewScript("Monitor",
                    "return 'foo'");

                    var result = service.ExecuteEx(job.id, script);
                    Assert.IsTrue(result.Length == 1);
                    Assert.IsTrue(result[0].type == LuaType.LUA_TSTRING);
                    Assert.IsTrue(result[0].value == "foo");
                }
            }
        }

        [TestMethod]
        public void TestSyncExecuteExRenew()
        {
            for (int i = 0; i < 1000; i++)
            {
                using (RCCServiceSoap service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port))
                {
                    ScriptExecution script = Lua.NewScript("Monitor",
                        "return 'foo'");

                    var result = service.ExecuteEx(job.id, script);
                    Assert.IsTrue(result.Length == 1);
                    Assert.IsTrue(result[0].type == LuaType.LUA_TSTRING);
                    Assert.IsTrue(result[0].value == "foo");

                    service.RenewLease(job.id, TimeSpan.FromMinutes(15).TotalSeconds);
                }
            }
        }
        private IEnumerator<ITask> AsyncLongOpenJob()
        {
            Job job = new Job();
            job.category = 1;
            job.cores = 1;
            job.expirationInSeconds = TimeSpan.FromSeconds(60).TotalSeconds;
            job.id = Guid.NewGuid().ToString();

            ScriptExecution script = Lua.NewScript("Madrak's Place",
                "i = 0 while i < 1000000 do i = i + 1 end");

            var result = new PortSet<Roblox.Grid.Rcc.LuaValue[], Exception>();
            var service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port);
            try
            {
                AsyncHelper.Call(
                    service.BeginOpenJobEx,
                    job,
                    script,
                    service.EndOpenJobEx,
                    result,
                    TimeSpan.FromSeconds(60),
                    service.Dispose);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                yield break;
            }
            yield return (Choice)result;

            var result2 = new SuccessFailurePort();
            service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port);
            try
            {
                AsyncHelper.Call(
                    service.BeginCloseJob,
                    job.id,
                    service.EndCloseJob,
                    result2,
                    TimeSpan.FromSeconds(10),
                    service.Dispose);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                yield break;
            }
            yield return (Choice)result2;

        }

        volatile static int done;
        volatile static int failure;
        private IEnumerator<ITask> AsyncExecuteEx()
        {
            try
            {
                for (int i = 0; i < 5; ++i)
                {
                    ScriptExecution script = Lua.NewScript("Monitor",
                        "return 'foo'");

                    var result = new PortSet<Roblox.Grid.Rcc.LuaValue[], Exception>();
                    var service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port);
                    try
                    {
                        AsyncHelper.Call(
                            service.BeginExecuteEx,
                            job.id,
                            script,
                            service.EndExecuteEx,
                            result,
                            TimeSpan.FromSeconds(30),
                            () => { service.Abort(); service.Dispose(); }
                        );
                    }
                    catch (Exception e)
                    {
                        Console.WriteLine(e.Message);
                        Interlocked.Increment(ref failure);
                        yield break;
                    }
                    yield return (Choice)result;

                    Exception ex = result.Test<Exception>();
                    if (ex != null)
                    {
                        Console.WriteLine(ex.Message);
                        System.Threading.Interlocked.Increment(ref failure);
                        yield break;
                    }
                    Roblox.Grid.Rcc.LuaValue[] luaResult = result.Test<Roblox.Grid.Rcc.LuaValue[]>();
                    //if (luaResult.Length != 1 || luaResult[0].type != LuaType.LUA_TSTRING || luaResult[0].value != "foo")
                    //{
                    //    System.Threading.Interlocked.Increment(ref failure);
                    //    yield break;
                    //}

                    var result2 = new PortSet<double, Exception>();
                    service = Roblox.Grid.Common.GridServiceUtils.GetService(serviceUri.Host, serviceUri.Port);
                    try
                    {
                        AsyncHelper.Call(
                            service.BeginRenewLease,
                            job.id,
                            TimeSpan.FromMinutes(15).TotalSeconds,
                            service.EndRenewLease,
                            result2,
                            TimeSpan.FromSeconds(30),
                            service.Dispose
                        );
                    }
                    catch (Exception e)
                    {
                        Console.WriteLine(e.Message);
                        Interlocked.Increment(ref failure);
                        yield break;
                    }
                    yield return (Choice)result2;

                    ex = result2.Test<Exception>();
                    if (ex != null)
                    {
                        Console.WriteLine(ex.Message);
                        System.Threading.Interlocked.Increment(ref failure);
                        yield break;
                    }
                    double dResult = result2.Test<double>();
                    if (dResult != 0)
                    {
                        System.Threading.Interlocked.Increment(ref failure);
                        yield break;
                    }
                }
            }
            finally
            {
                System.Threading.Interlocked.Increment(ref done);
            }
        }
    }
}
