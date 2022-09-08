/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "GfxBase/IAdornable.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ModelInstance.h"

namespace RBX {
	namespace Network {
		class Players;
		class Player;
	}

	class PartInstance;
	class ModelInstance;

	extern const char* const sClickDetector;
	class ClickDetector : public DescribedCreatable<ClickDetector, Instance, sClickDetector>
					, public IAdornable
	{
	private:
		int cycle;
	    float maxActivationDistance; // max distance a character can be from the button and still raise events
		shared_ptr<Instance> lastHoverPart;

		// IAdornable
		/*override*/ bool shouldRender3dAdorn() const {return true;}
		/*override*/ void render3dAdorn(Adorn* adorn);

	public:
		ClickDetector();
		virtual ~ClickDetector() {}

		void fireMouseClick(float distance, RBX::Network::Player* player);
		void fireMouseHover(RBX::Network::Player* player);
		void fireMouseHoverLeave(RBX::Network::Player* player);
		static Reflection::BoundProp<float> propMaxActivationDistance;

		shared_ptr<Instance> getLastHoverPart() { return lastHoverPart; }
		bool updateLastHoverPart(shared_ptr<Instance> newHover, RBX::Network::Player* player);

		rbx::remote_signal<void(shared_ptr<Instance>)> mouseClickSignal;
		rbx::remote_signal<void(shared_ptr<Instance>)> mouseHoverSignal;
		rbx::remote_signal<void(shared_ptr<Instance>)> mouseHoverLeaveSignal;

		static int cycles() {return 30;}

		float getMaxActivationDistance() {return maxActivationDistance;}

		static bool isClickable(shared_ptr<PartInstance> part, float distanceToCharacter, bool raiseClickedEvent, RBX::Network::Player* player);
		static bool isHovered(PartInstance *part, float distanceToCharacter, bool raiseHoveredEvent, RBX::Network::Player* player);
		static void stopHover(shared_ptr<PartInstance> part, RBX::Network::Player* player);
		/* override */ bool askSetParent(const Instance* parent) const {return (Instance::fastDynamicCast<PartInstance>(parent) != NULL) || (Instance::fastDynamicCast<ModelInstance>(parent) != NULL);}
		/* override */ bool askAddChild(const Instance* instance) const {return true;}

		
	};
}	// namespace RBX