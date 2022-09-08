#pragma once

#include "rbx/TaskScheduler.h"
#include "v8Tree/Instance.h"
#include "v8Tree/Service.h"

#include "boost/bind.hpp"
#include "boost/function.hpp"

#include <queue>

// TODO: Refactor: Move this out of Util
namespace RBX
{
	class DataModel;
	class Region2;

	class Stepped
	{
	public:
		const double gameTime;
		const double gameStep;
        const bool longStep;
		Stepped(double gameTime, double gameStep, bool longStep):gameTime(gameTime),gameStep(gameStep),longStep(longStep) {}
	};

	class Heartbeat 		// This event occurs at regular intervals
	{
	public:
		const double wallTime;
		const double wallStep;
		const double gameTime;
		const double gameStep;

		const Time expirationTime;
		Heartbeat(double wallTime, double wallStep, double gameTime, double gameStep, Time expirationTime):wallTime(wallTime),gameTime(gameTime),wallStep(wallStep),gameStep(gameStep),expirationTime(expirationTime) {}
	};

	// note - for now, in low to high precedence, for when used as events && multiple events
	enum RunState {		RS_STOPPED, 
						RS_RUNNING, 
						RS_PAUSED};

	class RunTransition {
	public:
		RunState oldState;
		RunState newState;
		RunTransition(RunState oldState, RunState newState) 
			: oldState(oldState), newState(newState)
		{}
	};

	extern const char* const sRunService;

class PhysicsJob;
class HeartbeatTask;

namespace Lua
{
	class WeakFunctionRef;
}

	class RunService 
		: public DescribedNonCreatable<RunService, Instance, sRunService, Reflection::ClassDescriptor::RUNTIME_LOCAL>
		, public Service
	{
	private:
		typedef DescribedNonCreatable<RunService, Instance, sRunService, Reflection::ClassDescriptor::RUNTIME_LOCAL> Super;

		typedef std::pair<std::string, Lua::WeakFunctionRef>  FunctionNameRefPair;
		typedef std::map<int,std::vector<FunctionNameRefPair> > EventCallbackMap;

		friend class PhysicsJob;
		shared_ptr<PhysicsJob> physicsJob;
		friend class HeartbeatTask;
		shared_ptr<HeartbeatTask> heartbeatTask;
#ifdef RBX_TEST_BUILD
        friend class DummyTask;
        std::vector<shared_ptr<DummyTask> > dummyTasks;
#else
        std::vector<shared_ptr<void*> > dummyTasksPadding;
#endif

		RunState runState;
		double totalGameTime;			// i.e. time in game
		double totalGameTimeAtLastHeartbeat;
		double totalWallTime;
		double skippedTimeAccumulated;

		friend class DataModel;

		void stepDataModel();

		EventCallbackMap renderSteppedEarlyCallbackMap;
	public:
		typedef enum
		{
			RENDERPRIORITY_FIRST = 0,
			RENDERPRIORITY_INPUT = 100,
			RENDERPRIORITY_CAMERA = 200,
			RENDERPRIORITY_CHARACTER = 300,
			RENDERPRIORITY_LAST = 2000,
		} RenderPriority;

		RunService();
		~RunService();

		static bool parallelPhysicsUserEnabled;

		rbx::signal<void(const Stepped&)> highPrioritySteppedSignal;
		rbx::signal<void(const Stepped&)> steppedSignal;
		rbx::signal<void(const Stepped&)> renderSteppedSignal;
		rbx::signal<void()>				  earlyRenderSignal;
		rbx::signal<void(const Heartbeat&)> heartbeatSignal;
		rbx::signal<void(RunTransition)> runTransitionSignal;

		TaskScheduler::Job* getPhysicsJob();
		TaskScheduler::Job* getHeartbeat();

		rbx::signal<void(double, double)> scriptSteppedSignal;
		rbx::signal<void(double)> scriptHeartbeatSignal;
		rbx::signal<void(double)> scriptRenderSteppedSignal;
		rbx::signal<void()> scriptRenderSteppedEarlySignal;

		rbx::signal<void(double)>* getOrCreateScriptRenderSteppedSignal(bool create = true);

		void bindFunctionToRenderStepEarly(std::string name, int priority, Lua::WeakFunctionRef functionToBind);
		void unbindFunctionFromRenderStepEarly(std::string name);

		void fireRenderStepEarlyFunctions();

		void setRunState(RunState newState);
		void run() {setRunState(RS_RUNNING);}
		void pause() {setRunState(RS_PAUSED);}
		void stop() {setRunState(RS_STOPPED);}
        
		void stopTasks();
		void start();

		void raiseHeartbeat(double step, const Time::Interval& stepBudget);
		void gameStepped(double step, bool longStep);
		void gameNotStepped(double skipped);
		void renderStepped(double step, bool longStep);

		RunState getRunState() const		{return runState;}
		bool isEditState() const			{return (runState == RS_STOPPED);}
		bool isRunState() const				{return (runState == RS_RUNNING);}
		bool isPauseState()	const			{return (runState == RS_PAUSED);}
		bool isRunning() 					{return (runState == RS_RUNNING);}

		bool isServer();
		bool isClient();
		bool isStudio();
		bool isRunMode();

		//////////////////////////////////////////////////////
		//
		double wallTime() const {return totalWallTime;}
		double gameTime() const {return totalGameTime;}
		double smoothFps() const;
		double heartbeatFps() const;
		double physicsCpuFraction() const;
		double heartbeatCpuFraction() const;
		double physicsAverageStep() const;
		double heartbeatAverageStep() const;
	protected:
		void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
	};

}	// namespace
