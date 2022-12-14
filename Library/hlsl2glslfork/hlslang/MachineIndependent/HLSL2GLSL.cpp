// Copyright (c) The HLSL2GLSLFork Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.


#include "SymbolTable.h"
#include "ParseHelper.h"
#include "RemoveTree.h"

#include "InitializeDll.h"

#include "../../include/hlsl2glsl.h"
#include "Initialize.h"
#include "../GLSLCodeGen/hlslSupportLib.h"

#include "../GLSLCodeGen/hlslCrossCompiler.h"
#include "../GLSLCodeGen/hlslLinker.h"


// A symbol table for each language.  Each has a different set of built-ins, and we want to preserve that 
// from compile to compile.
TSymbolTable SymbolTables[EShLangCount];


// Global pool allocator (per process)
TPoolAllocator* PerProcessGPA = 0;

///ACS: added fixedTargetVersion
///     * If left as default (ETargetVersionCount) Hlsl2Glsl operates as normal
///     * If set, only Hlsl2Glsl_Translate calls of matching target will work, and 
///       when set to higher than ETargetGLSL_120, will emit non-deprecated-after-120
///       texture lookup calls e.g. texture() & textureLod() instead of texture2D() & textureCubeLod()
ETargetVersion FixedTargetVersion = ETargetVersionCount;

/// Initializize the symbol table
/// \param BuiltInStrings
///      Pointer to built in strings.
/// \param language
///      Shading language to initialize symbol table for
/// \param infoSink
///      Information sink (for errors/warnings)
/// \param symbolTables
///      Array of symbol tables (one for each language)
/// \param bUseGlobalSymbolTable
///      Whether to use the global symbol table or the per-language symbol table
/// \return
///      True if succesfully initialized, false otherwise
static bool InitializeSymbolTable( TBuiltInStrings* BuiltInStrings, EShLanguage language, TInfoSink& infoSink, 
                            TSymbolTable* symbolTables, bool bUseGlobalSymbolTable )
{
   TSymbolTable* symbolTable;

   if ( bUseGlobalSymbolTable )
      symbolTable = symbolTables;
   else
      symbolTable = &symbolTables[language];

	//@TODO: for now, we use same global symbol table for all target language versions.
	// This is wrong and will have to be changed at some point.
	TParseContext parseContext(*symbolTable, language, ETargetVersionCount, 0, infoSink);

   GlobalParseContext = &parseContext;

   setInitialState();

   assert(symbolTable->isEmpty() || symbolTable->atSharedBuiltInLevel());

   //
   // Parse the built-ins.  This should only happen once per
   // language symbol table.
   //
   // Push the symbol table to give it an initial scope.  This
   // push should not have a corresponding pop, so that built-ins
   // are preserved, and the test for an empty table fails.
   //

   symbolTable->push();

   //Initialize the Preprocessor
   int ret = InitPreprocessor();
   if (ret)
   {
      infoSink.info.message(EPrefixInternalError,  "Unable to intialize the Preprocessor");
      return false;
   }

   for (TBuiltInStrings::iterator i  = BuiltInStrings[parseContext.language].begin();
       i != BuiltInStrings[parseContext.language].end();
       ++i)
   {
      const char* builtInShaders = (*i).c_str();

      if (PaParseString(const_cast<char*>(builtInShaders), parseContext) != 0)
      {
         infoSink.info.message(EPrefixInternalError, "Unable to parse built-ins");
         return false;
      }
   }

   if (  !bUseGlobalSymbolTable )
   {
      IdentifyBuiltIns(parseContext.language, *symbolTable);
   }

   FinalizePreprocessor();

   return true;
}


/// Generate the built in symbol table
/// \param infoSink
///      Information sink (for errors/warnings)
/// \param symbolTables
///      Array of symbol tables (one for each language)
/// \param bUseGlobalSymbolTable
///      Whether to use the global symbol table or the per-language symbol table
/// \param language
///      Shading language to build symbol table for
/// \return
///      True if succesfully built, false otherwise
static bool GenerateBuiltInSymbolTable(TInfoSink& infoSink, TSymbolTable* symbolTables, EShLanguage language)
{
   TBuiltIns builtIns;

   if ( language != EShLangCount )
   {      
      InitializeSymbolTable(builtIns.getBuiltInStrings(), language, infoSink, symbolTables, true);
   }
   else
   {
      builtIns.initialize();
      InitializeSymbolTable(builtIns.getBuiltInStrings(), EShLangVertex, infoSink, symbolTables, false);
      InitializeSymbolTable(builtIns.getBuiltInStrings(), EShLangFragment, infoSink, symbolTables, false);
   }

   return true;
}



