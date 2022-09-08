#pragma once
// This file is used only to build a very special in-house version of Roblox Studio needed to develop Xbox-specific CoreScripts.

// Set to 1 to enable the special build. DO NOT SUBMIT TO CI OR TRUNK WITH THIS SET TO ON.
#define ENABLE_XBOX_STUDIO_BUILD 0


#ifndef _MSC_VER // visual studio only
#   undef  ENABLE_XBOX_STUDIO_BUILD
#   define ENABLE_XBOX_STUDIO_BUILD 0
#endif
