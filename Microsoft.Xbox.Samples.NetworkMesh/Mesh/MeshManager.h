//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once
#include "macros.h"
#include "XboxNetworkMeshDiagnosticsTraceLevel.h"
#include "MeshThread.h"
#include "MeshPacketManager.h"
#include "MeshConnection.h"
#include "MeshEvents.h"

namespace Microsoft {
namespace Xbox {
namespace Samples {
namespace NetworkMesh {

ref class MeshPacketManager;

#define DEFAULT_HEARTBEAT_PERIOD_MILLISECONDS 2048

public ref class MeshManager sealed
{
public:
    /// <summary>
    /// Pass in the template for the title's SDA and kick off the WSA initialization
    /// </summary>
    /// <param name="secureDeviceAssociationTemplateName"></param>
    MeshManager(
        uint8 localConsoleId, 
        Platform::String^ secureDeviceAssociationTemplateName, 
        Platform::String^ localConsoleName, 
        bool dropOutOfOrderPackets
        );

    Platform::String^ GetLocalConsoleDisplayName();
    Platform::String^ GetLocalConsoleName();
    void SetLocalConsoleName(Platform::String^ consoleName);

    /// <summary>
    /// Return the template being used by local mesh manager
    /// </summary>
    Windows::Xbox::Networking::SecureDeviceAssociationTemplate^ GetSecureDeviceAssociationTemplate();

    /// <summary>
    /// Pass in the SDA you want to connect to.
    /// </summary>
    /// <param name="securedeviceAddress"></param>
    void ConnectToAddress(Windows::Xbox::Networking::SecureDeviceAddress^ secureDeviceAddress, Platform::String^ debugName );

    /// <summary>
    /// </summary>
    Windows::Foundation::Collections::IVectorView<MeshConnection^>^ GetConnections();

    /// <summary>
    /// </summary>
    Windows::Foundation::Collections::IVectorView<MeshConnection^>^ GetConnectionsByType(ConnectionStatus type);

    /// <summary>
    /// </summary>
    MeshConnection^ GetConnectionFromAssociation(Windows::Xbox::Networking::SecureDeviceAssociation^ association);

    /// <summary>
    /// </summary>
    MeshConnection^ GetConnectionFromSecureDeviceAddress(Windows::Xbox::Networking::SecureDeviceAddress^ address);
    
    MeshConnection^ GetConnectionFromConsoleId(uint8 consoleId);

    /// <summary>
    /// This will destroy the association and remove the connection from the list of all connections.
    /// </summary>
    void DestroyConnection( MeshConnection^ connection); 

    /// <summary>
    /// Given a secure device address, this will destroy the association
    /// and remove the connection from the list of all connections.
    /// </summary>
    void DisconectFromAddress( Windows::Xbox::Networking::SecureDeviceAddress^ address); 

    /// <summary>
    /// Deletes all connections and destroys all associations.
    /// </summary>
    void DestroyAndDisconnectAll(); 

    /// <summary>
    /// Cleanup winsock
    /// </summary>
    void Shutdown();

    /// <summary>
    /// </summary>
    Microsoft::Xbox::Samples::NetworkMesh::MeshPacketManager^ GetMeshPacketManager();

    /// <summary>
    /// Gets the Heartbeat period (in milliseconds)
    /// </summary>
    UINT GetHeartbeatPeriod();

    /// <summary>
    /// sets the Heartbeat period (in milliseconds)
    /// </summary>
    void SetHeartbeatPeriod(UINT periodInMilliseconds);

    /// <summary>
    /// // Warn the game that this connection is being disconnected. The game would have to then explicitly 
    /// call DisconnectFromAddress or DisconnectFromConnection to remove him from the connection list, if needed.
    /// Otherwise, we will retry connecting to him.
    /// </summary>
    event Windows::Foundation::EventHandler<MeshConnection^>^ OnDisconnected;

    /// <summary>
    /// This event is triggered when a proper handshake has been established between 2 connections.
    /// </summary>
    event Windows::Foundation::EventHandler<MeshConnection^>^ OnPostHandshake;

    /// <summary>
    /// This event is triggered upon receiving heartbeats from remote connection.
    /// </summary>
    event Windows::Foundation::EventHandler<MeshConnection^>^ OnHeartbeat;

    event Windows::Foundation::EventHandler<Microsoft::Xbox::Samples::NetworkMesh::DebugMessageEventArgs^>^ OnDebugMessage;

internal:

    void OnAssociationChange( 
        Windows::Xbox::Networking::SecureDeviceAssociationStateChangedEventArgs^ args, 
        Windows::Xbox::Networking::SecureDeviceAssociation^ association );

    /// <summary>
    /// Return the template being used by local mesh manager
    /// </summary>
    void RefreshConnections();

private:
    Concurrency::critical_section m_connectionsLock;
    
    Microsoft::Xbox::Samples::NetworkMesh::MeshPacketManager^ m_meshPacketManager;
    Platform::String^ m_localConsoleName;

    MeshThread^ m_autoConnectThread;
    Windows::Xbox::Networking::SecureDeviceAssociationTemplate^ m_associationTemplate;
    std::vector<MeshConnection^> m_connections;
    bool m_dropOutOfOrderPackets;

    MeshThread^ m_heartbeatThread;

    void Initialize(uint8 localConsoleId);
    void RegisterMeshPacketEventHandlers();
    
    MeshConnection^ AddConnection( 
        Windows::Xbox::Networking::SecureDeviceAddress^ secureDeviceAddress,
        Windows::Xbox::Networking::SecureDeviceAssociation^ secureDeviceAssociation,
        bool inComingAssociation,
        ConnectionStatus connectionStatus    
        );

    bool AreSecureDeviceAddressesEqual(
        Windows::Xbox::Networking::SecureDeviceAddress^ secureDeviceAddress1,
        Windows::Xbox::Networking::SecureDeviceAddress^ secureDeviceAddress2
        );

    /// <summary>
    /// Deletes the connection from the list of all connections.
    /// </summary>
    void DeleteConnection(MeshConnection^ connection);
    void DestroyAllTemplateAssociations();
    bool DoesConnectionExistInList(Windows::Foundation::Collections::IVectorView<MeshConnection^>^ list, MeshConnection^ connection);

    ///////////////////////
    // Events

    Windows::Foundation::EventRegistrationToken m_associationIncomingToken;
    Windows::Foundation::EventRegistrationToken m_onHelloReceivedToken;
    Windows::Foundation::EventRegistrationToken m_onHeartbeatReceivedToken;
    Windows::Foundation::EventRegistrationToken m_onDebugMessageReceivedToken;

    void OnHeartbeatReceived( Microsoft::Xbox::Samples::NetworkMesh::MeshHeartbeatReceivedEvent^ args );
    void OnHelloReceived( Microsoft::Xbox::Samples::NetworkMesh::MeshHelloReceivedEvent^ args );
    void OnDebugMessageReceived( Microsoft::Xbox::Samples::NetworkMesh::DebugMessageEventArgs^ args );

    void OnAutoConnectWorkerThreadDoWork ( Microsoft::Xbox::Samples::NetworkMesh::ProcessThreadsEventArgs^ args );
    void OnAssociationIncoming( 
        Windows::Xbox::Networking::SecureDeviceAssociationTemplate^ associationTemplate, 
        Windows::Xbox::Networking::SecureDeviceAssociationIncomingEventArgs^ args 
        );

    void OnHeartbeatWorkerThreadDoWork( Microsoft::Xbox::Samples::NetworkMesh::ProcessThreadsEventArgs^ args );

    ///////////////////////
    // Logging
    void LogComment( Platform::String^ strText );
    void LogCommentFormat( LPCWSTR strMsg, ... );
};

}}}}


