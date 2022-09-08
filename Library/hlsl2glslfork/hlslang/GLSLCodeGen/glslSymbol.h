// Copyright (c) The HLSL2GLSLFork Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.


#ifndef GLSL_SYMBOL_H
#define GLSL_SYMBOL_H

#include "glslCommon.h"
#include "glslStruct.h"

class GlslSymbol 
{
public:
	GlslSymbol( const std::string &n, const std::string &s, int id, EGlslSymbolType t, TPrecision precision, EGlslQualifier q, int as = 0 );

	bool getIsParameter() const { return isParameter; }
	void setIsParameter( bool param ) { isParameter = param; }

	bool getIsGlobal() const { return isGlobal || qual == EqtUniform || qual == EqtMutableUniform; }
	void setIsGlobal(bool global) { isGlobal = global; }
	
	bool getIsMutable() const { return qual == EqtMutableUniform; }

	/// Get mangled name
	const std::string &getName( bool local = true ) const { return ( (local ) ? mutableMangledName : mangledName ); }

	bool hasSemantic() const { return (semantic.size() > 0); }
	const std::string &getSemantic() const { return semantic; }

	int getId() const { return identifier; }

	bool isArray() const { return (arraySize > 0); }
	int getArraySize() const { return arraySize; }

	EGlslSymbolType getType() const { return type; }
	EGlslQualifier getQualifier() const { return qual; }
	TPrecision getPrecision() const { return precision; }

	void updateType( EGlslSymbolType t ) { assert( type == EgstSamplerGeneric); type = t; }

	const GlslStruct* getStruct() const { return structPtr; }
	GlslStruct* getStruct() { return structPtr; }
	void setStruct( GlslStruct *s ) { structPtr = s; }

	enum WriteDeclFlags {
		WRITE_DECL_MUTABLE_UNIFORMS = (1<<0),
	};
	void writeDecl (std::stringstream& out, unsigned flags); // flags = bitmask of WriteDeclFlags
	/// Set the mangled name for the symbol
	void mangleName();    

	void addRef() { refCount++; }
	void releaseRef() { assert (refCount >= 0 ); if ( refCount > 0 ) refCount--; }
	int getRef() const { return refCount; }

private:
	std::string name;
	std::string mangledName;
	std::string mutableMangledName;
	std::string semantic;
	int identifier;
	EGlslSymbolType type;
	EGlslQualifier qual;
	TPrecision precision;
	int arraySize;
	int mangleCounter;
	GlslStruct *structPtr;
	bool isParameter;
	int refCount;
	bool isGlobal;
};

#endif //GLSL_SYMBOL_H

