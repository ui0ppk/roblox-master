using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace Roblox.Test
{
    public interface IRemoteTestShim
    {
        bool DoesTestExist(string AssemblyName, string MethodName);
        void RunTest(string AssemblyName, string MethodName, string DeploymentDir, TextWriter tw);
        string GetTestHeader(string AssemblyName, string MethodName);
    }

    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
    public class HeaderStringAttribute : Attribute
    {
        private string header;
        public HeaderStringAttribute(string header) { this.header = header; }
        public string Header { get { return header; } }
    }

}
