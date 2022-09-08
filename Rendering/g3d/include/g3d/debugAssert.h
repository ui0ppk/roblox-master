#ifndef G3D_DEBUGASSERT_H
#define G3D_DEBUGASSERT_H

// This stuff is only used in G3D, so I've taken out all those glorious asserts with bells and whistles and
// left this barely functional husk behind.
//   -- Max


#ifdef _MSC_VER
// conditional expression is constant
#   pragma warning (disable : 4127)
#endif

namespace G3D {

#ifdef G3D_DEBUG

#    if defined(_MSC_VER) 
#       define rawBreak()  __debugbreak()
#    else
#       define rawBreak() __builtin_trap()
#    endif


#    define debugBreak() rawBreak()
#    define debugAssert(exp) ( (void) ((exp) || (rawBreak(),1)) )
#    define debugAssertM(exp, message) debugAssert(exp) 
#    define alwaysAssertM debugAssertM

#else  // Release

// In the release build, just define away assertions.
#   define rawBreak() do {} while (0)
#   define debugAssert(exp) do {} while (0)
#   define debugAssertM(exp, message) do {} while (0)
#   define debugBreak() do {} while (0)

// But keep the 'always' assertions
#   define alwaysAssertM(exp, message) do {} while (0)

#endif  // if debug


};  // namespace

#endif
