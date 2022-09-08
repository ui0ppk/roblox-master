/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/JointsService.h"
#include "V8DataModel/Workspace.h"
#include "GfxBase/IAdornableCollector.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/PartInstance.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "V8World/Mechanism.h"

#include "Network/NetworkOwner.h"
#include "Network/Players.h"

DYNAMIC_FASTFLAG(NetworkOwnershipRuleReplicates);

namespace RBX
{
    const char *const sJointsService = "JointsService";

    REFLECTION_BEGIN();
    static Reflection::BoundFuncDesc<JointsService, void(shared_ptr<Instance>)> func_setJoinAfterMoveInstance(&JointsService::setJoinAfterMoveInstance, "SetJoinAfterMoveInstance", "joinInstance", Security::None);
    static Reflection::BoundFuncDesc<JointsService, void(shared_ptr<Instance>)> func_setJoinAfterMoveTarget(&JointsService::setJoinAfterMoveTarget, "SetJoinAfterMoveTarget", "joinTarget", Security::None);
    static Reflection::BoundFuncDesc<JointsService, void()> func_showPermissibleJoints(&JointsService::showPermissibleJoints, "ShowPermissibleJoints", Security::None);
    static Reflection::BoundFuncDesc<JointsService, void()> func_createJoinAfterMoveJoints(&JointsService::createJoinAfterMoveJoints, "CreateJoinAfterMoveJoints", Security::None);
    static Reflection::BoundFuncDesc<JointsService, void()> func_clearJoinAfterMoveJoints(&JointsService::clearJoinAfterMoveJoints, "ClearJoinAfterMoveJoints", Security::None);
    REFLECTION_END();

/*
	JOINT LIFETIME

	//// CREATION /////
	1.	Joint created in world by joining two parts	// either on client or server)

		a.	Is there already a joint in World between these parts?
				.....
				... Destruction cycle on old joint see #3, #4 below
				.....
		a.	World-> notify AddJoint
		b.	JointService:	onEvent (AddJoint)
				JointInstance::JointInstance(Joint*)
				jointInstance->setParent(JointService)
				JointInstance::

	2.	Joint added by Replication	// On other side - either client or server

		a.	JointInstance created with (joint==NULL, tempPersistJoint != NULL)
		b.  Wait for P0, P1 to be valid, then createJoint();
			// P0, P1, coord - all NULL/Bad
			//
			//	wait until P0, P1, C0, C1, Type received
			// then->insert into world
			// onAncestorChanged with stuff missing




	//// DESTRUCTION

	3.	Joint is destroyed by world by unjoining two parts // either client or server

		a.	World-> notify destroyJoint
		b.	JointService::onDestroyEvent
				If JointInstance-> disassociateJoint (Joint* == NULL)
				Parent->NULL;
		XXX	Joint is laying around with joint == NULL, tempPersistJoint == NULL


	4.	Joint deleted by Replication
		
		a.	JointInstance - Parent is set to NULL
		b.	removeJoint() called
		c.	disassociateJoint();
		d.	world->destroyJoint

		XXX Joint is sitting around with joint and tempPersistJoint == NULL


	/////// SPURIOUS Events

	1.	Joint Deletion from REPLICATION without finding joint?

	2.	Joint Creation from REPLICATION with joint already in existence?
			-> setParent() with !tempPersistJoint and !Joint

	3.	Set Part or Coord properties for Joint already in World -> IGNORE

	4.	Set Coord properties not set when Set Part properties are already set -> WHOLE INSTANCE ATOMIC

	5.	Part destroyed in world->Joint should be destroyed prior to the part being destroyed
		

*/

JointsService::JointsService()
	:Service(true)
	,world(NULL)
{
	setName("JointsService");
	Instance::propArchivable.setValue(this, false);
}

void JointsService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	postInertJointConnection.disconnect();
	postDestroyJointConnection.disconnect();

	autoJoinConnection.disconnect();
	autoDestroyConnection.disconnect();

	world = NULL;

	Super::onServiceProvider(oldProvider, newProvider);

