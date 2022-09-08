
#include "p2p.h"

#include <collection.h>

#include "rbx/Debug.h"
#include "async.h"

extern void dprintf( const char* fmt, ... );

enum { dbg = 0 };

//#undef RBXASSERT
//#define RBXASSERT(X)  ( (void)( (X) || (__debugbreak(), 1) ) )



namespace Xp2p
{
    const wchar_t* kChatTemplateName = L"MultiplayerUdp"; // check appmanifest

    enum 
    { 
        kRetransmitMaxPackets = 3,
        kRetransmitPeriodMs = 1000,
        kRetransmitLoopMs   = 30,
        kRetransmitQueueLength = 1000,
    };


    //////////////////////////////////////////////////////////////////////////

    Address::Address()
    {
        memset( &rawdata, 0, sizeof(rawdata) );
    }


    Address::Address(in6_addr addr, unsigned port)
    {
        memset( &rawdata, 0, sizeof(rawdata) );
        sockaddr_in6& sa = (sockaddr_in6&)rawdata;
        sa.sin6_family = AF_INET6;
        sa.sin6_port = htons(port);
        sa.sin6_addr = addr;
    }


    char*  Address::str(char* to ) const
    {
        char buf[128];
        inet_ntop(AF_INET6, &addr6(), buf, 128 ); 
        sprintf( to, "%s:%u", buf, port()  );
        return to;
    }

    //////////////////////////////////////////////////////////////////////////

    Sock::Sock(unsigned port)
    {
        int result;

        fd = socket( AF_INET6, SOCK_DGRAM, IPPROTO_UDP );

        int v6only = 0;
        setsockopt( fd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&v6only, sizeof(v6only) ); // MS tells to do this, idk.

        Address local( in6addr_any, port );
        result = bind( fd, local.toSockaddr(), local.size() );
        if(dbg) dprintf( "p2p: socket %d bind to local %d\n", fd, result );
    }


    Sock::~Sock()
    {
        closesocket( fd );
        if(dbg) dprintf( "p2p: socket %d closed\n", fd );
    }


    int Sock::send( const Address& to, const void* data, unsigned size, unsigned timeoutMs )
    {
        RBXASSERT(data);

        fd_set set;
        int result;
        timeval tout = { timeoutMs / 1000, (timeoutMs % 1000)*1000 };

        FD_ZERO( &set );
        FD_SET( fd, &set );
        result = select( 1, 0, &set, 0, &tout );
        if( result > 0 && FD_ISSET( fd, &set ) )
        {
            result = ::sendto( fd, (const char*)data, size, 0, to.toSockaddr(), to.size() );
            DWORD err = WSAGetLastError();
            return result;
        }
        if( result == 0 )
        {
            return Net_Timeout;
        }
        return Net_Fail;
    }


    int Sock::recv( Address* from, void* data, unsigned size, unsigned timeoutMs )
    {
        RBXASSERT(from);
        RBXASSERT(data);

        fd_set set;
        int result;
        int namelen = from->size();
        timeval tout = { timeoutMs / 1000, (timeoutMs % 1000)*1000 };

        FD_ZERO( &set );
        FD_SET( fd, &set );
        result = select( 1, &set, 0, 0, &tout );
        if( result > 0 && FD_ISSET( fd, &set ) )
        {
            result = ::recvfrom( fd, (char*)data, size, 0, from->toSockaddr(), &namelen );
            DWORD err = WSAGetLastError();
            return result;
        }
        if( result == 0 )
        {
            return Net_Timeout;
        }
        return Net_Fail;
    }



    //////////////////////////////////////////////////////////////////////////
    //

    Peer::Peer()
    {
        state     = State_New;
        pingTime  = 0;
        userData  = 0;
        devAssoc  = nullptr;
        userObject   = nullptr;
        txseq     = 0;
        rxseq     = 0;
        txseqR    = 0;
        rxseqR    = 0;
        rxackR    = 0;
        id        = 0xffffffff;
    }


