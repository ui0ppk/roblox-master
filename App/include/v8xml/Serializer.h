#ifndef V8XML_SERIALIZER_H
#define V8XML_SERIALIZER_H

#pragma once

#include "SerializerV2.h"

#include "util/SoundService.h"

#include "v8datamodel/Workspace.h"
#include "v8datamodel/Lighting.h"
#include "v8datamodel/ServerStorage.h"
#include "v8datamodel/ReplicatedStorage.h"
#include "v8datamodel/ReplicatedFirst.h"
#include "v8datamodel/PlayerGui.h"
#include "v8datamodel/Hopper.h"
#include "v8datamodel/StarterPlayerService.h"
#include "v8datamodel/ServerScriptService.h"
#include "v8datamodel/CSGDictionaryService.h"

class Serializer : public SerializerV2 
{
public:
	static bool canWriteChild(const shared_ptr<RBX::Instance> instance, RBX::Instance::SaveFilter saveFilter)
	{
		if(!instance->getIsArchivable())
			return false;

		switch(saveFilter)
		{
		case RBX::Instance::SAVE_ALL:
			return true;

		case RBX::Instance::SAVE_WORLD:
			if ( RBX::Instance::fastDynamicCast<RBX::Workspace>(instance.get()) )
				return true;
			if ( RBX::Instance::fastDynamicCast<RBX::Lighting>(instance.get()) )
				return true;
			if ( RBX::Instance::fastDynamicCast<RBX::Soundscape::SoundService>(instance.get()) )
				return true;
			if ( RBX::Instance::fastDynamicCast<RBX::ServerStorage>(instance.get()) )
				return true;
			if ( RBX::Instance::fastDynamicCast<RBX::ReplicatedStorage>(instance.get()) )
				return true;
			if ( RBX::Instance::fastDynamicCast<RBX::CSGDictionaryService>(instance.get()) )
				return true;

			return false;

		case RBX::Instance::SAVE_GAME:
			if ( RBX::Instance::fastDynamicCast<RBX::StarterGuiService>(instance.get()) )
				return true;
			if ( RBX::Instance::fastDynamicCast<RBX::StarterPackService>(instance.get()) )
				return true;
			if ( RBX::Instance::fastDynamicCast<RBX::StarterPlayerService>(instance.get()) )
				return true;
			if ( RBX::Instance::fastDynamicCast<RBX::ServerScriptService>(instance.get()) )
				return true;
			if ( RBX::Instance::fastDynamicCast<RBX::ReplicatedFirst>(instance.get()) )
				return true;

			return false;

		default:
			return true;
		}
	}
};



#endif