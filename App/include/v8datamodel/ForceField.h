/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "Util/RunStateOwner.h"
#include "GfxBase/IAdornable.h"
#include "V8DataModel/Effect.h"

namespace RBX {
	
	class PartInstance;

	static const float largeSize = 1.1f;

	extern const char* const sForceField;
	class ForceField : public DescribedCreatable<ForceField, Instance, sForceField>
					, public IAdornable
					, public Effect
	{
	private:
		//typedef DescribedCreatable<ForceField, Instance, sForceField> Super;

		Time startTime;
		int cycle;
		int invertCycle;
		shared_ptr<Instance> torso;

		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const;

		// IAdornable
		/*override*/ bool shouldRender3dAdorn() const {return true;}
		/*override*/ void render3dAdorn(Adorn* adorn);

	public:
		ForceField();
		virtual ~ForceField() {}

		static bool partInForceField(PartInstance* part);
		static int cycles() {return 60;}
	};
}	// namespace RBX