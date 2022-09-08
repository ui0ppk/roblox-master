#pragma once

#include "UserMeshConnectionPropertyBag.h"

namespace Microsoft {
namespace Xbox {
namespace Samples {
namespace NetworkMesh {

ref class MeshManager;

public enum class ConnectionStatus
{
    Disconnected,
    Pending,
    Connected,
    PostHandshake
};

public ref class MeshConnection sealed
{
internal:
    MeshConnection(Windows::Xbox::Networking::SecureDeviceAddress^ secureDeviceAddress, MeshManager^ manager);
    

public:

    virtual ~MeshConnection();

    uint8 GetConsoleId();
    void SetConsoleId(uint8 consoleId);

    Platform::String^ GetConsoleName();
    void SetConsoleName(Platform::String^ consoleName);

    Platform::Object^ GetCustomProperty();
    void SetCustomProperty(Platform::Object^ object);

    int GetNumberOfRetryAttempts();
    void SetNumberOfRetryAttempts(int retryAttempt);

    ConnectionStatus GetConnectionStatus();
    void SetConnectionStatus(ConnectionStatus status);

    bool GetAssocationFoundInTemplate();
    void SetAssocationFoundInTemplate(bool bFound);
    
    Windows::Xbox::Networking::SecureDeviceAssociation^ GetAssociation();
    void SetAssociation(Windows::Xbox::Networking::SecureDeviceAssociation^ association);

    bool IsInComingAssociation();
    void SetInComingAssociation(bool val);

    bool IsConnectionDestroying();
    void SetConnectionDestroying(bool val);

    bool IsConnectionInProgress();
    void SetConnectionInProgress(bool val);

    void SetHeartTimer( float timer );
    float GetHeartTimer();

    Windows::Xbox::Networking::SecureDeviceAddress^ MeshConnection::GetSecureDeviceAddress();

   UserMeshConnectionPropertyBag^ GetUserPropertyBag(Platform::String^ xboxUserId);

   UserMeshConnectionPropertyBag^ AddUserPropertyBag(Platform::String^ xboxUserId);

private:
    Concurrency::critical_section m_stateLock;
    
    Windows::Xbox::Networking::SecureDeviceAddress^ m_secureDeviceAddress;
    Windows::Xbox::Networking::SecureDeviceAssociation^ m_association;
    Windows::Foundation::EventRegistrationToken m_associationStateChangeToken;

    Platform::WeakReference m_meshManager; // weak ref to MeshManager^ 
    std::map<Platform::String^, UserMeshConnectionPropertyBag^> m_userIdsToUserData;

    uint8 m_consoleId;
    Platform::String^ m_consoleName;
    Platform::Object^ m_customProperty;
    ConnectionStatus m_connectionStatus;
    bool m_isInComingAssociation;
    bool m_isConnectionInProgress;
    bool m_isConnectionDestroying;
    bool m_assocationFoundInTemplate;
    int m_retryAttempts;
    float m_timerSinceLastAttempt;
    float m_heartTimer;

    void HandleAssociationChangedEvent(
        Windows::Xbox::Networking::SecureDeviceAssociation^ association, 
        Windows::Xbox::Networking::SecureDeviceAssociationStateChangedEventArgs^ args
        );
};

}}}}