#pragma once

#include "V8Kernel/IStage.h"
#include "V8Kernel/BodyPvSetter.h"
#include "boost/scoped_ptr.hpp"
#include "solver/Solver.h"

namespace RBX {

	namespace Profiling
	{
		class CodeProfiler;
	}

	class Connector;
	class Body;
	class Point;
	class KernelData;

	class Kernel : public IStage,
				   public BodyPvSetter
	{
	private:
		static int numKernels;
		int maxBodies;
		bool inStepCode;
		int numLastIterations;
		int numOfMaxIterations;
		float error;
		float maxError;
		bool validateBody(Body* b);
		bool validateConnector(Connector* connector) const;
		bool validateConnectorBody(Body* b) const;

		KernelData* kernelData;

		// Funny Physics
		void stepWorldFunnyPhysics(int worldStepId);
		void stepFunnyPhysics(const Vector3& move);
		void stepFunnyPhysicsBody(Body* b, const Vector3& move);

		Point* searchForDuplicatePoint(Point* tempPoint);

		void preStep();
		void preStepThrottled();
		void stepWorld( boost::uint64_t distDebugTime );
		void stepWorldThrottled( boost::uint64_t debugTime );

		bool usingPGSSolver;

	public:
        PGSSolver pgsSolver;
		Kernel(IStage* upstream);
		~Kernel();

		///////////////////////////////////////////
		// IStage
		/*override*/ IStage::StageType getStageType() const {return IStage::KERNEL_STAGE;}
		
		/*override*/ Kernel* getKernel() {return this;}

		void step(bool throttling, int numThreads, boost::uint64_t debugTime);

		void insertBody(Body* b);
		void insertPoint(Point* p);
		void insertConnector(Connector* c);

		void removeBody(Body* b);
		void removePoint(Point* p);
		void removeConnector(Connector* c);

		// double up on points if same body, position....
		// TODO:  move this to the point class - reference counted pointer
		//
		Point* newPointLocal(class Body* _body, const Vector3& worldPos);
		Point* newPoint(class Body* _body, const Vector3& worldPos);
		void deletePoint(class Point* point);

		////////////////////////////////////////////////////////
		//
		// Debugging Stuff
		//
		void report();	// system energy to log file
		static void reportMemorySizes(); 

		float connectorSpringEnergy() const;
		float bodyPotentialEnergy()  const;
		float bodyKineticEnergy()  const;
		float totalEnergy()  const {
			return connectorSpringEnergy() + bodyPotentialEnergy() + bodyKineticEnergy();
		}
		float totalKineticEnergy()  const {
			return connectorSpringEnergy() + bodyKineticEnergy();
		}

		int numFreeFallBodies()	const;
		int numRealTimeBodies() const;
		int numJointBodies() const;
		int numContactBodies() const;
		int numBodies() const			{return numFreeFallBodies() + numRealTimeBodies() + numJointBodies() + numContactBodies();}
		int numBodiesMax() const		{return maxBodies;}
		int numLeafBodies()	const;
		int numPoints()	const;
		int numConnectors()	const;
		int numHumanoidConnectors() const;
		int numRealTimeConnectors() const;
		int numSecondPassConnectors() const;
		int numJointConnectors() const;
		int numBuoyancyConnectors() const;
		int numContactConnectors() const;

		inline int numIterations() const {return numLastIterations;}
		inline int numMaxIterations() const {return numOfMaxIterations;}
		inline float getSolverError() const {return error;}
		inline float getMaxSolverError() const {return maxError;}
		int fakeDeceptiveSolverIterations() const;
		int fakeDeceptiveMatrixSize() const;

		///////////////////////////////////////////
		// Profiler
		boost::scoped_ptr<Profiling::CodeProfiler> profilingKernelBodies;
		boost::scoped_ptr<Profiling::CodeProfiler> profilingKernelConnectors;

		void setUsingPGSSolver(bool pgsOn) { usingPGSSolver = pgsOn; }
		bool getUsingPGSSolver() const { return usingPGSSolver; }
        void dumpLog( bool enable ) { pgsSolver.dumpLog( enable ); }
	};
} // namespace
