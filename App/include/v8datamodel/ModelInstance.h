/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/Camera.h"
#include "V8DataModel/PVInstance.h"
#include "V8DataModel/IModelModifier.h"
#include "GfxBase/IAdornable.h"
#include "GfxBase/Part.h"
#include "Util/CameraSubject.h"
#include "Util/Selectable.h"

#include "boost/optional/optional.hpp"

namespace RBX {

	class IModelModifier;

	extern const char* const sModel;
	class ModelInstance 
		: public DescribedCreatable<ModelInstance, PVInstance, sModel>
		, public IAdornable
		, public CameraSubject
		, public Diagnostics::Countable<ModelInstance>
	{
	private:
		typedef DescribedCreatable<ModelInstance, PVInstance, sModel> Super;

	public:
		static bool showModelCoordinateFrames;
		bool lockedName;

		static void hackPhysicalCharacter();

		void translateBy(Vector3 offset);

		CoordinateFrame getPrimaryCFrame();
		void setPrimaryCFrame(CoordinateFrame newCFrame);

		/////////////////////////////////////////////////////////////
		// Instance
		//
		/*override*/ void setName(const std::string& value);
	private:
		PartInstance* computedPrimaryPart;							// should always be safe	
		weak_ptr<PartInstance> primaryPartSetByUser;				// any risk this could be set outside the model?
		CoordinateFrame modelInPrimary;

		boost::optional<CoordinateFrame> modelCoordinateFrame;
		boost::optional<Extents> primaryPartSpaceExtents;

		CoordinateFrame modelCFrame;
		Vector3 modelSize;

		bool needsSizeRecalc;
		bool needsCFrameRecalc;

		static G3D::Color4 sPrimaryPartHoverOverColor;
        static G3D::Color4 sPrimaryPartSelectColor;
		static float sPrimaryPartLineThickness;

		void computePrimaryPart();

		void dirtyAll();
		std::vector<Instance*> modelModifiers;				// child model modifiers

		Extents computeExtentsLocal(const CoordinateFrame& modelLocation);

		void setExtentsDirty();

	protected:
		/////////////////////////////////////////////////////////////
		// IAdornable3d
		//
		/*override*/ bool shouldRender3dAdorn() const;
		/*override*/ void render3dAdorn(Adorn* adorn);
		/*override*/ void render3dSelect(Adorn* adorn, SelectState selectState);

		/////////////////////////////////////////////////////////////
		// Instance
		//
		/*override*/ void onDescendantAdded(Instance* instance);
		/*override*/ void onDescendantRemoving(const shared_ptr<Instance>& instance);
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ void onChildAdded(Instance* child);
		/*override*/ void onChildRemoving(Instance* child);
		/*override*/ void onChildChanged(Instance* instance, const PropertyChanged& event);

		// PVInstance
		/*override*/ bool hitTestImpl(const class RBX::RbxRay& worldRay, G3D::Vector3& worldHitPoint);

	public:
		ModelInstance();
		~ModelInstance();

		// Legacy - just storing for now
		void setPrimaryPartSetByUser(PartInstance* set);
		PartInstance* getPrimaryPartSetByUser() const;

		void lockName() { lockedName = true; }

		void setModelInPrimary(const CoordinateFrame& set) {modelInPrimary = set;}
		const CoordinateFrame& getModelInPrimary() const {return modelInPrimary;}

		Part computePart();

		Vector3 calculateModelSize();
		CoordinateFrame calculateModelCFrame();
		void setIdentityOrientation();
		void resetOrientationToIdentity();

		void setInsertFlash(bool value);

		////////////////////////////////////////////////////////////////////////
		// IHasLocation
		//
		/*override*/ const CoordinateFrame getLocation();

		////////////////////////////////////////////////////////////////////////
		// CameraSubject
		//
		/*override*/ const CoordinateFrame getRenderLocation() {return getLocation();}
		/*override*/ const Vector3 getRenderSize()	{return calculateModelSize();}
		/*override*/ void onCameraNear(float distance);
		/*override*/ void getCameraIgnorePrimitives(std::vector<const Primitive*>& primitives);

		////////////////////////////////////////////////////////////////////////
		// IHasPrimaryPart
		//
		/*override*/ PartInstance* getPrimaryPart();

		////////////////////////////////////////////////////////////////////////
		// PVInstance
		//
		/*override*/ Extents computeExtentsWorld() const;

        //////////////////////////////////////////////////////////////////////////
        // Selectable
        //
        /*override*/ bool isSelectable3d();

		/////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////

		bool containedByFrustum(const class RBX::Frustum& frustrum) const;
		void setFrontAndTop(const Vector3& front);

		void makeJoints();
		void breakJoints();

		// util
		int computeNumParts() const;			// fast way of counting parts
		float computeTotalMass() const;			// kg
		float computeLargestMoment() const;		// kg*rbx^2
		std::vector<PartInstance*> getDescendantPartInstances() const;

		static const G3D::Color4& primaryPartSelectColor() { return sPrimaryPartSelectColor; }
		static void setPrimaryPartSelectColor(const G3D::Color4& color) { sPrimaryPartSelectColor = color; }

        static const G3D::Color4& primaryPartHoverOverColor() { return sPrimaryPartHoverOverColor; }
		static void setPrimaryPartHoverOverColor(const G3D::Color4& color) { sPrimaryPartHoverOverColor = color; }

		static float primaryPartLineThickness() { return sPrimaryPartLineThickness; }
		static void setPrimaryPartLineThickness(float thickness) { sPrimaryPartLineThickness = thickness; }

		///////////////////////////////////////////////////////
		// Group
		template<class _InIt>
		shared_ptr<ModelInstance> group(_InIt _First, _InIt _Last)
		{
			// TODO: Exception handling
			// TODO: safety: Make a copy of the collection and use THandles?
			shared_ptr<ModelInstance> newModelInstance = ModelInstance::createInstance();

			if (!newModelInstance->canSetChildren(_First, _Last))
				throw std::runtime_error("The requested items cannot be grouped together");

			ModelInstance* commonModel = NULL;
			{
				_InIt iter = _First;
				if (iter != _Last)
				{
					Instance* commonParent = (*iter)->getParent();
					for (++iter; iter != _Last; ++iter)
						commonParent = Instance::findCommonNode(commonParent, (*iter)->getParent());

					for (; commonParent; commonParent = commonParent->getParent())
					{
						commonModel = Instance::fastDynamicCast<ModelInstance>(commonParent);
						if (commonModel)
							break;
					}
				}
			}
			if (!commonModel)
				commonModel = this;

			newModelInstance->setParent(commonModel);

			for (; _First != _Last; ++_First)
				(*_First)->setParent(newModelInstance.get());

			return newModelInstance;
		}								 

		// Find Modifier
		template<class C>
		C* findFirstModifierOfType() 
		{
			std::vector<Instance*>::iterator end = modelModifiers.end();
			for (std::vector<Instance*>::iterator iter = modelModifiers.begin(); iter!=end; ++iter)
				if (C* c = Instance::fastDynamicCast<C>(*iter))
					return c;

			return NULL;
		}

		// Find Modifier
		template<class C>
		const C* findConstFirstModifierOfType() const
		{
			std::vector<Instance*>::const_iterator end = modelModifiers.end();
			for (std::vector<Instance*>::const_iterator iter = modelModifiers.begin(); iter!=end; ++iter)
				if (const C* c = Instance::fastDynamicCast<C>(*iter))
					return c;
			return NULL;
		}


		// Find Modifier - static version that takes any instance and checks if its a model
		template<class C>
		static C* findFirstModifierOfType(Instance* instance)
		{
			if (instance) {
				if (ModelInstance* m = instance->fastDynamicCast<ModelInstance>()) {
					return m->findFirstModifierOfType<C>();
				}
			}
			return NULL;
		}

		// Find Modifier - static version that takes any instance and checks if its a model
		template<class C>
		static const C* findConstFirstModifierOfType(const Instance* instance)
		{
			if (instance) {
				if (const ModelInstance* m = instance->fastDynamicCast<ModelInstance>()) {
					return m->findConstFirstModifierOfType<C>();
				}
			}
			return NULL;
		}

	};

} // namespace
