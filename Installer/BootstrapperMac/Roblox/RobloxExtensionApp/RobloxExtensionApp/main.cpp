//
//  main.cpp
//  RobloxExtensionApp
//
//  Created by Ganesh Agrawal on 11/20/13.
//  Copyright (c) 2013 Roblox. All rights reserved.
//

#include <iostream>
#include <map>

typedef std::map<std::string, std::string> Parameters;

bool match(char x, char y)
{
    return x == y;
}

Parameters parametrizeJson(std::string& jsonIn)
{
    Parameters params;
    
    //balanceString = {"PlaceId": "1818", "LaunchModeInPlayer": "True", "AuthTicket": "abcdefg"}
    std::string balanceString = jsonIn;
    std::string token = "{";
    while (balanceString.find(token) != -1)
    {
        //balanceString = "PlaceId": "1818", "LaunchModeInPlayer": "True", "AuthTicket": "abcdefg"}
        balanceString = balanceString.substr(balanceString.find(token)+token.length());
        balanceString = balanceString.substr(0, balanceString.find("}"));
        balanceString = balanceString.append(",");

        token = ",";
        if(balanceString.find(token) != -1)
        {
            //keyVal = "PlaceId": "1818"
            std::string keyVal = balanceString.substr(0, balanceString.find(token));

            //balanceString = , "LaunchModeInPlayer": "True", "AuthTicket": "abcdefg"}
            balanceString = balanceString.substr(keyVal.length());
            token = "\"";
            
            //key = PlaceId
            std::string key, val;
            key = keyVal.substr(0, keyVal.find(":"));
            key.erase(std::remove_if(key.begin(),
                                     key.end(),
                                     [](char x){return match(x, '\"');}),
                      key.end());
            
            //val = 1818
            val = keyVal.substr(keyVal.find(":") + 1);
            val.erase(std::remove_if(val.begin(),
                                     val.end(),
                                     [](char x){return match(x, '\"');}),
                      val.end());

            val.erase(std::remove_if(val.begin(),
                                     val.end(),
                                     [](char x){return std::isspace(x);}),
                      val.end());
            
            params[key] = val;
        }
    }
  
    return params;
};

std::string jsonifyParameters(Parameters& params)
{
    std::string json = "{";
    
    Parameters::iterator iter;
    
    for (iter = params.begin(); iter != params.end(); ++iter)
    {
        if(json.find("?") != -1)
        {
            json = json.substr(0, json.find("?"));
            json = json.append(", ");
        }
        
        json.append("\"");
        json.append(iter->first);
        json.append("\": \"");
        
        json.append(iter->second);
        json.append("\"?");
    }
    
    json = json.substr(0, json.find("?"));
    json = json.append("}");
    
    return json;
}




int main(int argc, const char * argv[])
{
   
#if 1
    std::string json = "{\"PlaceId\": \"1818\", \"LaunchModeInPlayer\": \"True\", \"AuthTicket\": \"abcdefg\"}";
    Parameters params = parametrizeJson(json);
    json = jsonifyParameters(params);
#endif
    
    std::cout.setf( std::ios_base::unitbuf );
    
    int messageCount = 0;
    if (argv[1] != NULL)
    {
        std::string ret;
        std::string inp = argv[1];
        unsigned int c, i, t;
        bool nativeMessaging = false;
        
        
        // Check if this is being called as a native messaging host from chrome
        if (inp.find("chrome-extension://") != std::string::npos)
        {
            nativeMessaging = true;
            
            while (true)
            {
                ++messageCount;
                // Reset inp
                inp = "";
                t = 0;
                
                // Sum the first 4 chars from stdin (the length of the message passed).
                for (i = 0; i <= 3; i++)
                    t += getchar();
                
                // Do not accept any inputs greater than 1000 chars
                if(t > 5000)
                    break;
                
                // Loop getchar to pull in the message until we reach the total
                //  length provided.
                for (i=0; i < t; i++)
                {
                    c = getchar();
                    inp += c;
                }
                
                if (inp.length() == 0)
                    break;
                
                Parameters params = parametrizeJson(inp);

                // TODO here we will get the function Name & Parameters to call from the input json
                // Input json will be of following format FunctionName and then list of parameters name value pair
                // {"FunctionName": "StartGame", "PlaceId": "1818", "LaunchModeInPlayer": "True", "AuthTicket": "abcdefg"}
                
                // Call the fn here by reading the FunctionName key from the map
                
                // TODO after the fn processing we wil read the params and jsonify it This will be the function return value sent to extension in json string format
                std::string message = jsonifyParameters(params);
                
                // Collect the length of the message
                unsigned long len = message.length();
                // We need to send the 4 btyes of length information
                std::cout << char(((len>>0) & 0xFF))
                << char(((len>>8) & 0xFF))
                << char(((len>>16) & 0xFF))
                << char(((len>>24) & 0xFF));
                
                try
                {
                    std::cout << message;
                } catch (...)
                {
                    break;
                }
            }
        }
    }
    return 0;
}

