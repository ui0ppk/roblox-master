#include "pch.h"
#include "MeshConnection.h"
#include "MeshManager.h"

using namespace Microsoft::Xbox::Samples::NetworkMesh;
using namespace Windows::Foundation;

namespace Microsoft {
namespace Xbox {
namespace Samples {
namespace NetworkMesh {

MeshConnection::MeshConnection( Windows::Xbox::Networking::SecureDeviceAddress^ secureDeviceAddress, MeshManager^ manager ) :
    m_secureDeviceAddress(secureDeviceAddress),
    m_customProperty(nullptr),
    m_assocationFoundInTemplate(false),
    m_isInComingAssociation(false),
    m_isConnectionDestroying(false),
    m_isConnectionInProgress(false),
    m_retryAttempts(0),
    m_timerSinceLastAttempt(0.0f),
    m_connectionStatus(ConnectionStatus::Disconnected),
    m_consoleId(0xFF),
    m_heartTimer(0.0f)
{
    m_meshManager = Platform::WeakReference(manager);
    m_userIdsToUserData = std::map<Platform::String^, UserMeshConnectionPropertyBag^>();

    if(secureDeviceAddress == nullptr || manager == nullptr)
    {
        throw ref new Platform::InvalidArgumentException();
    }
}

MeshConnection::~MeshConnection()
{
    if (m_association != nullptr)
    {
        m_association->StateChanged -= m_associationStateChangeToken;
    }
}

uint8 MeshConnection::GetConsoleId()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_consoleId;
}

void MeshConnection::SetConsoleId(uint8 consoleId)
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_consoleId = consoleId;
}

Platform::String^ MeshConnection::GetConsoleName()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);

    Platform::String^ consoleName = L"n/a";
    if( !m_consoleName->IsEmpty() )
    {
        consoleName = m_consoleName;
    }

    return consoleName;
}

void MeshConnection::SetConsoleName(Platform::String^ consoleName)
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_consoleName = consoleName;
}

Platform::Object^ MeshConnection::GetCustomProperty()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_customProperty;
}

void MeshConnection::SetCustomProperty(Platform::Object^ object)
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_customProperty = object;
}

int MeshConnection::GetNumberOfRetryAttempts()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_retryAttempts;
}

void MeshConnection::SetNumberOfRetryAttempts(int retryAttempt)
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_retryAttempts = retryAttempt;
}

ConnectionStatus MeshConnection::GetConnectionStatus()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_connectionStatus;
}

void MeshConnection::SetConnectionStatus(ConnectionStatus status)
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_connectionStatus = status;
}

bool MeshConnection::GetAssocationFoundInTemplate()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_assocationFoundInTemplate;
}

void MeshConnection::SetAssocationFoundInTemplate(bool val)
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_assocationFoundInTemplate = val;
}

Windows::Xbox::Networking::SecureDeviceAssociation^ MeshConnection::GetAssociation()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_association;
}

void MeshConnection::SetAssociation( Windows::Xbox::Networking::SecureDeviceAssociation^ association )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);

    if (m_association != nullptr)
    {
        m_association->StateChanged -= m_associationStateChangeToken;
    }

    m_association = association;

    if(association != nullptr)
    {
        // This is needed to know if the association is ever dropped.
        TypedEventHandler<Windows::Xbox::Networking::SecureDeviceAssociation^, Windows::Xbox::Networking::SecureDeviceAssociationStateChangedEventArgs^>^ stateChangeEvent = 
            ref new TypedEventHandler<Windows::Xbox::Networking::SecureDeviceAssociation^, Windows::Xbox::Networking::SecureDeviceAssociationStateChangedEventArgs^>(
            [this] (Windows::Xbox::Networking::SecureDeviceAssociation^ association, Windows::Xbox::Networking::SecureDeviceAssociationStateChangedEventArgs^ args)
        {
            HandleAssociationChangedEvent(association, args);
        });

        m_associationStateChangeToken = association->StateChanged += stateChangeEvent;
    }
}

void MeshConnection::HandleAssociationChangedEvent(
    Windows::Xbox::Networking::SecureDeviceAssociation^ association, 
    Windows::Xbox::Networking::SecureDeviceAssociationStateChangedEventArgs^ args)
{
    MeshManager^ meshManager = m_meshManager.Resolve<MeshManager>();
    //if (meshManager)
        meshManager->OnAssociationChange(args, association);
}

Windows::Xbox::Networking::SecureDeviceAddress^ MeshConnection::GetSecureDeviceAddress()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_secureDeviceAddress;
}

bool MeshConnection::IsInComingAssociation()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_isInComingAssociation;
}

void MeshConnection::SetInComingAssociation(bool val)
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_isInComingAssociation = val;
}

bool MeshConnection::IsConnectionDestroying()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_isConnectionDestroying;
}

void MeshConnection::SetConnectionDestroying( bool val )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_isConnectionDestroying = val;
}

bool MeshConnection::IsConnectionInProgress()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_isConnectionInProgress;
}

void MeshConnection::SetConnectionInProgress( bool val )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_isConnectionInProgress = val;
}

void MeshConnection::SetHeartTimer( float timer )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_heartTimer = timer;
}

float MeshConnection::GetHeartTimer()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_heartTimer;
}


UserMeshConnectionPropertyBag^ MeshConnection::GetUserPropertyBag( Platform::String^ xboxUserId )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_userIdsToUserData[xboxUserId];
}

UserMeshConnectionPropertyBag^ MeshConnection::AddUserPropertyBag( Platform::String^ xboxUserId )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    UserMeshConnectionPropertyBag^ userMeshConnectionPropertyBag = ref new UserMeshConnectionPropertyBag(xboxUserId);
    m_userIdsToUserData[xboxUserId] = userMeshConnectionPropertyBag;
    return userMeshConnectionPropertyBag;
}

}}}}