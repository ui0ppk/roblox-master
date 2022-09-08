/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

//#include "Util/SoundWorld.h"
#include <string>


namespace RBX {

	// A property that computes and caches its value
	template <class Type, class O>
	class ComputeProp
	{
	private:
		Type val;
		bool dirty;
		O* object;
		typedef Type (O::*GetFunc)();
		GetFunc getFunc;

	public:
		ComputeProp(O* object, GetFunc getFunc) : 
			dirty(true),
			object(object),
			getFunc(getFunc)
		{}
		inline Type getValue()
		{
			if (dirty) {
				val = (object->*getFunc)();
				dirty = false;
			}
			return val;
		}
		inline Type getLastComputedValue() const
		{
			RBXASSERT(!dirty);
			return val;
		}

		inline operator Type()
		{
			return getValue();
		}
		inline Type* getValuePointer()
		{
			getValue();
			return &val;
		}
		inline Type& getValueRef()
		{
			getValue();
			return val;
		}
		bool setDirty()
		{
			bool didSomething = !dirty;
			dirty = true;
			return didSomething;
		}
		bool getDirty() const
		{
			return dirty;
		}
	};

}	// namespace RBX