	Workspace* w = ServiceProvider::find<Workspace>(newProvider);
	if (w)
	{
		world = w->getWorld();
		adornableCollector = w->getAdornableCollector();

		postInertJointConnection = world->postInsertJointSignal.connect(boost::bind(&JointsService::onPostInsertJoint, this, _1, _2, _3));
		postDestroyJointConnection = world->postRemoveJointSignal.connect(boost::bind(&JointsService::onPostRemoveJoint, this, _1, _2, _3));

		autoJoinConnection = world->autoJoinSignal.connect(boost::bind(&JointsService::onAutoJoin, this, _1));
		autoDestroyConnection = world->autoDestroySignal.connect(boost::bind(&JointsService::onAutoDestroy, this, _1));
	}
}


void JointsService::onDescendantRemoving(const shared_ptr<Instance>& instance)
{
	if (IAdornable* iR = dynamic_cast<IAdornable*>(instance.get())) {
		adornableCollector->onRenderableDescendantRemoving(iR);
	}

	Super::onDescendantRemoving(instance);
}

void JointsService::onDescendantAdded(Instance* instance)
{
	Super::onDescendantAdded(instance);

	if (IAdornable* iR = dynamic_cast<IAdornable*>(instance)) {
		adornableCollector->onRenderableDescendantAdded(iR);
	}
}

// If created by replication, then association will already exist
// joint was created in world -> create here

void JointsService::onPostInsertJoint(Joint* joint, Primitive* unGroundedPrim, std::vector<Primitive*>& combiRoots)
{
	RBXASSERT(joint);
	RBX::SystemAddress consistentAddress;
	bool hasConsistentOwner;
	bool hasConsistentManualOwnershipRule;
	PartInstance::checkConsistentOwnerAndRuleResetRoots(consistentAddress, hasConsistentOwner, hasConsistentManualOwnershipRule, combiRoots);
	if (hasConsistentOwner)
	{
		if (!unGroundedPrim)
		{
			// At this point prim0 and prim1 should be part of same mechanism;
			RBXASSERT(joint->getPrimitive(0) && joint->getPrimitive(1) 
						? joint->getPrimitive(0)->getMechRoot() == joint->getPrimitive(1)->getMechRoot() 
						: false);
			if (Primitive* prim = joint->getPrimitive(0) ? joint->getPrimitive(0) : joint->getPrimitive(1))
			{
				Primitive* rootPrim = prim->getRootMovingPrimitive();
				PartInstance* rootPart = PartInstance::fromPrimitive(rootPrim);
				if (rootPrim && !PartInstance::isPlayerCharacterPart(rootPart))
				{
					if (DFFlag::NetworkOwnershipRuleReplicates)
					{
						rootPart->setNetworkOwnerNotifyIfServer(consistentAddress, hasConsistentManualOwnershipRule);
					}
					else
					{
						rootPart->setNetworkOwner(consistentAddress);
						if ((RBX::Network::Players::serverIsPresent(this) || DFFlag::NetworkOwnershipRuleReplicates)
							&& hasConsistentManualOwnershipRule)
						{
							rootPart->setNetworkOwnershipRule(NetworkOwnership_Manual);
						}
					}
				}
			}
		}
		else
		{
			if (Primitive* rootPrim = unGroundedPrim->getMechRoot())
			{
				Assembly* rootAssembly = rootPrim->getAssembly();
				for (int i = 0; i < rootAssembly->numChildren(); i++)
				{
					Assembly* child = rootAssembly->getTypedChild<Assembly>(i);
					if (Mechanism::isMovingAssemblyRoot(child))
					{
						Primitive* movingPrimRoot = child->getAssemblyPrimitive();
						PartInstance* movingRootPart = PartInstance::fromPrimitive(movingPrimRoot);
						if (unGroundedPrim->isAncestorOf(movingPrimRoot) && !PartInstance::isPlayerCharacterPart(movingRootPart))
						{
							if (DFFlag::NetworkOwnershipRuleReplicates)
							{
								movingRootPart->setNetworkOwnerNotifyIfServer(consistentAddress, hasConsistentManualOwnershipRule);
							}
							else
							{
								if ((RBX::Network::Players::serverIsPresent(this) || DFFlag::NetworkOwnershipRuleReplicates) 
									&& hasConsistentManualOwnershipRule)
								{
										movingRootPart->setNetworkOwnershipRule(NetworkOwnership_Manual);
								}
							}
						}
					}
				}
			}
		}
	}
}

