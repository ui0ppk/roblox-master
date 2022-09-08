using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.IO;

namespace Roblox.Test
{
    [Serializable]
    class RemoteTestShimImpl : MarshalByRefObject, IRemoteTestShim
    {
        private class MyTestContext : TestContext
        {
            Dictionary<string, object> properties = new Dictionary<string, object>();
            TextWriter textwriter;

            public MyTestContext(string TestDeploymentDir, TextWriter tw)
            {
                textwriter = tw;
                properties["TestDeploymentDir"] = TestDeploymentDir;
            }

            public override void AddResultFile(string fileName)
            {
                throw new NotImplementedException();
            }

            public override void BeginTimer(string timerName)
            {
                throw new NotImplementedException();
            }

            public override System.Data.Common.DbConnection DataConnection
            {
                get { throw new NotImplementedException(); }
            }

            public override System.Data.DataRow DataRow
            {
                get { throw new NotImplementedException(); }
            }

            public override void EndTimer(string timerName)
            {
                throw new NotImplementedException();
            }

            public override System.Collections.IDictionary Properties
            {
                get { return properties; }
            }

            public override void WriteLine(string format, params object[] args)
            {
                textwriter.WriteLine(format, args);
            }
        }



        public void RunTest(string AssemblyName, string MethodName, string DeploymentDir, TextWriter tw)
        {
            Type ClassType = null;
            MemberInfo memberCleanup = null;
            MemberInfo memberInitialize = null;
            MemberInfo memberTestMethod = null;

            Assembly ass = Assembly.Load(AssemblyName);

            foreach (Type type in ass.GetTypes())
            {
                object[] obj = type.GetCustomAttributes(typeof(TestClassAttribute), false);
                if (obj.Length > 0)
                {
                    ClassType = type;
                    memberCleanup = null;
                    memberInitialize = null;
                    memberTestMethod = null;

                    foreach (MemberInfo memberinfo in ClassType.GetMembers())
                    {
                        obj = memberinfo.GetCustomAttributes(typeof(TestMethodAttribute), false);
                        if (obj.Length > 0 && memberinfo.Name == MethodName)
                        {
                            memberTestMethod = memberinfo;
                            continue;
                        }

                        obj = memberinfo.GetCustomAttributes(typeof(TestCleanupAttribute), false);
                        if (obj.Length > 0)
                        {
                            memberCleanup = memberinfo;
                            continue;
                        }

                        obj = memberinfo.GetCustomAttributes(typeof(TestInitializeAttribute), false);
                        if (obj.Length > 0)
                        {
                            memberInitialize = memberinfo;
                            continue;
                        }
                    }
                    if (memberTestMethod != null)
                    {
                        // found it!
                        break;
                    }
                }

            }

            if (memberTestMethod != null)
            {
                object instance = ass.CreateInstance(ClassType.FullName);

                if (instance != null)
                {
                    //setup TestContext
                    PropertyInfo propertyinfo = ClassType.GetProperty("TestContext");
                    if (propertyinfo != null)
                    {
                        propertyinfo.SetValue(instance, new MyTestContext(DeploymentDir, tw), null);
                    }
                }

                if (memberInitialize != null)
                {
                    ClassType.InvokeMember(memberInitialize.Name, BindingFlags.InvokeMethod, null, instance, new object[] { });
                }
                try
                {
                    ClassType.InvokeMember(memberTestMethod.Name, BindingFlags.InvokeMethod, null, instance, new object[] { });
                }
                finally
                {
                    if (memberCleanup != null)
                    {
                        ClassType.InvokeMember(memberCleanup.Name, BindingFlags.InvokeMethod, null, instance, new object[] { });
                    }
                }


            }
            else
            {
                throw new Exception(String.Format("Test method {0} not found!", MethodName));
            }
           
        }

        public bool DoesTestExist(string AssemblyName, string MethodName)
        {
            throw new NotImplementedException();
        }

        public string GetTestHeader(string AssemblyName, string MethodName)
        {
            Type ClassType = null;
            Assembly ass = Assembly.Load(AssemblyName);

            foreach (Type type in ass.GetTypes())
            {
                object[] obj = type.GetCustomAttributes(typeof(TestClassAttribute), false);
                if (obj.Length > 0)
                {
                    ClassType = type;

                    foreach (MemberInfo memberinfo in ClassType.GetMembers())
                    {
                        obj = memberinfo.GetCustomAttributes(typeof(HeaderStringAttribute), false);
                        if (obj.Length > 0 && memberinfo.Name == MethodName)
                        {
                            HeaderStringAttribute attrib = (HeaderStringAttribute)(obj[0]);
                            return attrib.Header;
                        }
                    }
                }

            }

            // no header. No biggie.
            return "";
        }
    }
}
