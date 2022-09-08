/**
  @file debugAssert.h
 
  debugAssert(expression);
  debugAssertM(expression, message);
 
  @cite
     John Robbins, Microsoft Systems Journal Bugslayer Column, Feb 1999.
     <A HREF="http://msdn.microsoft.com/library/periodic/period99/feb99_BUGSLAYE_BUGSLAYE.htm">
     http://msdn.microsoft.com/library/periodic/period99/feb99_BUGSLAYE_BUGSLAYE.htm</A>
 
  @cite 
     Douglas Cox, An assert() Replacement, Code of The Day, flipcode, Sept 19, 2000
     <A HREF="http://www.flipcode.com/cgi-bin/msg.cgi?showThread=COTD-AssertReplace&forum=cotd&id=-1">
     http://www.flipcode.com/cgi-bin/msg.cgi?showThread=COTD-AssertReplace&forum=cotd&id=-1</A>
 
  @maintainer Morgan McGuire, matrix@graphics3d.com
 
  @created 2001-08-26
  @edited  2006-01-12

 Copyright 2000-2006, Morgan McGuire.
 All rights reserved.
 */

#ifndef x68BFA40003704acb85BC500AEC18DCA7
#define x68BFA40003704acb85BC500AEC18DCA7 

#include <string>
#include "RbxBase.h"


namespace RBX {
typedef bool (*AssertionHook)(
    const char* _expression,
    const char* filename,
    int lineNumber);


/** 
  Allows customization of the global function invoked when a debugAssert fails.
  The initial value is RBX::_internal::_handleDebugAssert_.  RBX will invoke
  rawBreak if the hook returns true.  If NULL, assertions are not handled.
*/
void setAssertionHook(AssertionHook hook);

AssertionHook assertionHook();

/**
 Called by alwaysAssertM in case of failure in release mode.  If returns
 true then the program exits with -1 (you can replace this with your own
 version that throws an exception or has other failure modes).
 */
void setFailureHook(AssertionHook hook);
AssertionHook failureHook();

namespace _internal {
    extern AssertionHook _debugHook;
    extern AssertionHook _failureHook;
} // internal
} // RBX

#endif
