#pragma once

// A simple hash function from Robert Sedgwicks Algorithms in C book. 

namespace RBX {

	typedef enum {INSERT_RAW, INSERT_TO_TREE, INSERT_TO_3D_VIEW} InsertMode;

	typedef enum {
		PUT_TOOL_IN_STARTERPACK, // The user has been prompted about putting a tool into the starter pack
		SUPPRESS_PROMPTS} 
	PromptMode;

}	// namespace