    int Peer::setstate( int expect, int replace )
    {
        return InterlockedCompareExchange( &state, replace, expect );
    }

    //////////////////////////////////////////////////////////////////////////

    static void dbgPrintPacket( const PacketHeader* header )
    {
        dprintf( "packet: id %u  type %u  size %u  peer %u  seq %u  seqR %u  flags 0x%x\n", (unsigned)header->id, (unsigned)header->type, (unsigned)header->size, (unsigned)header->id, (unsigned)header->seq, (unsigned)header->seqR, (unsigned)header->flags );
    }

    static void dbgPrintTx( PeerPtr peer )
    {
        dprintf( "peer: id %u  tx %u  rx %u  txr %u  rxr %u\n", (unsigned)peer->id, (unsigned)peer->txseq, (unsigned)peer->rxseq, (unsigned)peer->txseqR, (unsigned)peer->rxseqR );
    }

    //////////////////////////////////////////////////////////////////////////

    Network::Network( MultiplayerSessionMember^ currentUser ) 
        : sock( kNetworkPort )
    {
        myMemberId = currentUser->MemberId;
        assTemplate = SecureDeviceAssociationTemplate::GetTemplateByName( ref new Platform::String(kChatTemplateName) );
        controlRunFlag = 1;
        controlThreadObject = boost::thread( [this]()->void { controlThread(); } );

        incomingHandler = assTemplate->AssociationIncoming +=  ref new TypedEventHandler<SecureDeviceAssociationTemplate^, SecureDeviceAssociationIncomingEventArgs^>(
            [this]( SecureDeviceAssociationTemplate^, SecureDeviceAssociationIncomingEventArgs^ args )->void
            {
                lock lck(peerMutex);
                pendingList.push_back( args->Association );
                if(dbg) dprintf( "p2p: incoming association: %S\n", args->Association->RemoteHostName->DisplayName->Data() );
            }
        );

    }


    Network::~Network()
    {
        controlRunFlag = 0;
        controlThreadObject.join();

        {
            lock lck(peerMutex);
            for( auto p: peers )
                destroyPeer(p);
        }

        for( unsigned num = 1; num; )
        {
            {
                lock lck(peerMutex);
                num = peers.size();
            }
            Sleep(10);
        }

        assTemplate->AssociationIncoming -= incomingHandler;
    }



    NetResult Network::sendPacket( PeerPtr peer, PacketHeader* packet, unsigned timeoutMs, unsigned flags )
    {
        RBXASSERT( packet->type != 0 ); // don't use type 0, probably an error!
        RBXASSERT( packet->size <= sizeof(PacketStorage) ); // uwot!
        RBXASSERT( packet->size >= sizeof(PacketHeader)  ); // uwot! #2
        RBXASSERT( peer.get() );

        int oldstate = peer->setstate( Peer::State_Ready, Peer::State_Busy );
        switch(oldstate)
        {
        case Peer::State_Ready:
            break;
        case Peer::State_Busy:
        case Peer::State_Busy2:
            return Net_Retry;
        default:
            return Net_NotReady;
        }

        packet->id    = myMemberId;
        packet->flags = flags;
        packet->seq = ++ peer->txseq;
        packet->timestamp = GetTickCount();
       
        if( flags & Packet_Reliable )
        {
            packet->seqR = ++ peer->txseqR;

            if( peer->reliable.size() < kRetransmitQueueLength )
            {
                PacketHeader* copy = (PacketHeader*)malloc( packet->size ); // retain the packet
                memcpy( copy, packet, packet->size );
                peer->reliable.push_back( copy );
            }
            else
            {
                dprintf( "p2p: WARN: peer retransmit queue is FULL, packet will not be reliable: ");
                dbgPrintPacket(packet);
            }
        }
        else
        {
            packet->seqR = peer->txseqR;
        }
        // NOTE: do not change the packet below! If it's reliable, its copy will not have the changes!

        if( Peer::State_Busy != peer->setstate( Peer::State_Busy, Peer::State_Ready ) )
            RBXASSERT( !"peer lock violation" );

        char buf[128];
        if(dbg) dprintf( "p2p: outgoing to %s (%u) type %u seq %u seqR %u size %u flags 0x%x", peer->remoteAddr.str(buf), (unsigned)packet->id, (unsigned)packet->type, (unsigned)packet->seq, (unsigned)packet->seqR, (unsigned)packet->size, (unsigned)packet->flags );

        // assumes peer->remoteAddr never changes
        int result = sock.send( peer->remoteAddr, packet, packet->size, timeoutMs );

        if(dbg) dprintf( result == packet->size? "... ok\n": "... failed!\n" );

        if( result < 0 )
        {
            return (NetResult)result;
        }
        if( result != packet->size )
        {
            return Net_Fail;
        }
        return Net_Ok;
    }


