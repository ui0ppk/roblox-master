/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8Kernel/Kernel.h"
#include "V8Kernel/KernelData.h"
#include "V8Kernel/Constants.h"
#include "V8Kernel/Connector.h"
#include "V8Kernel/ContactConnector.h"
#include "rbx/Debug.h"
#include "Util/Profiling.h"
#include "util/Units.h"
#include "rbx/rbxTime.h"

#include "rbx/Profiler.h"

FASTFLAGVARIABLE(UsePGSSolver, false)

namespace RBX {

int Kernel::numKernels = 0;

Kernel::Kernel(IStage* upstream) 
	: IStage(upstream, NULL) 
	, inStepCode(false)
	, profilingKernelBodies(new Profiling::CodeProfiler("Kernel Bodies"))
	, profilingKernelConnectors(new Profiling::CodeProfiler("Kernel Connectors"))
	, kernelData(new KernelData())
	, maxBodies(0)
	, error(0.0f)
	, maxError(0.0f)
	, numLastIterations(0)
	, numOfMaxIterations(0)
	, usingPGSSolver(false)
{
	numKernels++;
}



Kernel::~Kernel() 
{
	numKernels--;
	
	RBXASSERT(!inStepCode);
	delete kernelData;
}

bool Kernel::validateConnector(Connector* connector) const
{
	bool oneInKernel = validateConnectorBody(connector->getBody(Connector::body0));
	oneInKernel = validateConnectorBody(connector->getBody(Connector::body1)) || oneInKernel;
	return oneInKernel;
}

bool Kernel::validateConnectorBody(Body* b) const
{
	if (b)
	{
		if (b->getRootSimBody()->isInKernel())
		{
			return true;
		} else if (b->isLeafBody())
		{
			return true;
		}
	}
	return false;
}

bool Kernel::validateBody(Body* b)
{
	RBXASSERT_VERY_FAST(!inStepCode); 
	RBXASSERT_FISHING(Math::longestVector3Component(b->getBranchForce()) < 1e9f);
	return true;
}


void Kernel::insertBody(Body* b)
{
	RBXASSERT(b->getRootSimBody()->getDt() == 0.0f);
	
	kernelData->insertBody(b);				//bodies.fastAppend(b);
	RBXASSERT(validateBody(b));
	maxBodies = std::max(maxBodies, numBodies());

	if (usingPGSSolver)	
		pgsSolver.addSimBody( b->getRootSimBody(), !b->getCanThrottle() );
}


void Kernel::removeBody(Body* b)
{
	if (usingPGSSolver)
		pgsSolver.removeSimBody( b->getRootSimBody() );

	RBXASSERT(!inStepCode); 
	RBXASSERT((!b->getRootSimBody()->isContactBody() && !b->getRootSimBody()->isFreeFallBody()) ||
				Math::fuzzyEq(b->getRootSimBody()->getDt(), Constants::freeFallDt(), 1e-6f));
	RBXASSERT((b->getRootSimBody()->isContactBody() || b->getRootSimBody()->isFreeFallBody()) ||
				Math::fuzzyEq(b->getRootSimBody()->getDt(), Constants::kernelDt(), 1e-6f));
	kernelData->removeBody(b);		//	kernelData->bodies.fastRemove(b);

}

void Kernel::insertPoint(Point* p)
{
	RBXASSERT(!inStepCode); 
	kernelData->points.fastAppend(p);
}

void Kernel::insertConnector(Connector* c)
{
	RBXASSERT(!inStepCode); 
	kernelData->addConnector(c, usingPGSSolver);
}

void Kernel::removePoint(Point* p)						
{
	RBXASSERT(!inStepCode); 
	kernelData->points.fastRemove(p);
}

void Kernel::removeConnector(Connector* c)				
{
	RBXASSERT(!inStepCode); 
	kernelData->removeConnector(c);
}

int Kernel::numFreeFallBodies()		const	{return kernelData->freeFallBodies.size();}
int Kernel::numRealTimeBodies()		const	{return kernelData->realTimeBodies.size();}
int Kernel::numJointBodies()		const	{return kernelData->jointBodies.size();}
int Kernel::numContactBodies()		const	{return kernelData->contactBodies.size();}
int Kernel::numLeafBodies()			const	{return kernelData->leafBodies.size();}
int Kernel::numPoints()				const	{return kernelData->points.size();}
int Kernel::numHumanoidConnectors()	const	{return kernelData->humanoidConnectors.size();}
int Kernel::numRealTimeConnectors()	const	{return kernelData->realTimeConnectors.size();}
int Kernel::numSecondPassConnectors()	const	{return kernelData->secondPassConnectors.size();}
int Kernel::numJointConnectors()	const	{return kernelData->jointConnectors.size();}
int Kernel::numBuoyancyConnectors()	const	{return kernelData->buoyancyConnectors.size();}
int Kernel::numContactConnectors()	const	{return kernelData->contactConnectors.size();}
int Kernel::numConnectors()			const	{return numRealTimeConnectors() + numSecondPassConnectors() + numJointConnectors() + numContactConnectors() + numBuoyancyConnectors();}

// double up on points if same body, position....
Point* Kernel::newPoint(Body* _body, const Vector3& worldPos) 
{
	RBXASSERT(!inStepCode);

	Point* tempPoint = new Point(_body);
	tempPoint->setWorldPos(worldPos);

	return searchForDuplicatePoint(tempPoint);
}

Point* Kernel::newPointLocal(class Body* _body, const Vector3& localPos)
{
	RBXASSERT(!inStepCode);

	Point* tempPoint = new Point(_body);
	tempPoint->setLocalPos(localPos);

	return searchForDuplicatePoint(tempPoint);
}

Point* Kernel::searchForDuplicatePoint(Point* tempPoint)
{
	for (int i = 0; i < kernelData->points.size(); i++) {
		if (Point::sameBodyAndOffset(*tempPoint, *kernelData->points[i])) {
			kernelData->points[i]->numOwners++;
			delete tempPoint;
			return kernelData->points[i];
		}
	}

	insertPoint(tempPoint);
	return tempPoint;
}



void Kernel::deletePoint(class Point* _point) 
{
	RBXASSERT(!inStepCode);

	if (_point) {
		_point->numOwners--;
		if (_point->numOwners == 0) {
			removePoint(_point);
			delete _point;
		}
	}
}

void Kernel::step(bool throttling, int numThreads, boost::uint64_t debugTime) 
{
	RBXASSERT(!inStepCode);
	inStepCode = true;

	if (throttling)
	{
		preStepThrottled();
		stepWorldThrottled(debugTime);
	} else
	{
		preStep();
		stepWorld(debugTime);
	}
	inStepCode = false;
}

void Kernel::preStep() 
{

    IndexArray<SimBody, &SimBody::getRealTimeBodyIndex>&	realTimeBodies(kernelData->realTimeBodies);	// humanoid bodies that are not throttle-able
	IndexArray<SimBody, &SimBody::getJointBodyIndex>&		jointBodies(kernelData->jointBodies);		// bodies with joint connectors
	IndexArray<SimBody, &SimBody::getContactBodyIndex>&		contactBodies(kernelData->contactBodies);	// bodies with contact connectors
	IndexArray<Connector, &Connector::getContactIndex>&		contactConnectors(kernelData->contactConnectors);

	{
		RBX::Profiling::Mark mark(*profilingKernelBodies, false);
		for (int i = 0; i < realTimeBodies.size(); ++i)
			realTimeBodies[i]->updateIfDirty();
		for (int i = 0; i < jointBodies.size(); ++i)
			jointBodies[i]->updateIfDirty();

		if (!usingPGSSolver)
		{
			for (int i = 0; i < contactBodies.size(); ++i)
			{
				SimBody* simBody = contactBodies[i];
				simBody->updateIfDirty();
				if (simBody->hasExternalForceOrImpulse())
				{
					contactBodies.fastRemove(simBody);
					realTimeBodies.fastAppend(simBody);
					kernelData->addLeafBodies(simBody->getBody());
					simBody->setDt(Constants::kernelDt());
					--i;
				} else
				{
					simBody->updateSymmetricContactState();
					simBody->stepVelocity();
				}
			}
		}
	}
	{
		RBX::Profiling::Mark mark(*profilingKernelConnectors, false);

		if (!usingPGSSolver)
		{
			// TODO: optimize this
			for (int i = 0; i < contactConnectors.size(); ++i)
			{
				ContactConnector* conn = rbx_static_cast<ContactConnector*>(contactConnectors[i]);		
				conn->clearImpulseComputed();
			}
		}
	}
}

void Kernel::preStepThrottled() 
{
	const IndexArray<SimBody, &SimBody::getRealTimeBodyIndex>&	realTimeBodies(kernelData->realTimeBodies);	// humanoid bodies that are not throttle-able
	{
		RBX::Profiling::Mark mark(*profilingKernelBodies, false);
		// forces update of Cofm as well
		for (int i = 0; i < realTimeBodies.size(); ++i)
			realTimeBodies[i]->updateIfDirty();
	}
}

void Kernel::stepWorld( boost::uint64_t debugTime )
{
    RBXPROFILER_SCOPE("Physics", "Kernel::stepWorld");

	IndexArray<SimBody, &SimBody::getFreeFallBodyIndex>&		 freeFallBodies(kernelData->freeFallBodies);// bodies with no connectors
	IndexArray<SimBody, &SimBody::getRealTimeBodyIndex>&		 realTimeBodies(kernelData->realTimeBodies);// humanoid bodies that are not throttle-able
	IndexArray<SimBody, &SimBody::getContactBodyIndex>&			 contactBodies(kernelData->contactBodies);	// bodies with contact connectors but no joint connectors
	const IndexArray<SimBody, &SimBody::getJointBodyIndex>&		 jointBodies(kernelData->jointBodies);		// bodies with joint connectors
	const IndexArray<Body, &Body::getLeafBodyIndex>&			 leafBodies(kernelData->leafBodies);		// need update PV every step, NOT in kernel!
	const IndexArray<Point, &Point::getKernelIndex>&			 points(kernelData->points);
	const IndexArray<Connector, &Connector::getHumanoidIndex>&	 humanoidConnectors(kernelData->humanoidConnectors);	// humanoid
	const IndexArray<Connector, &Connector::getRealTimeIndex>&	 realtimeConnectors(kernelData->realTimeConnectors);	// connectors on humanoid body parts
	const IndexArray<Connector, &Connector::getSecondPassIndex>& secondPassConnectors(kernelData->secondPassConnectors);// kernel joints
	const IndexArray<Connector, &Connector::getJointIndex>&		 jointConnectors(kernelData->jointConnectors);
	const IndexArray<Connector, &Connector::getBuoyancyIndex>&	 buoyancyConnectors(kernelData->buoyancyConnectors);
	const IndexArray<Connector, &Connector::getContactIndex>&	 contactConnectors(kernelData->contactConnectors);

    if( usingPGSSolver )
    {
		std::vector< ContactConnector* > allContactConnectors;

		// humanoid connectors
		for (int j = 0; j < humanoidConnectors.size(); ++j)
			humanoidConnectors[j]->computeForce(false);
		
		// buoyancy
		for (int j = 0; j < buoyancyConnectors.size(); ++j)
			buoyancyConnectors[j]->computeForce(false);

        for (int j = 0; j < secondPassConnectors.size(); ++j) {
            RBXASSERT(validateConnector(secondPassConnectors[j]));
            secondPassConnectors[j]->computeForce(false);
        }

		// contact with humanoids
		for (int j = 0; j < realtimeConnectors.size(); ++j) 
		{
			ContactConnector* contact = static_cast< ContactConnector* >( realtimeConnectors[j] );
			allContactConnectors.push_back( contact );
		}

		// all standard contacts
        for (int j = 0; j < contactConnectors.size(); ++j) 
        {
            ContactConnector* contact = static_cast< ContactConnector* >( contactConnectors[j] );
            allContactConnectors.push_back( contact );
        }

		// joints
        for (int j = 0; j < jointConnectors.size(); ++j) 
        {
            if (jointConnectors[j]->getConnectorKernelType() == Connector::CONTACT)
            {
                ContactConnector* contact = static_cast< ContactConnector* >( jointConnectors[j] );
                allContactConnectors.push_back( contact );
            }
        }

        pgsSolver.solve( allContactConnectors, Constants::worldDt(), debugTime, false );
    }
	else // legacy solver
	{
		///////////////////////////  Free Fall Solver ///////////////////////////
		{
			RBX::Profiling::Mark mark(*profilingKernelBodies, false);
			for (int i = 0; i < freeFallBodies.size(); ++i)
			{
				SimBody* simBody = freeFallBodies[i];
				simBody->updateIfDirty();
				if (simBody->hasExternalForceOrImpulse())
				{
					simBody->updateAngMomentum();
					freeFallBodies.fastRemove(simBody);
					realTimeBodies.fastAppend(simBody);
					simBody->setDt(Constants::kernelDt());
					--i;
				} else
				{
					simBody->stepFreeFall();
				}
			}
		}

		if (contactConnectors.size() > 0)
		{
			RBX::Profiling::Mark mark(*profilingKernelConnectors, false);
			float residualVelocity = 0.0f;
			int i;
			float tolerance = Constants::impulseSolverAccuracy() * 
							  (contactBodies.size() / Constants::impulseSolverAccuracyScalar() + 1);

			for (i = 0; i < Constants::impulseSolverMaxIterations(); ++i)
			{
				residualVelocity = 0.0f;
				for (int j = 0; j < contactConnectors.size(); ++j)
				{
					RBXASSERT(validateConnector(contactConnectors[j]));
					contactConnectors[j]->computeImpulse(residualVelocity);
				}
				residualVelocity /= contactConnectors.size();
				if (residualVelocity < tolerance)
				{
					++i;
					break;
				}		
			}
			numLastIterations = i;
			if (numLastIterations > numOfMaxIterations)
				numOfMaxIterations = numLastIterations;
			error = residualVelocity;
			if (error > maxError)
				maxError = error;
		}

		{
			RBX::Profiling::Mark mark(*profilingKernelBodies, false);
			for (int i = 0; i < contactBodies.size(); ++i)
				contactBodies[i]->stepPosition();
		}

		///////////////////////////  Joint Solver //////////////////////////////
		for (int i = 0; i < Constants::kernelStepsPerWorldStep(); i++) 
		{
			{
				RBX::Profiling::Mark mark(*profilingKernelConnectors, false);

				for (int j = 0; j < points.size(); ++j)
					points[j]->step();
			
				for (int j = 0; j < realtimeConnectors.size(); ++j) {	
					RBXASSERT(validateConnector(realtimeConnectors[j]));
					realtimeConnectors[j]->computeForce(false);
				}

				for (int j = 0; j < jointConnectors.size(); ++j) {	
					RBXASSERT(validateConnector(jointConnectors[j]));
					jointConnectors[j]->computeForce(false);
				}

				for (int j = 0; j < points.size(); ++j)
					points[j]->forceToBody();

				for (int j = 0; j < humanoidConnectors.size(); ++j)
					humanoidConnectors[j]->computeForce(false);

				for (int j = 0; j < secondPassConnectors.size(); ++j) {
					RBXASSERT(validateConnector(secondPassConnectors[j]));
					secondPassConnectors[j]->computeForce(false);
				}
			}
			{
				RBX::Profiling::Mark mark(*profilingKernelBodies, false);
			
				for (int j = 0; j < realTimeBodies.size(); ++j)
					realTimeBodies[j]->step();

				for (int j = 0; j < jointBodies.size(); ++j)
					jointBodies[j]->step();

				// TODO: Remove this later
				const int leafBodiesSize = leafBodies.size();	    
				// TODO: Parallel? Only if using getPV with lock protectiob
				for (int j = 0; j < leafBodiesSize; ++j)
					leafBodies[j]->getPvUnsafe();						// forces Update		  
			}
		}
	}
}

void Kernel::stepWorldThrottled( boost::uint64_t debugTime ) 
{
    RBXPROFILER_SCOPE("Physics", "Kernel::stepWorldThrottled");

	const IndexArray<SimBody, &SimBody::getRealTimeBodyIndex>&	realtimeBodies(kernelData->realTimeBodies);	// humanoid bodies that are not throttle-able
	const IndexArray<Body, &Body::getLeafBodyIndex>&			leafBodies(kernelData->leafBodies);			// need update PV every step, NOT in kernel!
	const IndexArray<Point, &Point::getKernelIndex>&			points(kernelData->points);
	const IndexArray<Connector, &Connector::getRealTimeIndex>&	 realtimeConnectors(kernelData->realTimeConnectors);	// connectors on humanoid body parts
	const IndexArray<Connector, &Connector::getHumanoidIndex>&	humanoidConnectors(kernelData->humanoidConnectors);	// humanoids

	if( usingPGSSolver )
    {
        // Compute forces
		for (int j = 0; j < humanoidConnectors.size(); ++j)
			humanoidConnectors[j]->computeForce(false);

        // Gather the connectors
        std::vector< ContactConnector* > allContactConnectors;
        for (int j = 0; j < realtimeConnectors.size(); ++j) 
        {
            ContactConnector* contact = static_cast< ContactConnector* >( realtimeConnectors[j] );
            allContactConnectors.push_back( contact );
        }

        pgsSolver.solve( allContactConnectors, Constants::worldDt(), debugTime, true );
	}
	else
	{
		///////////////////////////  Joint Solve //////////////////////////////
		for (int i = 0; i < Constants::kernelStepsPerWorldStep(); i++) 
		{
			{
				RBX::Profiling::Mark mark(*profilingKernelConnectors, false);

				for (int j = 0; j < points.size(); ++j)
					points[j]->step();

				for (int j = 0; j < realtimeConnectors.size(); ++j) {	
					RBXASSERT(validateConnector(realtimeConnectors[j]));
					realtimeConnectors[j]->computeForce(false);
				}

				for (int j = 0; j < points.size(); ++j)
					points[j]->forceToBody();

				for (int j = 0; j < humanoidConnectors.size(); ++j)
					humanoidConnectors[j]->computeForce(false);
			}
			{
				RBX::Profiling::Mark mark(*profilingKernelBodies, false);

				for (int j = 0; j < realtimeBodies.size(); ++j)
					realtimeBodies[j]->step();

				// TODO: Remove this later
				const int leafBodiesSize = leafBodies.size();	    
    
				// TODO: Parallel? Only if using getPV with lock protectiob
				for (int j = 0; j < leafBodiesSize; ++j)
					leafBodies[j]->getPvUnsafe();						// forces Update		  
			}
		}
	}
}



float Kernel::connectorSpringEnergy() const 
{
	float springPotential = 0.0;

	for (int j = 0; j < kernelData->secondPassConnectors.size(); ++j)
		springPotential += kernelData->secondPassConnectors[j]->potentialEnergy();

	for (int j = 0; j < kernelData->realTimeConnectors.size(); ++j)
		springPotential += kernelData->realTimeConnectors[j]->potentialEnergy();

	for (int j = 0; j < kernelData->jointConnectors.size(); ++j)
		springPotential += kernelData->jointConnectors[j]->potentialEnergy();

	for (int j = 0; j < kernelData->contactConnectors.size(); ++j)
		springPotential += kernelData->contactConnectors[j]->potentialEnergy();


	return springPotential;
}

float Kernel::bodyPotentialEnergy() const 
{
	float gravitationalPotential = 0.0;

	for (int j = 0; j < kernelData->realTimeBodies.size(); ++j)
		gravitationalPotential += kernelData->realTimeBodies[j]->getBody()->potentialEnergy();

	for (int j = 0; j < kernelData->freeFallBodies.size(); ++j)
		gravitationalPotential += kernelData->freeFallBodies[j]->getBody()->potentialEnergy();

	for (int j = 0; j < kernelData->jointBodies.size(); ++j)
		gravitationalPotential += kernelData->jointBodies[j]->getBody()->potentialEnergy();

	for (int j = 0; j < kernelData->contactBodies.size(); ++j)
		gravitationalPotential += kernelData->contactBodies[j]->getBody()->potentialEnergy();

	return gravitationalPotential;
}

float Kernel::bodyKineticEnergy() const 
{
	float kineticEnergy = 0.0;

    for (int j = 0; j < kernelData->realTimeBodies.size(); ++j)
		kineticEnergy += kernelData->realTimeBodies[j]->getBody()->kineticEnergy();

	for (int j = 0; j < kernelData->freeFallBodies.size(); ++j)
		kineticEnergy += kernelData->freeFallBodies[j]->getBody()->kineticEnergy();

	for (int j = 0; j < kernelData->jointBodies.size(); ++j)
		kineticEnergy += kernelData->jointBodies[j]->getBody()->kineticEnergy();
	
	for (int j = 0; j < kernelData->contactBodies.size(); ++j)
		kineticEnergy += kernelData->contactBodies[j]->getBody()->kineticEnergy();
	return kineticEnergy;
}


void Kernel::report() 
{
}

void Kernel::reportMemorySizes() 
{
}


int Kernel::fakeDeceptiveSolverIterations() const
{
	int fakeMatrixSize = fakeDeceptiveMatrixSize() + 4;

	return 1 + static_cast<int>(sqrt(sqrt(static_cast<float>(fakeMatrixSize))));
}



int Kernel::fakeDeceptiveMatrixSize() const
{
	return numConnectors() + (6 * numBodies());
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// Funny Physics
void Kernel::stepWorldFunnyPhysics(int worldStepId)
{
	if ((worldStepId % Constants::worldStepsPerUiStep()) == 0) 
	{
		int seconds = worldStepId * Constants::worldDt();
		int phase = seconds % 4;
		Vector3 move;
		switch (phase)
		{
		case 0:	move.x = 1.0f;	break;
		case 1: move.y = 1.0f;	break;
		case 2: move.x = -1.0f; break;
		case 3: move.y = -1.0f; break;
		default: break;
		}
		
		move *= 0.4f;
		stepFunnyPhysics(move);
	}
}

void Kernel::stepFunnyPhysicsBody(Body* b, const Vector3& move)
{
	CoordinateFrame c = b->getCoordinateFrame();
	c.translation += move;
	Math::rotateAboutYGlobal(c, 0.01f);
	Math::orthonormalizeIfNecessary(c.rotation);
	b->setCoordinateFrame(c, *this);
}


void Kernel::stepFunnyPhysics(const Vector3& move)
{
	const IndexArray<SimBody, &SimBody::getFreeFallBodyIndex>&	freeFallBodies(kernelData->freeFallBodies);	// bodies with no connectors
	const IndexArray<SimBody, &SimBody::getRealTimeBodyIndex>&	realTimeBodies(kernelData->realTimeBodies);	// humanoid bodies that are not throttle-able
	const IndexArray<SimBody, &SimBody::getJointBodyIndex>&		jointBodies(kernelData->jointBodies);		// bodies with joint connectors
	const IndexArray<SimBody, &SimBody::getContactBodyIndex>&	contactBodies(kernelData->contactBodies);	// bodies with contact connectors but no joint connectors

	for (int i = 0; i < freeFallBodies.size(); ++i) 
		stepFunnyPhysicsBody(freeFallBodies[i]->getBody(), move);

	for (int i = 0; i < realTimeBodies.size(); ++i) 
		stepFunnyPhysicsBody(realTimeBodies[i]->getBody(), move);

	for (int i = 0; i < jointBodies.size(); ++i) 
		stepFunnyPhysicsBody(jointBodies[i]->getBody(), move);

	for (int i = 0; i < contactBodies.size(); ++i) 
		stepFunnyPhysicsBody(contactBodies[i]->getBody(), move);

}



} // namespace
