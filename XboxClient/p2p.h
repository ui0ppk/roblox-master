#pragma once

// p2p.h - peer to peer UDP networking for voice
//
// This header is *very* toxic.

#include <wrl.h>
#include <xdk.h>

#include <vector>
#include <algorithm>
#include <mutex>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>

#include <winsock2.h>
//#include <ws2def.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>

#include <boost/function.hpp>


using namespace Microsoft::Xbox::Services::Multiplayer;
using namespace Windows::Xbox::Networking;
using namespace Windows::Foundation;

namespace Xp2p
{
    struct PacketHeader;
    class Peer;
    typedef boost::shared_ptr<Peer> PeerPtr;

    enum
    {
        kNetworkPort   = 8700, // NOTE: check the manifest!
        kMaxUdpSize    = 1264, // what MS says in the XDK help
    };

    enum NetResult
    {
        Net_Ok,
        Net_Fail      = -1,     // operation failed
        Net_Timeout   = -2,     // operation timed out (you'll be getting quite a few of these)
        Net_Peer      = -3,     // bad peer
        Net_Retry     = -4,     // operation aborted because the peer was busy, the calling code should try again
        Net_NotReady  = -5,     // operation aborted because the peer is not ready, 
        Net_Packet    = -6,     // bad incoming packet
    };

    enum PacketFlags
    {
        Packet_Default  = 0,
        Packet_Reliable = 1,
        Packet_Ordered  = 2,
    };

    struct Address
    {
        SOCKADDR_STORAGE  rawdata;

        Address();
        Address(in6_addr addr, unsigned port);

        sockaddr* toSockaddr()   const      { return (sockaddr*)&rawdata; }
        int       size()         const      { return sizeof(rawdata); }
        in6_addr  addr6()        const      { return ((sockaddr_in6&)rawdata).sin6_addr; }
        unsigned  port()         const      { return ntohs( ((sockaddr_in6&)rawdata).sin6_port ); }
        char*     str(char* to)  const;
    };

    class Sock : public boost::noncopyable
    {
        int fd;
    public:
        Sock(unsigned port);
        ~Sock();
        int send( const Address& to, const void* data, unsigned size, unsigned timeoutMs );
        int recv( Address* from, void* data, unsigned size, unsigned timeoutMs );
    };

    

    class Peer : public boost::enable_shared_from_this<Peer>, boost::noncopyable
    {
        friend class Network;
        friend void dbgPrintTx(PeerPtr p);

        enum State { State_New, State_Connecting, State_Waiting, State_Ready, State_Busy, State_Busy2, State_Error, State_Dead };

        volatile long              state;      // mutex and state indicator
        SecureDeviceAssociation^   devAssoc;   // device association (kinda like VPN transport layer between two consoles)
        Address                    remoteAddr; // remote address as reported by devass
        uint32                     pingTime;   // last ping time
        uint32                     id;         // peer id (MultiplayerSessionMember::MemberId)

        // transmission control, see comment below
        uint32                     txseq;
        uint32                     rxseq;
        uint32                     txseqR;
        uint32                     rxseqR;
        uint32                     rxackR;

        std::vector<PacketHeader*> reliable;   // retransmit queue for reliable packets

        Peer();
        int   setstate( int expect, int replace );

    public:
        void*                 userData;   // whatever you want to put in here
        Platform::Object^     userObject; // or here   
        ~Peer() {}

        uint32  getPeerId() const { return id; }
    };
    /*
     * TODO: write an essay
     * 
     */


    //////////////////////////////////////////////////////////////////////////
    // Packets

#include <pshpack1.h>
    struct PacketHeader
    {
        // fill these out:
        uint16 size;     // entire packet size (incl. this header)
        uint8  type;     // type 0 is reserved, do not use!

        // don't bother with these:
        uint8  flags;    // a combination of Packet_XXX
        uint32 seq;      // sequence # (all)
        uint32 seqR;     // sequence # (last reliable)
        uint32 id;       // peer id (MultiplayerSessionMember::MemberId)
        uint32 timestamp;

        uint8  reserved[32]; // reserved for now
    };


    // defines a packet of maximum length
    // used to recv() an unknown packet
    struct PacketStorage: PacketHeader
    {
        enum { MaxPayloadSize = kMaxUdpSize - sizeof(PacketHeader)  };
        uint8 payload[MaxPayloadSize];
    };
#include <poppack.h>
    //////////////////////////////////////////////////////////////////////////
    // Network


    class Network: public boost::noncopyable
    {
        typedef std::lock_guard<std::mutex>  lock;
        Sock                      sock;

        uint32                    myMemberId; // (MultiplayerSessionMember::MemberId)
        SecureDeviceAssociationTemplate^             assTemplate;
        Windows::Foundation::EventRegistrationToken  incomingHandler;
        volatile long             controlRunFlag; // control thread run/exit flag

        std::mutex                              peerMutex; // only used to manipulate these arrays; use Peer::setstate() to lock individual peers
        std::vector< PeerPtr >                  peers;
        std::vector< SecureDeviceAssociation^ > pendingList;
        boost::thread                           controlThreadObject;

    public:
        Network( MultiplayerSessionMember^ currentUser );
        ~Network();

        PeerPtr createPeer( MultiplayerSessionMember^ member, boost::function<void(PeerPtr)> onConnected );

        void destroyPeer( PeerPtr peer );

        // sends a packet to that peer
        // returns number of bytes sent or -1 on error
        NetResult  sendPacket( PeerPtr peer, PacketHeader* packet, unsigned timeoutMs, unsigned flags );

        // receives a packet
        NetResult  recvPacket( PacketStorage* stor, unsigned timeoutMs );

        // MUST be called after each recvPacket
        // figures our who sent this packet
        NetResult processPacket( PeerPtr& who, PacketHeader* header );

    private:
        void    connectPeer( PeerPtr peer, MultiplayerSessionMember^ member, boost::function<void(PeerPtr)> onConnected ); // call on a separate thread!
        PeerPtr getPeerByAssoc( SecureDeviceAssociation^ incoming );

        
        void          controlThread();
    };
}


