//
//  NotificationService.cpp
//  App
//
//  Created by Ganesh Agrawal on 5/15/14.
//
//
#include "stdafx.h"

#include "V8DataModel/NotificationService.h"
#include "V8DataModel/UserInputService.h"
#include "Network/Players.h"



FASTFLAGVARIABLE(NotificationServiceEnabledForEveryone, false);

namespace RBX
{
	const char* const sNotificationService = "NotificationService";
    
    
    REFLECTION_BEGIN();
    static Reflection::BoundFuncDesc<NotificationService, void(int, int, std::string, int) >
    func_ScheduleNotification( &NotificationService::scheduleNotification, "ScheduleNotification", "userId", "alertId", "alertMsg", "minutesToFire", Security::RobloxPlace);
    
    static Reflection::BoundFuncDesc<NotificationService, void(int, int) >
    func_CancelNotification( &NotificationService::cancelNotification, "CancelNotification", "userId", "alertId", Security::RobloxPlace);

    static Reflection::BoundFuncDesc<NotificationService, void(int) >
    func_CancelAllNotification( &NotificationService::cancelAllNotification, "CancelAllNotification", "userId", Security::RobloxPlace);
    
    static Reflection::BoundYieldFuncDesc<NotificationService, shared_ptr<const Reflection::ValueArray>(int)> func_GetScheduledNotifications( &NotificationService::getScheduledNotifications, "GetScheduledNotifications", "userId", Security::RobloxPlace);
    REFLECTION_END();
    
   
    NotificationService::NotificationService()
    {
        setName(sNotificationService);
    }
    
    bool NotificationService::canUseService()
    {
        bool retVal = true;
        
        if (!FFlag::NotificationServiceEnabledForEveryone)
        {
            StandardOut::singleton()->printf(MESSAGE_WARNING, "Sorry, NotificationService is currently off.");
            retVal = false;
        }
        
        if (RBX::UserInputService* inputService = RBX::ServiceProvider::find<RBX::UserInputService>(this))
        {
            if (!inputService->getTouchEnabled())
            {
                StandardOut::singleton()->printf(MESSAGE_WARNING, "Sorry, NotificationService only works on touch devices currently.");
                retVal = false;
            }
        }
        
        if (!Network::Players::frontendProcessing(this))
        {
            StandardOut::singleton()->printf(MESSAGE_WARNING, "NotificationService:ScheduleNotification must be called from a local script!");
            retVal = false;
        }
        return retVal;
    }
    
    void NotificationService::scheduleNotification(int userId, int alertId, std::string alerMsg, int minutesToFire)
    {
        if (canUseService())
            scheduleNotificationSignal(userId, alertId, alerMsg, minutesToFire);
    }
    
    void NotificationService::cancelNotification(int userId, int alertId)
    {
        if (canUseService())
            cancelNotificationSignal(userId, alertId);
    }
    
    void NotificationService::cancelAllNotification(int userId)
    {
        if (canUseService())
            cancelAllNotificationSignal(userId);
    }
    

    
    void NotificationService::getScheduledNotifications(int userId, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)>	errorFunction)
    {
        if (canUseService())
            getScheduledNotificationsSignal(userId, resumeFunction,	errorFunction);
        else
            errorFunction("Notification Service Not Available");
    }

}// namespace RBX


