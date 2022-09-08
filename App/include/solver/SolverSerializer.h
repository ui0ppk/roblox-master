#pragma once

#include "solver/SolverConfig.h"
#include "solver/SolverContainers.h"
#include "solver/DebugSerializer.h"

#include "util/standardout.h"

#include "boost/filesystem.hpp"

namespace RBX
{

class SolverSerializer
{
public:
    SolverSerializer(): enabled( false ), fileOpened( false ) { }

    void update( bool _enabled, int userId, boost::uint64_t debugTime )
    {
#ifdef ENABLE_SOLVER_DEBUG_SERIALIZER
        bool switchState = ( enabled != _enabled );
        bool close = false;
        if( switchState )
        {
            if( !enabled )
            {
                enabled = true;
            }
            else
            {
                enabled = false;
                close = true;
            }
        }
        else if( !enabled )
        {
            return;
        }

        static size_t bufferSize = 10 * 1024 * 1024;

        if( !fileOpened )
        {
            debugSerializer.data.reserve( bufferSize + 1024 * 1024 );
            boost::filesystem::path path = boost::filesystem::temp_directory_path();
            path /= "ROBLOX";
            path /= "SolverLog_Client";
            path += boost::lexical_cast<std::string>( userId );
            path += ".bin";
            myFile.open (path.c_str(), std::ios::out | std::ios::binary);
            fileOpened = true;
            debugSerializer & userId;
        }

        if( close || debugSerializer.data.size() > bufferSize )
        {
            myFile.write( debugSerializer.data.data(), debugSerializer.data.size() );
            debugSerializer.data.clear();
        }

        if( close && fileOpened )
        {
            close = false;
            enabled = false;
            myFile.close();
            fileOpened = false;
        }

        static boost::uint64_t frame = 0;
        if( enabled )
        {
            debugSerializer & frame;
            debugSerializer & debugTime;
            frame++;
        }
#endif
    }

    void serializeConstraints( const ArrayBase< Constraint* >& connectors )
    {
        if( enabled )
        {
            debugSerializer.tag("Connectors");
            debugSerializer & (boost::uint32_t)connectors.size();
            for( const auto* c : connectors )
            {
                debugSerializer  & (boost::uint8_t)c->getType() & c;
            }
        }
    }

    void serializeForces( const ArrayBase< SimBody* >& simBodies, boost::uint32_t total )
    {
        if( enabled )
        {
            debugSerializer.tag("Forces");
            debugSerializer & total;
            size_t i = 0;
            for( i = 0; i < simBodies.size(); i++ )
            {
                debugSerializer & simBodies[ i ]->getForce();
                debugSerializer & simBodies[ i ]->getTorque();
                debugSerializer & simBodies[ i ]->getImpulse();
                debugSerializer & simBodies[ i ]->getRotationallmpulse();
            }
            Vector3 zero = Vector3::zero();
            for( ; i < total; i++ )
            {
                debugSerializer & zero & zero & zero & zero;
            }
        }
    }

    void serializeComputedImpulse( const ArrayBase< ConstraintVariables >& velocityStage, const ArrayBase< ConstraintVariables >& positionStage )
    {
        if(enabled)
        {
            ArrayDynamic< float > impulses;
            impulses.reserve(velocityStage.size());
            for( const auto& v : velocityStage )
            {
                impulses.push_back(v.impulse);
            }
            debugSerializer & impulses;
            impulses.clear();
            impulses.reserve(velocityStage.size());
            for( const auto& v : positionStage )
            {
                impulses.push_back(v.impulse);
            }
            debugSerializer & impulses;
        }
    }

    template< class Cache >
    void serializeBodyCache( const Cache& bodyCache )
    {
        if( enabled )
        {
            debugSerializer & boost::uint32_t( bodyCache.size() );
            for( const auto& it : bodyCache )
            {
                debugSerializer & (boost::uint32_t)it.second.simBodyDebug->getBody()->getGuidIndex();
                debugSerializer & it.second;
            }
        }
    }

    template< class T >
    SolverSerializer& operator&( const T& t )
    {
#ifdef ENABLE_SOLVER_DEBUG_SERIALIZER
        if( enabled )
        {
            debugSerializer & t;
        }
#endif
        return *this;
    }

    template< class T >
    SolverSerializer& operator&( const ArrayDynamic< T >& t )
    {
#ifdef ENABLE_SOLVER_DEBUG_SERIALIZER
        if( enabled )
        {
            debugSerializer & static_cast< const ArrayBase< T >& >( t );
        }
#endif
        return *this;
    }

    SolverSerializer& tag( const char* name )
    {
        if( enabled )
        {
            debugSerializer.tag(name);
        }
        return *this;
    }

    std::ofstream myFile;
    bool fileOpened;
    bool enabled;
    DebugSerializer debugSerializer;
};

}