void JointsService::onPostRemoveJoint(Joint* joint, std::vector<Primitive*>& prim0Roots, std::vector<Primitive*>& prim1Roots)
{
	// We are doing this for each primitive individually BECAUSE 
	// sometimes Joints can be made between a primitive and a NON
	// Primitive, and if we destroy these joints we don't want to
	// ruin the SetNetworkOwner logic

	Primitive* prim0 = joint->getPrimitive(0);
	Primitive* prim1 = joint->getPrimitive(1);
	Primitive* rootMovingPrim0 = NULL;
	Primitive* rootMovingPrim1 = NULL;
	if (prim0 && prim1)
	{
		rootMovingPrim0 = prim0->getRootMovingPrimitive();
		rootMovingPrim1 = prim1->getRootMovingPrimitive();	
		if (rootMovingPrim0 && rootMovingPrim1 && prim0Roots.size())
		{
			// If we got this far, this was a simple case where a non-grounded mechanism split into two non grounded parts
			// After we set owners we need to stop early
			RBXASSERT(prim1Roots.size() == 0);
			for (int i = 0; i < 2; i++)
			{
				Primitive* rootMovingPrim = (i == 0) ? rootMovingPrim0 : rootMovingPrim1;
				PartInstance* rootMovingPart = PartInstance::fromPrimitive(rootMovingPrim);
				if (!PartInstance::isPlayerCharacterPart(rootMovingPart))
				{
					if (DFFlag::NetworkOwnershipRuleReplicates)
					{
						rootMovingPart->setNetworkOwnerNotifyIfServer(prim0Roots[0]->getNetworkOwner(), 
											prim0Roots[0]->getNetworkOwnershipRuleInternal() == NetworkOwnership_Manual);
					}
					else
					{
						rootMovingPart->setNetworkOwner(prim0Roots[0]->getNetworkOwner());
						if ((RBX::Network::Players::serverIsPresent(this) || DFFlag::NetworkOwnershipRuleReplicates) 
							&& (prim0Roots[0]->getNetworkOwnershipRuleInternal() == NetworkOwnership_Manual))
						{
							rootMovingPart->setNetworkOwnershipRule(NetworkOwnership_Manual);					
						}
					}
				}
				else
				{
					rootMovingPart->setNetworkOwnershipRule(NetworkOwnership_Auto);
				}
			}
			return;
		}
	}


	for (int i = 0; i < 2; i++)
	{
		RBX::SystemAddress addressToPass;
		bool hasConsistentAddress;
		bool hasConsistentOwnership;
		std::vector<Primitive*>& primRoots = (i == 0) ? prim0Roots : prim1Roots;
		if (Primitive* rootPrim = (i == 0) ? rootMovingPrim0 : rootMovingPrim1)
		{
			// If RootMovingPrim is non-zero, this means that this assembly is currently un-grounded, which means we should clean-up thep revious roots
			// and migrate appropriate ownership to the new root. If the RootMovingPrim is actually zero, this means that the new assembly has no
			// net change.
			PartInstance::checkConsistentOwnerAndRuleResetRoots(addressToPass, hasConsistentAddress, hasConsistentOwnership, primRoots);
			PartInstance* rootPrimPart = PartInstance::fromPrimitive(rootPrim);
			if (hasConsistentAddress && !PartInstance::isPlayerCharacterPart(rootPrimPart))
			{
				if (DFFlag::NetworkOwnershipRuleReplicates)
				{
					rootPrimPart->setNetworkOwnerNotifyIfServer(addressToPass, hasConsistentOwnership);
				}
				else
				{
					rootPrimPart->setNetworkOwner(addressToPass);
					if ((RBX::Network::Players::serverIsPresent(this) || DFFlag::NetworkOwnershipRuleReplicates) 
						&& hasConsistentOwnership)
					{
						rootPrimPart->setNetworkOwnershipRule(NetworkOwnership_Manual);
					}
				}
			}
		}
	}
}