    NetResult Network::recvPacket( PacketStorage* packet, unsigned timeoutMs )
    {
        Address from;
        int result = sock.recv( &from, packet, sizeof(PacketStorage), timeoutMs );
        
        if( result < 0 )                               return (NetResult)result;  // recv error (timeout/disconnect/whatever)
        if( packet->size     > sizeof(PacketStorage) ) return Net_Packet; // bad packet size
        if( (unsigned)result < sizeof(PacketHeader)  ) return Net_Packet; // bad packet size
        
        char buf[128];
        if(dbg) dprintf("p2p: incoming from %s (%u) type %u seq %u seqR %u size %u flags 0x%x ... ok\n", from.str(buf), (unsigned)packet->id, (unsigned)packet->type, (unsigned)packet->seq, (unsigned)packet->seqR, (unsigned)packet->size, (unsigned)packet->flags );
        return Net_Ok;
    }

    // it's kinda out-of-band, sequencing is irrelevant
    static bool sendAck( Sock& sock, const Address& addr, unsigned myid, unsigned seqR )
    {
        PacketHeader ack = {};
        ack.type         = 0; // yep
        ack.seqR         = seqR;
        ack.id           = myid;
        ack.timestamp    = GetTickCount();

        int acqr = sock.send( addr, &ack, sizeof(ack), 10 );
        return acqr == sizeof(ack);
    }

    NetResult Network::processPacket( PeerPtr& who, PacketHeader* header )
    {
        char buf[120];

        if(1)
        {
            lock lck(peerMutex);
            for( auto& itor: peers )  if( itor->id == header->id )  { who = itor; break; }
        }
        
        if( !who ) return Net_Peer;

        int oldstate = who->setstate( Peer::State_Ready, Peer::State_Busy );
        switch(oldstate)
        {
        case Peer::State_Ready:
            break;
        case Peer::State_Busy:
        case Peer::State_Busy2:
            return Net_Retry;
        default:
            return Net_NotReady;
        }

        Address remoteAddr( who->remoteAddr );
        bool       isReliable = header->flags & Packet_Reliable;
        bool       needAck    = false;
        NetResult  result; // intentionally uninitialized

        if( header->type == 0 ) // it's an ack packet from the peer
        {
            if( header->seqR == who->rxackR + 1 )
            {
                if(dbg) dprintf("p2p: got ACK for %u\n", header->seqR );
                who->rxackR = header->seqR;
            }
            result = Net_Packet;  // the client is not interested in it, discard
        }
        else if( isReliable )
        {
            if( header->seqR == who->rxseqR + 1 ) // okay: although the reliable packet is what we expect, looks like we're going to lose some unreliable packets later
            {
                needAck = true;
                result = Net_Ok;
            }
            else if( header->seqR <= who->rxseqR ) // okay..ish: it's from the past, so we discard the packet but send an ack
            {
                needAck = true;
                result = Net_Packet; 
            }
            else // it's from the future, so discard, because there's another reliable packet in flight
            {
                result = Net_Packet;
            }
        }
        else // regular
        {
            if( header->seq >= who->rxseq + 1 ) // packet is in-sequence or from the future
            {
                if( header->seqR == who->rxseqR ) // good: we don't have any reliable packets in-between, so even if the packet is from the future, we still accept it
                {
                    result = Net_Ok;
                }
                else // bad: there's a reliable packet with a lower 'seq' that's not been delivered yet, discard this packet
                {
                    result = Net_Packet;
                }
            }
            else // the packet is from the past, discard
            {
                result = Net_Packet;
            }
        }

        // final step:
        if( result == Net_Ok ) // update on success
        {
            who->rxseq    = header->seq;
            who->rxseqR   = header->seqR;
            who->pingTime = GetTickCount();
        }

        who->setstate( Peer::State_Busy, Peer::State_Ready );

        if( needAck )
        {
            sendAck(sock, remoteAddr, myMemberId, header->seqR);
        }
        
        return result;
    }


