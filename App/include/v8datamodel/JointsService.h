/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "V8World/World.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/PVInstance.h"
#include "GfxBase/IAdornable.h"
#include "Util/ConcurrencyValidator.h"
#include <boost/shared_ptr.hpp>
#include "V8DataModel/ManualJointHelper.h"

namespace RBX {
	class IAdornableCollector;

	extern const char *const sJointsService;
	class JointsService 
		: public DescribedNonCreatable<JointsService, Instance, sJointsService, Reflection::ClassDescriptor::INTERNAL>
		, public Service
	{
	private:
		typedef DescribedNonCreatable<JointsService, Instance, sJointsService, Reflection::ClassDescriptor::INTERNAL> Super;
		ConcurrencyValidator concurrencyValidator;
		rbx::signals::scoped_connection postInertJointConnection;
		rbx::signals::scoped_connection postDestroyJointConnection;
		rbx::signals::scoped_connection autoJoinConnection;
		rbx::signals::scoped_connection autoDestroyConnection;

		//////////////////////////////////////
		// Instance
		//
		/*override*/ void onDescendantAdded(Instance* instance);
		/*override*/ void onDescendantRemoving(const shared_ptr<Instance>& instance);
		/*override*/ bool askAddChild(const Instance* instance) const {
			return Instance::fastDynamicCast<JointInstance>(instance)!=NULL;
		}
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		void onPostInsertJoint(Joint* joint, Primitive* unGroundedPrim, std::vector<Primitive*>& combiRoots);
		void onPostRemoveJoint(Joint* joint, std::vector<Primitive*>& prim0Roots, std::vector<Primitive*>& prim1Roots);
		void onAutoJoin(Joint* joint);
		void onAutoDestroy(Joint* joint);

		boost::shared_ptr<IAdornableCollector> adornableCollector;
        ManualJointHelper manualJointHelper;
        boost::shared_ptr<PVInstance> joinAfterMoveInstance;
        boost::shared_ptr<PVInstance> joinAfterMoveTarget;
	public:
		World* world;
		JointsService();

        void setJoinAfterMoveInstance(shared_ptr<Instance> value);
        void setJoinAfterMoveTarget(shared_ptr<Instance> value);
        void showPermissibleJoints(void);
        void createJoinAfterMoveJoints(void);
        void clearJoinAfterMoveJoints(void);
	};

} // namespace