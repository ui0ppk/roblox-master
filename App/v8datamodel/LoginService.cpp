//
//  LoginService.cpp
//  App
//
//  Created by Ben Tkacheff on 5/1/13.
//
//
#include "stdafx.h"

#include "v8datamodel/LoginService.h"
#include "network/Players.h"

namespace RBX
{
    const char* const sLoginService = "LoginService";
    
    REFLECTION_BEGIN();
    static Reflection::BoundFuncDesc<LoginService, void(void)> func_PromptLogin(&LoginService::promptLogin, "PromptLogin", Security::Roblox);
    static Reflection::BoundFuncDesc<LoginService, void(void)> func_Logout(&LoginService::logout, "Logout", Security::Roblox);
    
    static Reflection::EventDesc<LoginService, void(std::string)> event_SignupFinished(&LoginService::loginSucceededSignal, "LoginSucceeded", "username", Security::Roblox);
    static Reflection::EventDesc<LoginService, void(std::string)> event_LoginFinished(&LoginService::loginFailedSignal, "LoginFailed", "loginError" , Security::Roblox);
    REFLECTION_END();
    
    LoginService::LoginService()
    {
        setName(sLoginService);
    }
    
    void LoginService::promptLogin()
    {
        if(RBX::Network::Players::frontendProcessing(this))
        {
            promptLoginSignal();
        }
        else
            throw std::runtime_error("LoginService:promptLogin() should only be accessed from a local script");
    }
    
    void LoginService::logout()
    {
        if(RBX::Network::Players::frontendProcessing(this))
        {
            promptLogoutSignal();
        }
        else
            throw std::runtime_error("LoginService:logout() should only be accessed from a local script");
    }
    
    
}