void JointsService::onAutoJoin(Joint* joint)
{
	// Disabling ConcurrencyValidator:
	// onAutoDestroy might be called as a result of setParent(NULL), so concurrencyValidator will think there's threading issue 
	//WriteValidator validator(concurrencyValidator);

	RBXASSERT(joint);
	RBXASSERT(!joint->getJointOwner());

	boost::shared_ptr<JointInstance> ji;
	switch (joint->getJointType()) 
	{
		case Joint::SNAP_JOINT:		ji = Creatable<Instance>::create<Snap>(joint);		break;

		case Joint::WELD_JOINT:		ji = Creatable<Instance>::create<Weld>(joint);		break;

		case Joint::GLUE_JOINT:		ji = Creatable<Instance>::create<Glue>(joint);		break;

		case Joint::ROTATE_JOINT:	ji = Creatable<Instance>::create<Rotate>(joint);	break;

		case Joint::ROTATE_P_JOINT:	ji = Creatable<Instance>::create<RotateP>(joint);	break;

		case Joint::ROTATE_V_JOINT:	ji = Creatable<Instance>::create<RotateV>(joint);	break;

		default:	RBXASSERT(0);
	}
	ji->setParent(this);
}



// joint was destroyed in World-> destroy it here as well
void JointsService::onAutoDestroy(Joint* joint)
{
	// Disabling ConcurrencyValidator:
	// onAutoDestroy might be called as a result of setParent(NULL) below, so concurrencyValidator will think there's threading issue 
	//WriteValidator validator(concurrencyValidator);
	RBXASSERT(joint);

	IJointOwner* jointOwner = joint->getJointOwner();
	RBXASSERT(jointOwner);

	if (jointOwner)
	{
		static_cast<JointInstance*>(jointOwner)->setParent(NULL);
	}
}

void JointsService::setJoinAfterMoveInstance(shared_ptr<Instance> value)
{
    joinAfterMoveInstance = shared_dynamic_cast<PVInstance>(value);
}

void JointsService::setJoinAfterMoveTarget(shared_ptr<Instance> value)
{
    joinAfterMoveTarget = shared_dynamic_cast<PVInstance>(value);
}

void JointsService::showPermissibleJoints(void)
{
    Workspace* w = ServiceProvider::find<Workspace>(this);
    if (w)
    {
        manualJointHelper.setWorkspace(w);
        manualJointHelper.setDisplayPotentialJoints(true);

        if (joinAfterMoveInstance && joinAfterMoveTarget)
        {
            manualJointHelper.setPVInstanceToJoin(*(joinAfterMoveInstance.get()));
            manualJointHelper.setPVInstanceTarget(*(joinAfterMoveTarget.get()));
	        manualJointHelper.findPermissibleJointSurfacePairs();
        }
    }
}

void JointsService::createJoinAfterMoveJoints(void)
{
    Workspace* w = ServiceProvider::find<Workspace>(this);
    if (w)
    {
        manualJointHelper.setWorkspace(w);
        manualJointHelper.setDisplayPotentialJoints(true);
        if(joinAfterMoveInstance)
        {
            manualJointHelper.setPVInstanceToJoin(*(joinAfterMoveInstance.get()));
            if(joinAfterMoveTarget)
                manualJointHelper.setPVInstanceTarget(*(joinAfterMoveTarget.get()));
            manualJointHelper.findPermissibleJointSurfacePairs();
            manualJointHelper.createJoints();
        }
        joinAfterMoveInstance.reset();
        manualJointHelper.clearAndDeleteJointSurfacePairs();
        manualJointHelper.clearSelectedPrimitives();
    }

}

void JointsService::clearJoinAfterMoveJoints(void)
{
    manualJointHelper.clearAndDeleteJointSurfacePairs();
    manualJointHelper.clearSelectedPrimitives();
}

}