    PeerPtr Network::createPeer( MultiplayerSessionMember^ member, boost::function<void(PeerPtr)> onConnected )
    {
        lock lck(peerMutex);

        for( auto x: peers )
            if( x->id == member->MemberId )
                return x;


        PeerPtr peer( new Peer );
        peers.push_back(peer);
        boost::thread( [this, peer, member, onConnected]() -> void { connectPeer( peer, member, onConnected ); } ).detach();
        return peer;
    }


    void Network::destroyPeer( PeerPtr peer )
    {
        RBXASSERT( !peers.empty() ); // um...
        boost::thread(
            [this, peer]() -> void
            {
                unsigned index;
                for (;;)
                {
                    peerMutex.lock();
                    index = std::find( peers.begin(), peers.end(), peer ) - peers.begin();
                    if( index == peers.size() ) { peerMutex.unlock(); return; } // not found
                    
                    if( Peer::State_Ready   == peer->setstate( Peer::State_Ready, Peer::State_Dead ) ) break;
                    if( Peer::State_New     == peer->setstate( Peer::State_New,   Peer::State_Dead ) ) break;
                    if( Peer::State_Waiting == peer->setstate( Peer::State_Waiting, Peer::State_Dead ) ) break;
                    if( Peer::State_Error   == peer->setstate( Peer::State_Error, Peer::State_Dead ) ) break;
                    peerMutex.unlock();

                    SleepEx(10, false);
                }
                // the mutex is still locked

                char buf[128];
                if(dbg) dprintf( "p2p: destroy peer %s\n", peer->remoteAddr.str(buf) );

                for( auto ptr: peer->reliable ) { free(ptr); }

                RBXASSERT( !peers.empty() );
                RBXASSERT( index < peers.size() );
                peers[index] = peers.back();
                peers.resize( peers.size() - 1 );

                async( peer->devAssoc->DestroyAsync() ).detach();
                peer->devAssoc = nullptr;
                
                peerMutex.unlock();
            }
        ).detach();
    }


