/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/Adornment.h"
#include "GfxBase/IAdornable.h"

namespace RBX
{
	extern const char* const sSurfaceSelection;

	class SurfaceSelection
		: public DescribedCreatable<SurfaceSelection, PartAdornment, sSurfaceSelection>
	{
	public:
		SurfaceSelection();

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ void render3dAdorn(Adorn* adorn);

		NormalId getSurface() const { return surfaceId; }
		void setSurface(NormalId value);

	private:
		NormalId surfaceId;

	};

}


