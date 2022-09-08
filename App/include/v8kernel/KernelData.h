#pragma once

#include "v8kernel/Body.h"
#include "V8Kernel/SimBody.h"
#include "V8Kernel/Point.h"
#include "V8Kernel/Connector.h"
#include "V8Kernel/ContactConnector.h"
#include "V8Kernel/BuoyancyConnector.h"
#include "V8kernel/Constants.h"
#include "V8datamodel/FastLogSettings.h"


namespace RBX {

	class KernelData {
	public:
		// main object types
		IndexArray<SimBody, &SimBody::getFreeFallBodyIndex>		freeFallBodies;	// bodies with no connectors
		IndexArray<SimBody, &SimBody::getRealTimeBodyIndex>		realTimeBodies;	// humanoid bodies that are not throttle-able
		IndexArray<SimBody, &SimBody::getJointBodyIndex>		jointBodies;	// bodies with joint connectors
		IndexArray<SimBody, &SimBody::getContactBodyIndex>		contactBodies;	// bodies with contact connectors but no joint connectors
		IndexArray<Body, &Body::getLeafBodyIndex>				leafBodies;		// need update PV every step, NOT in kernel!
		IndexArray<Point, &Point::getKernelIndex>				points;
		IndexArray<Connector, &Connector::getHumanoidIndex>		humanoidConnectors;		// The humanoids
		IndexArray<Connector, &Connector::getSecondPassIndex>	secondPassConnectors;	// kernel joints
		IndexArray<Connector, &Connector::getRealTimeIndex>		realTimeConnectors;	// connectors on humanoid body parts
		IndexArray<Connector, &Connector::getJointIndex>		jointConnectors;
		IndexArray<Connector, &Connector::getBuoyancyIndex>		buoyancyConnectors;
		IndexArray<Connector, &Connector::getContactIndex>		contactConnectors;
		
		KernelData()
		{
		}

		~KernelData() {
			RBXASSERT(freeFallBodies.size() == 0);
			RBXASSERT(realTimeBodies.size() == 0);
			RBXASSERT(jointBodies.size() == 0);
			RBXASSERT(contactBodies.size() == 0);
			RBXASSERT(leafBodies.size() == 0);
			RBXASSERT(points.size() == 0);
			RBXASSERT(humanoidConnectors.size() == 0);
			RBXASSERT(secondPassConnectors.size() == 0);
			RBXASSERT(realTimeConnectors.size() == 0);
			RBXASSERT(jointConnectors.size() == 0);
			RBXASSERT(buoyancyConnectors.size() == 0);
			RBXASSERT(contactConnectors.size() == 0);
		}

		inline void addLeafBodies(Body* b)
		{
			for (int i = 0; i < b->numChildren(); ++i)
			{
				Body* child = b->getChild(i);
				RBXASSERT(!child->isLeafBody());
				RBXASSERT(child != child->getRoot());
				if (child->connectorUseCount > 0)
				{
					addLeafBody(child);
				}
				addLeafBodies(child);
			}
		}

		inline void insertBody(Body* b)
		{
			RBXASSERT(b->getRoot() == b);
			SimBody* simBody = b->getSimBody();
			RBXASSERT(!b->isLeafBody() && !simBody->isInKernel());
			addBodyToNewList(simBody);
			RBXASSERT(simBody->validateBodyLists());
		}

		inline void removeBody(Body* b)
		{
			RBXASSERT(b->getRoot() == b);
			RBXASSERT(!b->isLeafBody());
			
			SimBody* simBody = b->getSimBody();
			removeBodyFromCurrentList(simBody);
			simBody->clearSymStateAndAccummulator();
			RBXASSERT(simBody->validateBodyLists());
		}

