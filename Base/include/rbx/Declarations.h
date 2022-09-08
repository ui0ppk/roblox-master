#pragma once

/*

http://msdn.microsoft.com/en-us/magazine/cc301398.aspx

	These days abstract classes are 
not just common, they're ubiquitous. Think: where in Windows® do 
abstract classes appear over and over again? That's right, in COM! 
A COM interface is an abstract class with only pure virtual functions. 
As everything in Windows migrates to COM land, Windows-based programs 
have COM interfaces up the wazoo. A typical COM class might implement 
a dozen or more interfaces, each with several functions.
      Even outside COM, the notion of an interface is quite powerful 
and useful, as in the Java language. Each interface implementation might 
use several layers of classes, none intended to be used by themselves, 
but only as base classes for yet more classes. ATL provides many such 
classes using templates, another source of class proliferation. All of 
this adds up to lots of initialization code and useless vtables with 
NULL entries. The total bloat can become significant, especially when 
you're developing small objects that must load over a slow medium like 
the Internet. 
  So __declspec(novtable) was invented to solve the problem. It's a 
Microsoft-specific optimization hint that tells the compiler: this class 
is never used by itself, but only as a base class for other classes, so 
don't bother with all that vtable stuff, thank you.
*/

#ifdef _WIN32

// Decoration to indicate a class is to be treated as an "Interface"
// The class should contain pure virtual functions and maybe a little
// trivial code. Otherwise, use RBXBaseClass.
// !!! You can't define a virtual destructor for a class of this type
#define RBXInterface	__declspec(novtable)

// Decoration to indicate a class should not be instantiated directly
// !!! You can't define a virtual destructor for a class of this type
#define RBXBaseClass	__declspec(novtable)

/****
Note:

C++ doesn't have a strict "Interface" type.  RBXInterface should be used
for classes that declare only pure virtual functions and maybe a constructor
and/or a field.  Classes that define non-trivial code should use RBXBaseClass instead.

***/

#else

#define RBXInterface 
#define RBXBaseClass 

#endif