    void Network::connectPeer( PeerPtr peer, MultiplayerSessionMember^ member, boost::function<void(PeerPtr)> onConnected )
    {
        if( Peer::State_New != peer->setstate( Peer::State_New, Peer::State_Connecting ) )
            RBXASSERT( !"oops" );

        SecureDeviceAddress^ addr = SecureDeviceAddress::FromBase64String( member->SecureDeviceAddressBase64 );
        auto devAssTemplate = this->assTemplate;

        try
        {
            async( devAssTemplate->CreateAssociationAsync( addr, CreateSecureDeviceAssociationBehavior::Default ) )
            .complete( 
                [peer]( SecureDeviceAssociation^ d ) mutable
                {
                    peer->devAssoc = d;
                }
            ).join();
        }
        catch(Platform::Exception^ e )
        {
            // maybe try again later?
            peer->setstate( Peer::State_Connecting, Peer::State_Error );
            return;
        }

        while(!peer->devAssoc)
        {
            if(1)
            {
                lock lck(peerMutex);
                for( unsigned j = 0; j<pendingList.size(); ++j )
                {
                    auto pendingAssoc = pendingList[j]; 
                    SecureDeviceAddress^ pendingAddr = pendingAssoc->RemoteSecureDeviceAddress;
                    if( 0 == addr->Compare(pendingAddr) )
                    {
                        peer->devAssoc = pendingAssoc;
                        pendingList[j] = pendingList.back();
                        pendingList.resize( pendingList.size() - 1);
                        break;
                    }
                }
                if(peer->devAssoc) break;
            }
            Sleep(10);
        }

        if(dbg) dprintf( "p2p: connected to: %S\n", peer->devAssoc->RemoteHostName->DisplayName->Data() );

        peer->id = member->MemberId;
        Platform::ArrayReference<BYTE> remoteSocketAddressBytes( (BYTE*)&peer->remoteAddr.rawdata, sizeof(peer->remoteAddr.rawdata) );
        peer->devAssoc->GetRemoteSocketAddressBytes( remoteSocketAddressBytes );

        // got the address, we can now send the data
        peer->setstate( Peer::State_Connecting, Peer::State_Ready );
        onConnected(peer);
    }


    // transmission control
    void Network::controlThread()
    {
        unsigned peerItor = 0;
        while( controlRunFlag )
        {
            Sleep(kRetransmitLoopMs);

            // pick a peer
            PeerPtr peer;
            peerMutex.lock();
            if( !peers.empty() ) 
                peer = peers[ peerItor++ % peers.size() ];
            peerMutex.unlock();

            if(!peer)
                continue;

            // Acquire a lock on the peer
            int oldstate;
            while( 1 ) 
            {
                oldstate = peer->setstate( Peer::State_Ready, Peer::State_Busy2 );
                if( oldstate == Peer::State_Ready ) break; // good
                if( oldstate != Peer::State_Busy ) break; // bad
                Sleep(1); // wait a bit and try again
            }

            if( oldstate != Peer::State_Ready )
                continue; // the peer is not in an operational state, ignore

            unsigned delUpTo = 0;
            unsigned numSend = 0;
            PacketHeader* toSend[kRetransmitMaxPackets] = {};

            for( unsigned j=0, e=peer->reliable.size(); j<e && numSend < kRetransmitMaxPackets; ++j )
            {
                PacketHeader* packet = peer->reliable[j];
                if( packet->seqR <= peer->rxackR )
                {
                    delUpTo = j; // we got confirmation for this packet, discard
                    continue;
                }

                if( packet->timestamp + kRetransmitPeriodMs > GetTickCount() )
                    break; // the packet is not yet eligible for retry, and all following packets either, because they're ordered
               
                toSend[numSend++] = packet;
            }

            if( delUpTo )
            {
                for( unsigned j=0; j<delUpTo; ++j ) 
                { 
                    if(dbg) { dprintf("p2p: removing reliable packet: "); dbgPrintPacket(peer->reliable[j] ); }
                    free(peer->reliable[j]); 
                }
                peer->reliable.erase( peer->reliable.begin(), peer->reliable.begin() + delUpTo );
            }

            // unlock the peer, as the actual net send operations can be concurrent
            if( Peer::State_Busy2 != peer->setstate(Peer::State_Busy2, Peer::State_Ready ) )
                RBXASSERT( !"peer lock violation #2" );

            // retransmit
            for( unsigned j=0; j<numSend; ++j )
            {
                toSend[j]->timestamp = GetTickCount();
                if(dbg) { dprintf("p2p: retransmit "); dbgPrintPacket(toSend[j]); }
                int result = sock.send( peer->remoteAddr, toSend[j], toSend[j]->size, 10 );
                if( result < 0 ) break; // error with the socket don't continue
            }
        }
    }

    enum Bool { True, False, IDunno };
}