		inline void addConnector(Connector* c, bool pgsOn)
		{
			RBXASSERT(!c->isInKernel());
			Body* body0 = c->getBody(Connector::body0);
			Body* body1 = c->getBody(Connector::body1);
			SimBody* simBody0 = body0 ? body0->getRootSimBody() : NULL;
			SimBody* simBody1 = body1 ? body1->getRootSimBody() : NULL;
			
			Connector::KernelType connectorType = c->getConnectorKernelType();
 			if ((simBody0 == NULL || !simBody0->isInKernel()) &&
				(simBody1 == NULL || !simBody1->isInKernel()) &&
				connectorType != Connector::HUMANOID &&
				connectorType != Connector::JOINT &&
				connectorType != Connector::KERNEL_JOINT)
 				return;

			if (connectorType == Connector::HUMANOID)
			{
				humanoidConnectors.fastAppend(c);
				if (simBody0)
					simBody0->incrementHumanoidConnectorCount();
				if (simBody1)
					simBody1->incrementHumanoidConnectorCount();
			} 
			else if (connectorType == Connector::KERNEL_JOINT)
			{
				secondPassConnectors.fastAppend(c);
				if (simBody0)
					simBody0->incrementSecondPassConnectorCount();
				if (simBody1)
					simBody1->incrementSecondPassConnectorCount();
			} 
			else if (pgsOn && connectorType == Connector::JOINT)
			{
						jointConnectors.fastAppend(c);
						if (simBody0)
							simBody0->incrementJointConnetorCount();
						if (simBody1)
							simBody1->incrementJointConnetorCount();
			} 
			else if (pgsOn && connectorType == Connector::BUOYANCY)
			{
				buoyancyConnectors.fastAppend(c);
				if (simBody0)
					simBody0->incrementBuoyancyConnectorCount();
				if (simBody1)
					simBody1->incrementBuoyancyConnectorCount();
			} 		
			else if ((simBody0 && !simBody0->getBody()->getCanThrottle()) ||
					 (simBody1 && !simBody1->getBody()->getCanThrottle()))
			{
				realTimeConnectors.fastAppend(c);
				if (simBody0)
					simBody0->incrementRealTimeConnectorCount();
				if (simBody1)
					simBody1->incrementRealTimeConnectorCount();
			} 
			else if (!pgsOn && 
					(connectorType == Connector::JOINT ||
					 connectorType == Connector::BUOYANCY ||
					 ((simBody0 && simBody0->isJointBody()) ||  // Contact connectors in touch with joint bodies 
					  (simBody1 && simBody1->isJointBody()))))	 // are considered joint connectors.
			{
				jointConnectors.fastAppend(c);
				if (simBody0)
					simBody0->incrementJointConnetorCount();
				if (simBody1)
					simBody1->incrementJointConnetorCount();
			}
			else
			{
				RBXASSERT(connectorType == Connector::CONTACT);
				contactConnectors.fastAppend(c);
				if (simBody0)
					simBody0->incrementContactConnectorCount();
				if (simBody1)
					simBody1->incrementContactConnectorCount();
			}

			if (body0)
				addConnectorToBody(c, body0);
			if (body1)
				addConnectorToBody(c, body1);

			RBXASSERT(simBody0 == NULL || simBody0->validateBodyLists());
			RBXASSERT(simBody1 == NULL || simBody1->validateBodyLists());
		}

		inline void removeConnector(Connector* c)
		{
			if (!c->isInKernel())
				return;
			
			Body* body0 = c->getBody(Connector::body0);
			Body* body1 = c->getBody(Connector::body1);
			SimBody* simBody0 = body0 ? body0->getRootSimBody() : NULL;
			SimBody* simBody1 = body1 ? body1->getRootSimBody() : NULL;

			if (c->isHumanoid())
			{
				humanoidConnectors.fastRemove(c);
				if (simBody0)
					simBody0->decrementHumanoidConnectorCount();
				if (simBody1)
					simBody1->decrementHumanoidConnectorCount();
			} 
			else if (c->isSecondPass())
			{
				secondPassConnectors.fastRemove(c);
				if (simBody0)
					simBody0->decrementSecondPassConnectorCount();
				if (simBody1)
					simBody1->decrementSecondPassConnectorCount();
			} 
			else if (c->isRealTime())
			{
				realTimeConnectors.fastRemove(c);
				if (simBody0)
					simBody0->decrementRealTimeConnectorCount();
				if (simBody1)
					simBody1->decrementRealTimeConnectorCount();
			} 
			else if (c->isJoint())
			{
				jointConnectors.fastRemove(c);
				if (simBody0)
					simBody0->decrementJointConnetorCount();
				if (simBody1)
					simBody1->decrementJointConnetorCount();
			} 
			else if (c->isBuoyancy())
			{
				buoyancyConnectors.fastRemove(c);
				if (simBody0)
					simBody0->decrementBuoyancyConnectorCount();
				if (simBody1)
					simBody1->decrementBuoyancyConnectorCount();
			}
			else
			{
				RBXASSERT(c->isContact());
				contactConnectors.fastRemove(c);
				if (simBody0)
					simBody0->decrementContactConnectorCount();
				if (simBody1)
					simBody1->decrementContactConnectorCount();

				ContactConnector* conn = static_cast<ContactConnector*>(c);
				PairParams params = conn->getContactPoint();
				if (conn->getReordedSimBody(simBody0, simBody1, params))
					conn->applyContactPointForSymmetryDetection(simBody0, simBody1, params, -1.0f);
			}

			if (body0)
				removeConnectorFromBody(c, body0);
			if (body1)
				removeConnectorFromBody(c, body1);

			RBXASSERT(simBody0 == NULL || simBody0->validateBodyLists());
			RBXASSERT(simBody1 == NULL || simBody1->validateBodyLists());
		}

