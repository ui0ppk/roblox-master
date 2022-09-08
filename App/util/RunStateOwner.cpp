#include "stdafx.h"

#include "Util/RunStateOwner.h"
#include "rbx/Debug.h"

#include "V8DataModel/Workspace.h"
#include "V8DataModel/DataModel.h"
#include "v8kernel/Constants.h"
#include "Script/ScriptContext.h"
#include "Util/Profiling.h"
#include "util/standardout.h"
#include "Network/Players.h"
#include "rbx/Log.h"
#include "VMProtect/VMProtectSDK.h"
#include "V8DataModel/HackDefines.h"
#include "V8World/Tolerance.h"
#include "script/ThreadRef.h"

#include "rbx/Profiler.h"

DYNAMIC_FASTFLAGVARIABLE(HeartBeatCanRunTwiceFor30Hz, true)
DYNAMIC_FASTFLAGVARIABLE(PhysicsFPSTimerFix, false)
DYNAMIC_FASTFLAGVARIABLE(ScriptExecutionContextApi, false)
LOGGROUP(CyclicExecutiveTiming)
LOGVARIABLE(HeartBeatFailure, 0)
LOGVARIABLE(PhysicsStepsPerSecond, 0)
FASTINTVARIABLE(NumDummyJobs, 0)

DYNAMIC_FASTFLAGVARIABLE(VariableHeartbeat, false)
DYNAMIC_FASTFLAGVARIABLE(TeamCreateIgnoreRunStateTransition, true)

DYNAMIC_FASTFLAG(UseR15Character)

namespace RBX {

namespace Reflection {
	template<>
	EnumDesc<RunService::RenderPriority>::EnumDesc()
		:EnumDescriptor("RenderPriority")
	{
		addPair(RunService::RENDERPRIORITY_FIRST, "First");
		addPair(RunService::RENDERPRIORITY_INPUT, "Input");
		addPair(RunService::RENDERPRIORITY_CAMERA, "Camera");
		addPair(RunService::RENDERPRIORITY_CHARACTER,"Character");
		addPair(RunService::RENDERPRIORITY_LAST, "Last");
	}
} // Namespace Reflection


const char* const sRunService = "RunService";

REFLECTION_BEGIN();
static Reflection::EventDesc<RunService, void(double, double)> event_Stepped(&RunService::scriptSteppedSignal, "Stepped", "time", "step", Security::None);
static Reflection::EventDesc<RunService, void(double)> event_Heartbeat(&RunService::scriptHeartbeatSignal, "Heartbeat", "step", Security::None);

static Reflection::EventDesc<RunService, 
	void(double), 
	rbx::signal<void(double)>,
	rbx::signal<void(double)>* (RunService::*)(bool)> event_RenderStepped(&RunService::getOrCreateScriptRenderSteppedSignal, "RenderStepped", "step", Security::None);

static Reflection::BoundFuncDesc<RunService, void()> runFunction(&RunService::run, "Run", Security::Plugin);
static Reflection::BoundFuncDesc<RunService, void()> pauseFunction(&RunService::pause, "Pause", Security::Plugin);
static Reflection::BoundFuncDesc<RunService, void()> resetFunction(&RunService::stop, "Reset", Security::Plugin, Reflection::Descriptor::Attributes::deprecated());
static Reflection::BoundFuncDesc<RunService, void()> func_Stop(&RunService::stop, "Stop", Security::Plugin);
static Reflection::BoundFuncDesc<RunService, bool()> func_isRunning(&RunService::isRunning, "IsRunning",  Security::RobloxScript);

static Reflection::BoundFuncDesc<RunService, void(std::string)> func_unbindRenderStepEarly(&RunService::unbindFunctionFromRenderStepEarly, "UnbindFromRenderStep", "name", Security::None);
static Reflection::BoundFuncDesc<RunService, void(std::string, int, Lua::WeakFunctionRef)> func_bindRenderStepEarly(&RunService::bindFunctionToRenderStepEarly, "BindToRenderStep", "name", "priority", "function", Security::None);

static Reflection::BoundFuncDesc<RunService, bool()> func_isServer(&RunService::isServer, "IsServer", Security::None);
static Reflection::BoundFuncDesc<RunService, bool()> func_isClient(&RunService::isClient, "IsClient", Security::None);
static Reflection::BoundFuncDesc<RunService, bool()> func_isStudio(&RunService::isStudio, "IsStudio", Security::None);
static Reflection::BoundFuncDesc<RunService, bool()> func_isRunMode(&RunService::isRunMode, "IsRunMode", Security::None);
REFLECTION_END();

class PhysicsJob : public DataModelJob
{
public:
	struct StepRecord {
		StepRecord( Time beginT, float ti , Time endT = Time() ) : beginTime( beginT ), timeInterval ( ti ), endTime( endT ) { }
		Time beginTime;
		Time endTime;
		float timeInterval;
	};
	boost::circular_buffer<StepRecord> stepRecords;
	Time lastTime;