int C_DECL Hlsl2Glsl_Initialize(GlobalAllocateFunction alloc, GlobalFreeFunction free, void* user, 
                                ETargetVersion fixedTargetVersion /*= ETargetVersionCount*/)
{
   TInfoSink infoSink;
   bool ret = true;

   SetGlobalAllocationAllocator(alloc, free, user);
	
   if (!InitProcess())
      return 0;

   FixedTargetVersion = fixedTargetVersion; 

   // This method should be called once per process. If its called by multiple threads, then 
   // we need to have thread synchronization code around the initialization of per process
   // global pool allocator
   if (!PerProcessGPA)
   {
      TPoolAllocator *builtInPoolAllocator = new TPoolAllocator(true);
      builtInPoolAllocator->push();
      TPoolAllocator* gPoolAllocator = &GlobalPoolAllocator;
      SetGlobalPoolAllocatorPtr(builtInPoolAllocator);

      TSymbolTable symTables[EShLangCount];
      GenerateBuiltInSymbolTable(infoSink, symTables, EShLangCount);

      PerProcessGPA = new TPoolAllocator(true);
      PerProcessGPA->push();
      SetGlobalPoolAllocatorPtr(PerProcessGPA);

      SymbolTables[EShLangVertex].copyTable(symTables[EShLangVertex]);
      SymbolTables[EShLangFragment].copyTable(symTables[EShLangFragment]);

      SetGlobalPoolAllocatorPtr(gPoolAllocator);

      symTables[EShLangVertex].pop();
      symTables[EShLangFragment].pop();

      initializeHLSLSupportLibrary(FixedTargetVersion);

      builtInPoolAllocator->popAll();
      delete builtInPoolAllocator;        

   }

   return ret ? 1 : 0;
}

void C_DECL Hlsl2Glsl_Shutdown()
{
	DetachProcess();
}

void Hlsl2Glsl_Finalize()
{
   if (PerProcessGPA)
   {
      SymbolTables[EShLangVertex].pop();
      SymbolTables[EShLangFragment].pop();

      PerProcessGPA->popAll();
      delete PerProcessGPA;
      PerProcessGPA = NULL;
      finalizeHLSLSupportLibrary();
   }
}


ShHandle C_DECL Hlsl2Glsl_ConstructCompiler( const EShLanguage language )
{
   if (!InitThread())
      return 0;

   HlslCrossCompiler* compiler = new HlslCrossCompiler(language);
   return compiler;
}

void C_DECL Hlsl2Glsl_DestructCompiler( ShHandle handle )
{
   if (handle == 0)
      return;

   delete handle;
}


int C_DECL Hlsl2Glsl_Parse(
	const ShHandle handle,
	const char* shaderString,
	ETargetVersion targetVersion,
	unsigned options)
{
   if (!InitThread())
      return 0;

   if (handle == 0)
      return 0;

   HlslCrossCompiler* compiler = handle;

   GlobalPoolAllocator.push();
   compiler->infoSink.info.erase();
   compiler->infoSink.debug.erase();

   if (!shaderString)
	   return 1;

   TSymbolTable symbolTable(SymbolTables[compiler->getLanguage()]);

   GenerateBuiltInSymbolTable(compiler->infoSink, &symbolTable, compiler->getLanguage());

   TParseContext parseContext(symbolTable, compiler->getLanguage(), targetVersion, options, compiler->infoSink);

   GlobalParseContext = &parseContext;

   setInitialState();

   InitPreprocessor();    
   //
   // Parse the application's shaders.  All the following symbol table
   // work will be throw-away, so push a new allocation scope that can
   // be thrown away, then push a scope for the current shader's globals.
   //
   bool success = true;

   symbolTable.push();
   if (!symbolTable.atGlobalLevel())
      parseContext.infoSink.info.message(EPrefixInternalError, "Wrong symbol table level");


   int ret = PaParseString(const_cast<char*>(shaderString), parseContext);
   if (ret)
      success = false;

   if (success && parseContext.treeRoot)
   {
		TIntermAggregate* aggRoot = parseContext.treeRoot->getAsAggregate();
		if (aggRoot && aggRoot->getOp() == EOpNull)
			aggRoot->setOperator(EOpSequence);

		if (options & ETranslateOpIntermediate)
			ir_output_tree(parseContext.treeRoot, parseContext.infoSink);

		compiler->TransformAST (parseContext.treeRoot);
		compiler->ProduceGLSL (parseContext.treeRoot, targetVersion, options);
   }
   else if (!success)
   {
      parseContext.infoSink.info.prefix(EPrefixError);
      parseContext.infoSink.info << parseContext.numErrors << " compilation errors.  No code generated.\n\n";
      success = false;
	  if (options & ETranslateOpIntermediate)
         ir_output_tree(parseContext.treeRoot, parseContext.infoSink);
   }

	ir_remove_tree(parseContext.treeRoot);

   //
   // Ensure symbol table is returned to the built-in level,
   // throwing away all but the built-ins.
   //
   while (! symbolTable.atSharedBuiltInLevel())
      symbolTable.pop();

   FinalizePreprocessor();
   //
   // Throw away all the temporary memory used by the compilation process.
   //
   GlobalPoolAllocator.pop();

   return success ? 1 : 0;
}


