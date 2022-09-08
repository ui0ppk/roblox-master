using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;

namespace IncludeChecker
{
    class Node
    {
        public string Name { get; set; }
        private Node(string name)
        {
            Name = name.ToLower();
            allNodes[Name] = this;
        }

        public static Node Get(string name)
        {
            name = name.ToLower();
            return allNodes.ContainsKey(name) ? allNodes[name] : new Node(name);
        }

        public void AddInclude(string name)
        {
            name = name.ToLower();
            includes.Add(Get(name));
        }

        public bool MayInclude(string name)
        {
            return MayInclude(name, new HashSet<Node>());
        }

        bool MayInclude(string name, HashSet<Node> visited)
        {
            if (visited.Contains(this))
                return false;
            visited.Add(this);

            name = name.ToLower();

            if (!allNodes.ContainsKey(name))
                return false;

            if (includes.Contains(allNodes[name]))
                return true;

            foreach (var node in includes)
                if (node.MayInclude(name, visited))
                    return true;

            return false;
        }

        static Dictionary<string, Node> allNodes = new Dictionary<string, Node>();
        HashSet<Node> includes = new HashSet<Node>();
    }
    class Program
    {
        static int errorCount = 0;
        static int checkCount = 0;
        static void DefineIncludes()
        {
            Node.Get("V8DataModel").AddInclude("V8World");  
            Node.Get("V8DataModel").AddInclude("V8Tree");
            Node.Get("V8DataModel").AddInclude("Network");
            Node.Get("V8DataModel").AddInclude("gui");
            Node.Get("V8DataModel").AddInclude("util");
            Node.Get("V8DataModel").AddInclude("Tool");
            Node.Get("V8DataModel").AddInclude("Humanoid");
            Node.Get("V8DataModel").AddInclude("Script");

            Node.Get("Darwin").AddInclude("V8DataModel");

            Node.Get("src").AddInclude("script");

            Node.Get("Humanoid").AddInclude("V8DataModel");
            Node.Get("Lua").AddInclude("V8DataModel");

            Node.Get("Script").AddInclude("LUa");

            Node.Get("V8Tree").AddInclude("Reflection");
            Node.Get("V8Tree").AddInclude("RBX/Intrusive");
            
            Node.Get("Reflection").AddInclude("Security");
            Node.Get("Reflection").AddInclude("util");
            Node.Get("Reflection").AddInclude("v8xml");

            Node.Get("Darwin").AddInclude("rbx");
            Node.Get("Darwin").AddInclude("util");

            Node.Get("Security").AddInclude("rbx");

            Node.Get("Tool").AddInclude("V8DataModel");

            Node.Get("V8Kernel").AddInclude("util");

            Node.Get("V8World").AddInclude("V8Kernel");
            Node.Get("V8World").AddInclude("rbx");

            Node.Get("V8Xml").AddInclude("Util");
            Node.Get("V8Xml").AddInclude("Reflection");

            Node.Get("util").AddInclude("rbx");
            Node.Get("util").AddInclude("rbxg3d");

            Node.Get("gui").AddInclude("Util");
            Node.Get("gui").AddInclude("AppDraw");
            Node.Get("gui").AddInclude("GfxBase");
            Node.Get("gui").AddInclude("V8DataModel");
        }
        static void Main(string[] args)
        {
            DefineIncludes();
            Console.WriteLine("Go!");
            DirectoryInfo app = new DirectoryInfo(Directory.GetCurrentDirectory());
            //Check(app.Parent.Parent.Parent.GetDirectories("Base")[0]);
            Check(app.Parent.Parent.Parent.GetDirectories("App")[0]);
            Console.WriteLine(errorCount + " errors out of " + checkCount);
        }

        private static void Check(DirectoryInfo dir)
        {
            foreach (var file in dir.GetFiles())
                Check(file);
            foreach (var d in dir.GetDirectories())
                Check(d);
        }

        private static void Check(FileInfo file)
        {
            if (!IsCFile(file))
                return;

            var paths = GetIncludePaths(file);

            foreach (var path in paths)
                Check(file, path);

        }

        private static void Check(FileInfo file, string includePath)
        {
            checkCount++;

            // Implement simple rules
            if (includePath == "")
                return;

            if (includePath.ToLower() == "g3d")
                return;
            if (includePath.ToLower() == "corefoundation")
                return;
            if (includePath.StartsWith("boost"))
                return;
            if (includePath.StartsWith("mach-o"))
                return;
            if (includePath.StartsWith("readline"))
                return;
            if (includePath.Contains("fmod"))
                return;
            if (includePath.StartsWith("ext"))
                return;
            if (includePath.StartsWith("openssl"))
                return;
            if (includePath.ToLower() == "rbx/darwin")
                return;
            if (includePath.StartsWith("sys"))
                return;
            if (includePath.StartsWith("tbb"))
                return;

            string dir = file.Directory.Name;

            if (dir.ToLower() == includePath.ToLower())
                return;

            Node node = Node.Get(file.Directory.Name);
            if (!node.MayInclude(includePath))
            {
                Console.WriteLine(file.Directory.Name + "/" + file.Name + " " + includePath);
                errorCount++;
            }
        }

        private static HashSet<string> GetIncludePaths(FileInfo file)
        {
            var paths = new HashSet<string>();
            using (StreamReader reader = new StreamReader(file.FullName))
            {
                while (!reader.EndOfStream)
                {
                    string line = reader.ReadLine().ToLower();
                    TryMatch(paths, Regex.Match(line, @"#include ""(.+)"""));
                    TryMatch(paths, Regex.Match(line, @"#include <(.+)>"));
                }
            }
            return paths;
        }

        private static void TryMatch(HashSet<string> paths, Match match)
        {
            if (match.Groups.Count == 2)
            {
                string path = match.Groups[1].Value;
                if (path.StartsWith("./"))
                    path = path.Substring(2);
                int index = path.LastIndexOf('/');
                if (index > 0)
                    paths.Add(path.Substring(0, index));
                else
                    paths.Add("");
            }
        }

        private static bool IsCFile(FileInfo file)
        {
            if (file.Extension == ".cpp")
                return true;
            if (file.Extension == ".c")
                return true;
            if (file.Extension == ".h")
                return true;
            if (file.Extension == ".hpp")
                return true;
            if (file.Extension == ".inl")
                return true;
            if (file.Extension == ".cxx")
                return true;
            return false;
        }
    }
}
