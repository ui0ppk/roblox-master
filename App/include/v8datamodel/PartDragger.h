/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/PVInstance.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ModelInstance.h"
#include "Script/Script.h"

namespace RBX {

	extern char sPartDragger;
	class PartDragger : public DescribedCreatable<PartDragger, Tool, sPartDragger>
	{
	public:
		PartDragger();
		~PartDragger();

		/*override*/// bool hitTest(const class G3D::Ray& worldRay, G3D::Vector3& worldHitPoint);

		/////////////////////////////////////////////////////////////
		// IRenderable3d
		//
		/*override*/ //bool shouldRender3dAdorn() const;
		/*override*/ //void render3dAdorn(G3D::RenderDevice* rd);
		/*override*/ //void render3dSelect(G3D::RenderDevice* rd, SelectState selectState);

	//	static Reflection::BoundProp<G3D::CoordinateFrame> propGrip;

		/* override */ //bool askSetParent(const Instance* instance) const {return true;}
		/* override */// bool askAddChild(const Instance* instance) const {return true;}

		/*override*/ //void onChildAdded(Instance* child);
		/*override*/ //void onChildRemoving(Instance* child);
/*
		void onPartTouched(shared_ptr<Instance> other);
		void equip();
		void unequip();
		void drop();
*/
	private:

		

	protected:
	//	G3D::CoordinateFrame grip;

		/*override*/ //void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider);
	private:
		//bool areScriptsActive() const;
		/*override*/ //bool ownScript(Script* script, ScriptContext* context);
		/*override*/ //void releaseScript(Script* script);
	};

} // namespace