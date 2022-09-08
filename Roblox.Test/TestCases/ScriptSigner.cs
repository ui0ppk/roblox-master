using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Security.Cryptography;
using System.IO;

namespace Roblox.Test
{
    public class ScriptSigner
    {
        public ScriptSigner()
        {
            string path = "Server/RobloxWebSite/Game/rbxPrivate.blob";
            int upcount = 0;
            while (!System.IO.File.Exists(path))
            {
                path = "../" + path;

                if (upcount++ >= 10)
                {
                    throw new FileNotFoundException("Cannot find Server/RobloxWebSite/Game/rbxPrivate.blob in any of the parent directories of the current dir");
                }
            }
            blob = System.IO.File.ReadAllBytes(path);
        }
        private byte[] blob;

        public string SignScript(string script)
        {
            return "%" + SharedSecurityNotary.CreateSignature(script, blob) + "%" + script;
        }
    }
}