	weak_ptr<DataModel> const dataModel;
	PhysicsJob(shared_ptr<DataModel> dataModel)
		:DataModelJob("Physics", DataModelJob::Physics, false, dataModel, Time::Interval(0.01))
		,dataModel(dataModel)
	{
		cyclicExecutive = true;
		cyclicPriority = CyclicExecutiveJobPriority_Physics;
		stepRecords.set_capacity(100);
	}
	
	virtual Time::Interval sleepTime(const Stats& stats)
	{
		return computeStandardSleepTime(stats, DataModel::throttleAt30Fps ? Constants::uiStepsPerSec() : 1000.0);
	}

	virtual double averageStepsPerSecond() const 
	{ 
		if( !RBX::TaskScheduler::singleton().isCyclicExecutive() )
		{
			return TaskScheduler::Job::averageStepsPerSecond(); // is dutyCycle.stepInterval().rate();
		}
		else
		{
			float timeInterval = 0.0f;
			RBX::Time lastCountedTimestamp = stepRecords.begin() != stepRecords.end() ? (stepRecords.end() - 1)->endTime : RBX::Time();
			for( boost::circular_buffer<StepRecord>::const_iterator i = stepRecords.begin(); i != stepRecords.end(); ++i )
			{
				timeInterval += i->timeInterval;
			}

			// The physics job runs however many 240Hz steps it needs to at various times
			// throughout the frame while waiting for rendering to finish.  In order to
			// maintain compatibility with existing scripts this calculates however many
			// 60Hz stepDataModelJob calls would have happened for the amount of simulation
			// that was done.
			double physicsFPS;
			if (DFFlag::PhysicsFPSTimerFix)
			{
				float denom = stepRecords.begin() != stepRecords.end() ? (lastCountedTimestamp - stepRecords.begin()->beginTime).seconds() : 1.0f;
				physicsFPS = (double)((float)Constants::uiStepsPerSec() * (timeInterval/ denom));
			}
			else
			{
				physicsFPS = (double)((float)Constants::uiStepsPerSec() * timeInterval);
			}
			return physicsFPS;
		}
	} 

	virtual Job::Error error(const Stats& stats)
	{
		Job::Error result;
        result.error = Constants::uiStepsPerSec() * stats.timespanSinceLastStep.seconds();
		return result;
	}

	virtual int getDesiredConcurrencyCount() const 
	{
		// return >1 if the job intends to do parallel work (using multiple threads)
		// During the step function, query the number of threads alloted by checking
		// allotedConcurrency
		return RunService::parallelPhysicsUserEnabled ? 1024 : 1; // We can always dream :)
	}

	virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
	{
		shared_ptr<DataModel> d(dataModel.lock());
		if (!d)
			return TaskScheduler::Done;

		FASTLOG1(FLog::DataModelJobs, "Physics Job start, data model: %p", d.get());
		DataModel::scoped_write_request request(d.get());
		// Step physics, replication and scripts (step, heartbeat, onStep)
//		double step = stats.timespanSinceLastStep.seconds();
		
		// If we didn't quite get the FPS we asked for, make up for it now
//		step = std::min(2.0/fps, step);
			
//		dataModel->physicsStep((float)step);

		double dt = stats.timespanSinceLastStep.seconds();
		double dutyDt = stats.timespanOfLastStep.seconds();
		Time stepBeginTime;
		if (DFFlag::PhysicsFPSTimerFix)
		{
			// To correctly record "Physics Frame Times" we use the time-stamp
			// at the end of last step, since total "stepTime" is the period 
			// between the end of last step and end of current step.
			// timeInterval + stepBeginTime = current time.
			stepBeginTime = Time::now<Time::Fast>() - stats.timespanSinceLastStep;
		}

		float timeInterval = d->physicsStep(Constants::uiDt(), dt, dutyDt, allotedConcurrency);

		if( RBX::TaskScheduler::singleton().isCyclicExecutive() )
		{
			// Use the end time as the current time so that adding a large timeInterval is
			// correctly accounted for.
			if (!DFFlag::PhysicsFPSTimerFix)
			{
				stepBeginTime = Time::now<Time::Fast>();
			}
			while( !stepRecords.empty() && (stepBeginTime - stepRecords.front().beginTime).seconds() > (DFFlag::PhysicsFPSTimerFix ? 2.0 : 1.0 ))
			{
				stepRecords.pop_front();
			}
			if( timeInterval > 0.0f )
			{
				if( stepRecords.full() )
				{
					stepRecords.set_capacity( stepRecords.capacity() * 2 );
				}
				stepRecords.push_back( StepRecord( stepBeginTime, timeInterval , Time::now<Time::Fast>()) );
			}
		}

		FASTLOG1F(FLog::PhysicsStepsPerSecond, "averageStepsPerSecond: %f", averageStepsPerSecond());


		FASTLOG1(FLog::DataModelJobs, "Physics Job finish, data model: %p", d.get());

#ifndef RBX_STUDIO_BUILD
        // Security Check.  Modifying lerp can result in speedhack.
        volatile double lerpCheck  = getStepStats().getIntervalLerp();
        // NoClip Check
        Vector3 noclipHighCheck = Tolerance::maxExtents().max();
        Vector3 noclipLowCheck = Tolerance::maxExtents().min();
        VMProtectBeginMutation("16");
        if ((lerpCheck > 0.051 || lerpCheck < 0.049)
            || (noclipHighCheck.x < 0.95e6f || noclipHighCheck.y < 0.96e6f || noclipHighCheck.z < 0.97e6f)
            || (noclipLowCheck.x > -0.95e6f || noclipLowCheck.y > -0.96e6f || noclipLowCheck.z > -0.97e6f))
        {
            if (boost::shared_ptr<DataModel> dataModelLocal = dataModel.lock())
            {
                dataModelLocal->addHackFlag(HATE_CONST_CHANGED);      
            }
        }
        if (!DataModel::throttleAt30Fps) // this will get set during testing as well.
        {
            if (boost::shared_ptr<DataModel> dataModelLocal = dataModel.lock())
            {
                dataModelLocal->addHackFlag(HATE_SPEEDHACK);
            }
        }
        VMProtectEnd();
#endif

		return TaskScheduler::Stepped;
	}
};

class HeartbeatTask : public DataModelJob
{
public:
	struct StepRecord {
		StepRecord( Time t, float ti ) : time( t ), timeInterval ( ti ) { }
		Time time;
		float timeInterval;
	};
	RBX::Time lastHeartbeatStamp;
	double maxStepsPerCycle;
	boost::circular_buffer<StepRecord> stepRecords;
	weak_ptr<RunService> const runService;
	weak_ptr<DataModel> const dataModel;	// TODO: Remove this field when no longer needed in step()
	double fps;
	HeartbeatTask(shared_ptr<RunService> runService)
		:DataModelJob("Heartbeat", DataModelJob::Write, false, shared_from_dynamic_cast<DataModel>(runService->getParent()), Time::Interval(0.003))
        ,fps(30)
		,runService(runService)
		,dataModel(shared_from_dynamic_cast<DataModel>(runService->getParent())) 
		,lastHeartbeatStamp(RBX::Time::now<RBX::Time::Fast>())
	{
		cyclicExecutive = true;
		cyclicPriority = CyclicExecutiveJobPriority_Heartbeat;
		stepRecords.set_capacity(100);
		maxStepsPerCycle = DFFlag::HeartBeatCanRunTwiceFor30Hz ? 2.0f : 1.0f;
	}

	Time::Interval sleepTime(const Stats& stats)
	{
		return computeStandardSleepTime(stats, fps);
	}

	virtual Job::Error error(const Stats& stats)
	{
		return computeStandardError(stats, fps);
	}

	virtual double averageStepsPerSecond() const 
	{ 
		if( !RBX::TaskScheduler::singleton().isCyclicExecutive() )
		{
			return TaskScheduler::Job::averageStepsPerSecond(); // is dutyCycle.stepInterval().rate();
		}
		else
		{
			float timeInterval = 0.0f;
			for( boost::circular_buffer<StepRecord>::const_iterator i = stepRecords.begin(); i != stepRecords.end(); ++i )
			{
				timeInterval += i->timeInterval;
			}

			// Heartbeat tries to maintain at 30Hz

			return (double)((float)fps * timeInterval);
		}
	} 

	TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
	{
		shared_ptr<DataModel> d(dataModel.lock());
		if (!d)
			return TaskScheduler::Done;
		shared_ptr<RunService> r(runService.lock());
		if (!r)
			return TaskScheduler::Done;

		FASTLOG1(FLog::DataModelJobs, "Heartbeat start, data model: %p", d.get());
		DataModel::scoped_write_request request(d.get());

		if (!DFFlag::VariableHeartbeat && (RBX::TaskScheduler::singleton().isCyclicExecutive() && cyclicExecutive))
		{
            // Clean Up step-record
            const Time now = Time::now<Time::Fast>();
            while( !stepRecords.empty() && (now-stepRecords.front().time).seconds() > 1.0 )
            {
                stepRecords.pop_front();
            }
            
			float testSteps = updateStepsRequiredForCyclicExecutive(stats.timespanSinceLastStep.seconds(),
																	fps,
																	maxStepsPerCycle,
																	maxStepsPerCycle);
			FASTLOG1F(FLog::HeartBeatFailure, "Steps for this Cycle: %4.7f", testSteps);
			FASTLOG1F(FLog::HeartBeatFailure, "Steps accumulated %4.7f", stepsAccumulated);
			if (testSteps < 1.0f)
			{
				return TaskScheduler::Stepped;
			}

			int totalSteps = floorf(testSteps);
			for (int i = 0; i < totalSteps; i++)
			{
				RBX::Time currentTime = RBX::Time::now<RBX::Time::Fast>();
				double dtSinceLast = (currentTime - lastHeartbeatStamp).seconds();
				r->raiseHeartbeat(dtSinceLast, stepBudget);
				lastHeartbeatStamp = currentTime;
                
                // Record step for Diagnostic Display
				if( stepRecords.full() )
				{
					stepRecords.set_capacity( stepRecords.capacity() * 2 );
				}
                stepRecords.push_back( StepRecord( now, dtSinceLast ) );
			}
		}
		else
		{
			r->raiseHeartbeat((float)stats.timespanSinceLastStep.seconds(), stepBudget);
		}


		FASTLOG1(FLog::DataModelJobs, "Heartbeat finish, data model: %p", d.get());
		return TaskScheduler::Stepped;
	}
};

#ifdef RBX_TEST_BUILD
class DummyTask : public DataModelJob
{
public:
    weak_ptr<RunService> const runService;
    weak_ptr<DataModel> const dataModel;	// TODO: Remove this field when no longer needed in step()
    double fps;
    DummyTask(shared_ptr<RunService> runService)
        :DataModelJob("DummyJob", DataModelJob::Write, false, shared_from_dynamic_cast<DataModel>(runService->getParent()), Time::Interval(0.003))
        ,fps(30)
        ,runService(runService)
        ,dataModel(shared_from_dynamic_cast<DataModel>(runService->getParent())) 
    {}

    Time::Interval sleepTime(const Stats& stats)
    {
        return computeStandardSleepTime(stats, fps);
    }

    virtual Job::Error error(const Stats& stats)
    {
        return computeStandardError(stats, fps);
    }

    TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
    {
        shared_ptr<DataModel> d(dataModel.lock());
        if (!d)
            return TaskScheduler::Done;
        shared_ptr<RunService> r(runService.lock());
        if (!r)
            return TaskScheduler::Done;

        DataModel::scoped_write_request request(d.get());

        return TaskScheduler::Stepped;
    }
};
#endif

RunService::RunService()
	: runState(RS_STOPPED)
	, totalGameTime(0.0)
	, totalGameTimeAtLastHeartbeat(0.0)
	, totalWallTime(0.0)
	, skippedTimeAccumulated(0.0)
{
#ifdef RBX_TEST_BUILD
    dummyTasks.resize(1000);
#endif
	setName("Run Service");
}

bool RunService::parallelPhysicsUserEnabled = false;

void RunService::stopTasks()
{
#if 1
	if (physicsJob)
	{
		TaskScheduler::singleton().remove(physicsJob);
		physicsJob.reset();	// remove the reference to avoid risk of memory leak
	}
    if (heartbeatTask)
    {
        TaskScheduler::singleton().remove(heartbeatTask);
        heartbeatTask.reset();	// remove the reference to avoid risk of memory leak
    }
#else
	TaskScheduler::singleton().removeTask(physicsJob, true);
	TaskScheduler::singleton().removeTask(heartbeatTask, true);
#endif

#ifdef RBX_TEST_BUILD
    for (size_t i=0; i<dummyTasks.size(); i++)
    {
        if (dummyTasks[i])
        {
            TaskScheduler::singleton().remove(dummyTasks[i]);
            dummyTasks[i].reset();	// remove the reference to avoid risk of memory leak
        }
    }
#endif
}

void RunService::start()
{
	if (DFFlag::UseR15Character)
	{
		if (DataModel* dm = DataModel::get(this))
		{
			if (dm->getUniverseDataRequested())
				dm->universeDataLoaded.get_future().wait();
		}
	}

	physicsJob = shared_ptr<PhysicsJob>(new PhysicsJob(shared_from_dynamic_cast<DataModel>(getParent())));
    heartbeatTask = shared_ptr<HeartbeatTask>(new HeartbeatTask(shared_from(this)));
    TaskScheduler::singleton().add(physicsJob);
    TaskScheduler::singleton().add(heartbeatTask);

#ifdef RBX_TEST_BUILD
    for (int i=0; i<FInt::NumDummyJobs; i++)
    {
	    dummyTasks[i] = shared_ptr<DummyTask>(new DummyTask(shared_from(this)));
        TaskScheduler::singleton().add(dummyTasks[i]);
    }
#endif
}

RunService::~RunService()
{
}

TaskScheduler::Job* RunService::getPhysicsJob() { return physicsJob.get(); }
TaskScheduler::Job* RunService::getHeartbeat() { return heartbeatTask.get(); };

void RunService::raiseHeartbeat(double wallStep, const Time::Interval& stepBudget)
{
	FASTLOG2F(FLog::HeartBeatFailure, "Heartbeat stepping: %4.4f, Step Budget: %4.4f", wallStep, stepBudget.seconds());
	totalWallTime += wallStep;

	double gameStep = totalGameTime - totalGameTimeAtLastHeartbeat;

	heartbeatSignal(Heartbeat(totalWallTime, wallStep, totalGameTime, gameStep, Time::nowFast() + stepBudget));
    
    scriptHeartbeatSignal(wallStep);

	totalGameTimeAtLastHeartbeat = totalGameTime;
}

void RunService::gameStepped(double gameStep, bool longStep)
{
	RBXPROFILER_SCOPE("Physics", "gameStepped");

	totalGameTime += gameStep + skippedTimeAccumulated;
	skippedTimeAccumulated = 0.0;

	float stepEventGameTime = totalGameTime;

	// Swapped
	highPrioritySteppedSignal(Stepped(stepEventGameTime, gameStep, longStep));
	steppedSignal(Stepped(stepEventGameTime, gameStep, longStep));

	if (longStep)
	{
		scriptSteppedSignal(stepEventGameTime, gameStep);
	}
}

void RunService::gameNotStepped(double skipped)
{
	skippedTimeAccumulated += skipped;
}

void RunService::renderStepped(double step, bool longStep)
{
	fireRenderStepEarlyFunctions();

	scriptRenderSteppedSignal(step);

	renderSteppedSignal(Stepped(0, step, longStep));
}

void RunService::bindFunctionToRenderStepEarly(std::string name, int priority, Lua::WeakFunctionRef functionToBind)
{
	renderSteppedEarlyCallbackMap[priority].push_back(FunctionNameRefPair(name, functionToBind));
}

void RunService::unbindFunctionFromRenderStepEarly(std::string name)
{
	int erasedFuncCount = 0;

	for (EventCallbackMap::iterator iter = renderSteppedEarlyCallbackMap.begin(); iter != renderSteppedEarlyCallbackMap.end(); ++iter)
	{
		std::vector<FunctionNameRefPair> functionVector = iter->second;
		std::vector<FunctionNameRefPair>::iterator funcIter = functionVector.begin();
		while (funcIter != functionVector.end())
		{
			if (funcIter->first.compare(name) == 0)
			{
				erasedFuncCount++;
				funcIter = functionVector.erase(funcIter);
			}
			else
			{
				++funcIter;
			}
		}

		iter->second = functionVector;
	}


	if (erasedFuncCount > 1)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING,"RunService:UnbindFromRenderStep removed different functions with same reference name %s %i times.", name.c_str(), erasedFuncCount);
	}
}