int C_DECL Hlsl2Glsl_Translate(
	const ShHandle handle,
	const char* entry,
	ETargetVersion targetVersion,
	unsigned options)
{
   if (handle == 0)
      return 0;

   HlslCrossCompiler* compiler = handle;
   compiler->infoSink.info.erase();

   //ACS: added FixedTargetVersion
   if (FixedTargetVersion!=ETargetVersionCount) {
       if(targetVersion!=FixedTargetVersion) {
           compiler->infoSink.info.message(EPrefixError, "Hlsl2Glsl was initialized with fixed target. Requested target does not match.");
           return 0;
       }
   }

	if (!compiler->IsASTTransformed() || !compiler->IsGlslProduced())
	{
		compiler->infoSink.info.message(EPrefixError, "Shader does not have valid object code.");
		return 0;
	}

   bool ret = compiler->GetLinker()->link(compiler, entry, targetVersion, options);

   return ret ? 1 : 0;
}


const char* C_DECL Hlsl2Glsl_GetShader( const ShHandle handle )
{
	if (!handle)
		return 0;
	return handle->GetLinker()->getShaderText();
}


const char* C_DECL Hlsl2Glsl_GetInfoLog( const ShHandle handle )
{
   if (!InitThread())
      return 0;
   if (handle == 0)
      return 0;
   HlslCrossCompiler* base = static_cast<HlslCrossCompiler*>(handle);
   TInfoSink* infoSink = &(base->getInfoSink());
   infoSink->info << infoSink->debug.c_str();
   return infoSink->info.c_str();
}


int C_DECL Hlsl2Glsl_GetUniformCount( const ShHandle handle )
{
	if (!handle)
		return 0;
   const HlslLinker *linker = handle->GetLinker();
   if (!linker)
      return 0;
   return linker->getUniformCount();
}


const ShUniformInfo* C_DECL Hlsl2Glsl_GetUniformInfo( const ShHandle handle )
{
	if (!handle)
		return 0;
   const HlslLinker *linker = handle->GetLinker();
   if (!linker)
      return 0;
   return linker->getUniformInfo();
}


int C_DECL Hlsl2Glsl_SetUserAttributeNames ( ShHandle handle, 
                                             const EAttribSemantic *pSemanticEnums, 
                                             const char *pSemanticNames[], 
                                             int nNumSemantics )
{
	if (!handle)
		return 0;
	HlslLinker* linker = handle->GetLinker();

   for (int i = 0; i < nNumSemantics; i++ )
   {
      bool bError = linker->setUserAttribName ( pSemanticEnums[i], pSemanticNames[i] );

      if ( bError == false )
         return false;
   }

   return true;
}


int C_DECL Hlsl2Glsl_UseUserVaryings ( ShHandle handle, bool bUseUserVaryings )
{
	if (!handle)
		return 0;
	HlslLinker* linker = handle->GetLinker();
   linker->setUseUserVaryings ( bUseUserVaryings );
   return 1;
}


static bool kVersionUsesPrecision[ETargetVersionCount] = {
	true,	// ES 1.00
	false,	// 1.10
	false,	// 1.20
    false,	// 1.40
};

bool C_DECL Hlsl2Glsl_VersionUsesPrecision (ETargetVersion version)
{
	assert (version >= 0 && version < ETargetVersionCount);
	return kVersionUsesPrecision[version];
}