	private:

		inline void addLeafBody(Body* b)
		{
			RBXASSERT(!b->isLeafBody());
			RBXASSERT(b->connectorUseCount > 0);
			leafBodies.fastAppend(b);
			RBXASSERT(b->isLeafBody());
		}

		inline void removeLeafBody(Body* b)
		{
			RBXASSERT(b->isLeafBody());
			leafBodies.fastRemove(b);
			RBXASSERT(!b->isLeafBody());
		}

		inline void removeLeafBodies(Body* b)
		{
			for (int i = 0; i < b->numChildren(); ++i)
			{
				Body* child = b->getChild(i);
				if (child->isLeafBody())
				{
					removeLeafBody(child);
				}
				removeLeafBodies(child);
			}
		}

		inline void removeBodyFromCurrentList(SimBody* simBody)
		{
			if (simBody->isRealTimeBody())
			{
				removeLeafBodies(simBody->getBody()); 
				realTimeBodies.fastRemove(simBody);
			} else if (simBody->isJointBody())
			{
				removeLeafBodies(simBody->getBody()); 
				jointBodies.fastRemove(simBody);
			} else if (simBody->isContactBody())
			{
				contactBodies.fastRemove(simBody);
			} else if (simBody->isFreeFallBody())
			{
				freeFallBodies.fastRemove(simBody);
				simBody->updateAngMomentum();
			} else
				return;
			simBody->setDt(0.0f);
		}

		inline void addBodyToNewList(SimBody* simBody)
		{
			if (!simBody->getBody()->getCanThrottle())
			{
				if (!simBody->isRealTimeBody())
				{
					removeBodyFromCurrentList(simBody);
					realTimeBodies.fastAppend(simBody);
					simBody->setDt(Constants::kernelDt());
					addLeafBodies(simBody->getBody());
				}
			} else if (simBody->getJointConnectorCount() > 0)
			{
				if (!simBody->isJointBody())
				{
					removeBodyFromCurrentList(simBody);
					jointBodies.fastAppend(simBody);
					simBody->setDt(Constants::kernelDt());
					addLeafBodies(simBody->getBody());
				}
			} else if (simBody->getContactConnectorCount() > 0)
			{
				if (!simBody->isContactBody())
				{
					removeBodyFromCurrentList(simBody);
					contactBodies.fastAppend(simBody);
					simBody->setDt(Constants::freeFallDt());
				}
			} else
			{
				if (simBody->getConnectorCount() == 0)
				{
					if (!simBody->isFreeFallBody())
					{
						removeBodyFromCurrentList(simBody);
						freeFallBodies.fastAppend(simBody);
						simBody->setDt(Constants::freeFallDt());
						simBody->clearSymStateAndAccummulator();		
					}
				} else
				{
					if (!simBody->isJointBody())
					{
						removeBodyFromCurrentList(simBody);
						jointBodies.fastAppend(simBody);
						simBody->setDt(Constants::kernelDt());
						addLeafBodies(simBody->getBody());
					}
				}
			}
		}

		inline void addConnectorToBody(Connector* c, Body* body)
		{
			body->connectorUseCount++;
			SimBody* simBody = body->getRootSimBody();

			if (!simBody->isInKernel())
				return;
			addBodyToNewList(simBody);

			// adds leaf bodies if root is already here and it is subject to the spring solver
			if (body !=  body->getRoot() && !body->isLeafBody() && 
				(simBody->isJointBody() || simBody->isRealTimeBody()))
				addLeafBody(body);
		}

		inline void removeConnectorFromBody(Connector* c, Body* body)
		{
			body->connectorUseCount--;
			
			if (body->isLeafBody() && body->connectorUseCount == 0)
				removeLeafBody(body);

			SimBody* simBody = body->getRootSimBody();
			if (!simBody->isInKernel())
				return;
			addBodyToNewList(simBody);			
		}
	};

} // namespace