static void InvokeCallback(weak_ptr<RunService> weakRunService, Lua::WeakFunctionRef callback, shared_ptr<Reflection::Tuple> args)
{
	if(shared_ptr<RunService> strongThis = weakRunService.lock())
	{
		if (Lua::ThreadRef threadRef = callback.lock())
		{
			if (ScriptContext* sc = ServiceProvider::create<ScriptContext>(strongThis.get()))
			{
				try
				{
					sc->callInNewThread(callback, *(args.get()));
				}
				catch (RBX::base_exception& e)
				{
					StandardOut::singleton()->printf(MESSAGE_ERROR, "RunService:fireRenderStepEarlyFunctions unexpected error while invoking callback: %s", e.what());
				}
			}
		}
	}
}

void RunService::fireRenderStepEarlyFunctions()
{
	for (EventCallbackMap::iterator iter = renderSteppedEarlyCallbackMap.begin(); iter != renderSteppedEarlyCallbackMap.end(); ++iter)
	{
		std::vector<FunctionNameRefPair> functionVector = iter->second;
		for (std::vector<FunctionNameRefPair>::iterator funcIter = functionVector.begin(); funcIter != functionVector.end(); ++funcIter)
		{
			try
			{
				if (!funcIter->second.empty())
				{
					InvokeCallback(weak_from(this), funcIter->second, rbx::make_shared<Reflection::Tuple>());
				}
			}
			catch(const std::runtime_error& e)
			{
				throw RBX::runtime_error("RunService::InvokeCallback failed because %s", e.what());
			}
		}
	}
}

void RunService::setRunState(RunState newState) 
{
	if (DFFlag::TeamCreateIgnoreRunStateTransition && Network::Players::isCloudEdit(this))
	{
		return;
	}

	if (newState==RS_RUNNING)
	{
		if (runState!=RS_STOPPED && runState!=RS_PAUSED)
			return;
		if (!physicsJob)
			start();
	}

	if (newState==RS_PAUSED)
	{
		if (runState!=RS_RUNNING)
			return;
	}

	if (newState==RS_STOPPED)		// a reset event
	{
		totalWallTime = 0.0;
		totalGameTime = 0.0;
		totalGameTimeAtLastHeartbeat = 0.0;
		skippedTimeAccumulated = 0.0;
	}

	if (runState != newState) {
		RunTransition transition(runState, newState);
		runState = newState;

		runTransitionSignal(transition);
	}
}

bool RunService::isServer() 
{ 
	if (DFFlag::ScriptExecutionContextApi)
	{
		return Network::Players::backendProcessing(this); 
	}
	throw std::runtime_error("RunService::IsServer() is not yet enabled");
}

bool RunService::isClient()
{
	if (DFFlag::ScriptExecutionContextApi)
	{
		return Network::Players::frontendProcessing(this);
	}
	throw std::runtime_error("RunService::IsClient() is not yet enabled");
}	

bool RunService::isStudio()
{
	if (DFFlag::ScriptExecutionContextApi)
	{
#if defined(RBX_STUDIO_BUILD)
		return true;
#else
		return false;
#endif
	}
	throw std::runtime_error("RunService::IsStudio() is not yet enabled");
}

bool RunService::isRunMode()
{
	if (DFFlag::ScriptExecutionContextApi)
	{
		if (DataModel* dm = DataModel::get(this))
		{
			return dm->isRunMode();
		}
		return false;
	}
	throw std::runtime_error("RunService::IsRunMode() is not yet enabled");
}

void RunService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (oldProvider)
	{
		stopTasks();
		renderSteppedEarlyCallbackMap.clear();
	}

	Super::onServiceProvider(oldProvider, newProvider);
}

double RunService::smoothFps() const {
    if (physicsJob)
        return physicsJob->averageStepsPerSecond();
    else
        return 0.f;
}

double RunService::heartbeatFps() const { return heartbeatTask->averageStepsPerSecond(); }

double RunService::physicsAverageStep() const { return physicsJob->averageStepTime(); }
double RunService::heartbeatAverageStep() const { return heartbeatTask->averageStepTime(); }

double RunService::physicsCpuFraction() const { return physicsJob->averageDutyCycle(); }
double RunService::heartbeatCpuFraction() const { return heartbeatTask->averageDutyCycle(); }

rbx::signal<void(double)>* RunService::getOrCreateScriptRenderSteppedSignal(bool create)
{
	if (create && !Network::Players::frontendProcessing(this))
        throw std::runtime_error("RenderStepped event can only be used from local scripts");

	return &scriptRenderSteppedSignal;
}
}	// namespace
