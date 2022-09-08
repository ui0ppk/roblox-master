using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ScriptSigner
{
    class Program
    {
        static void Main(string[] args)
        {
            string path = "Server/RobloxWebSite/Game/rbxPrivate.blob";
            while (!System.IO.File.Exists(path))
            {
                path = "../" + path;
            }
            byte[] blob = System.IO.File.ReadAllBytes(path);

            foreach (string s in args)
            {
                string script = System.IO.File.ReadAllText(s);
                System.IO.File.WriteAllText(s + ".sgnd", SharedSecurityNotary.SignScript(script, blob));
            }
        }
    }
}
