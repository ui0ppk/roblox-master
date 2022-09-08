using System;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.IO;
using System.Xml;

namespace Roblox.RccServiceArbiter
{
    class ClientSettings
    {
        static private string BuildSettingsUrl(string baseUrl, string group)
        {
            UriBuilder uriBuild = new UriBuilder(baseUrl);
            string host = uriBuild.Host;
            host = host.Replace("www.", "");

            return String.Format("https://clientsettings.api.{0}/Setting/QuietGet/{1}/?apiKey=D6925E56-BFB9-4908-AAA2-A5B1EC4B2D79", host, group);;
        }

        static public string Fetch(string group)
        {
            string baseUrl = Utils.GetBaseURL();
            if (baseUrl.Length == 0)
            {
                // You didn't set BaseURL before loading settings!
                Console.WriteLine("Failed to get base url");
                return null;
            }

            string settingsData = "";
            try
            {
                string url = BuildSettingsUrl(baseUrl, group);
                
                HttpWebRequest request = (HttpWebRequest)WebRequest.Create(url);
                
                HttpWebResponse response = (HttpWebResponse)request.GetResponse();
                if (response.ContentLength > 0)
                {
                    StreamReader readStream = new StreamReader(response.GetResponseStream(), System.Text.Encoding.GetEncoding("utf-8"));
                    settingsData = readStream.ReadToEnd();
                }
                response.Close();
            }
            catch (Exception exp)
            {
                Console.WriteLine(exp.Message);
                settingsData = null;
            }

            return settingsData;
        }

        static public string FetchThreadPoolConfig()
        {
            string jsonData = Fetch("Arbiter");

            if (jsonData != null)
            {
                Dictionary<string, object> json = new System.Web.Script.Serialization.JavaScriptSerializer().Deserialize<Dictionary<string, object>>(jsonData);

                object value;
                if (json.TryGetValue("ThreadPoolConfig", out value))
                {
                    if (value != null)
                        return value.ToString();
                }
            }

            return null;
        }
    };

    class Utils
    {
        static private string baseUrl = String.Empty;

        static public string GetBaseURL()
        {
            if (baseUrl.Length == 0)
            {
                System.Reflection.Assembly assembly = System.Reflection.Assembly.GetExecutingAssembly();
                string fullProcessPath = assembly.Location;
                string settingsFilePath = System.IO.Path.GetDirectoryName(fullProcessPath) + System.IO.Path.DirectorySeparatorChar + Properties.Settings.Default.AppSettingPath;

                XmlDocument xml = new XmlDocument();
                xml.Load(settingsFilePath);
                XmlNodeList nodes = xml.GetElementsByTagName("BaseUrl");
                baseUrl = nodes[0].InnerText;
            }
            return baseUrl;
        }
    }
}
