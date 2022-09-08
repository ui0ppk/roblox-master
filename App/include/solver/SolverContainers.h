#pragma  once

#include "solver/SolverConfig.h"
#include "boost/unordered/unordered_map.hpp"
#include "boost/container/vector.hpp"

#include "rbx/DenseHash.h"
#include "util/G3DCore.h"

#include <map>

namespace RBX
{

class SimBody;

// If you want to have the map visible in the debugger, enable this
// #define SOLVER_DEBUG_MAP

template< class K, class T >
struct SolverUnorderedMap
{
#ifdef SOLVER_DEBUG_MAP
    // Use a std map in non-release for easier debugger inspection
    typedef std::map< K, T > Type;
#else
    // This is faster than a std::map as it uses a hash table
    typedef boost::unordered::unordered_map< K, T > Type;
#endif
};

template< class K, class T >
struct SolverOrderedMap
{
    typedef std::map< K, T > Type;
};

//typedef SolverUnorderedMap< const SimBody*, int >::Type BodyIndexation;
typedef DenseHashMap< const SimBody*, int > BodyIndexation;

// It's so that we can use a Vector3 in unions
class Vector3Pod
{
public:
    void operator=( const Vector3& v )
    {
        x = v.x;
        y = v.y;
        z = v.z;
    }

    operator Vector3() const
    {
        return Vector3(x, y, z);
    }

    float dot( const Vector3& v ) const
    {
        return x * v.x + y * v.y + z * v.z;
    }

    Vector3Pod& operator+=( const Vector3& v )
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    float x, y, z;
};

inline Vector3Pod operator*( float s, const Vector3Pod& v )
{
    Vector3Pod r;
    r.x = s * v.x;
    r.y = s * v.y;
    r.z = s * v.z;
    return r;
}

inline Vector3Pod operator+( const Vector3Pod& u, const Vector3Pod& v )
{
    Vector3Pod r;
    r.x = u.x + v.x;
    r.y = u.y + v.y;
    r.z = u.z + v.z;
    return r;
}

}
