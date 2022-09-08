#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "script/ThreadRef.h"
#include "script/ScriptContext.h"

struct lua_State;
struct lua_Debug;
struct Table;

namespace RBX
{
	class Script;
	class ModuleScript;
	class DataModel;

	namespace Scripting
	{
		class ScriptDebugger;

		enum BreakOnErrorMode
		{
			BreakOnErrorMode_Never = 0,
			BreakOnErrorMode_AllExceptions,
			BreakOnErrorMode_UnhandledExceptions
		};

		enum ExecutionMode
		{
			ExecutionMode_Continue = 0,
			ExecutionMode_Break
		};

		struct ISpecialBreakpoint
		{
			virtual ~ISpecialBreakpoint() {}
			virtual bool hitTest(lua_State* L, lua_Debug *ar) = 0;
			lua_State* baseThread;
		};

		extern const char* const sDebuggerManager;
		// Contains all data related to Lua debugging of Scripts
		class DebuggerManager
			: public DescribedNonCreatable<DebuggerManager, Instance, sDebuggerManager, Reflection::ClassDescriptor::INTERNAL_LOCAL, Security::LocalUser>
		{
			typedef DescribedNonCreatable<DebuggerManager, Instance, sDebuggerManager, Reflection::ClassDescriptor::INTERNAL_LOCAL, Security::LocalUser> Super;
		public:
			typedef boost::unordered_map<const Instance*, ScriptDebugger*> Debuggers;
		private:
			bool enabled;
			Debuggers debuggers;

			rbx::signals::connection errorSignalConnection;
			rbx::signals::connection descendantAddedSignalConnection;

			typedef boost::unordered_map<const Instance*, boost::shared_ptr<ScriptDebugger> > UnaddedDebuggers;
			UnaddedDebuggers unaddedDebuggers;

			typedef boost::unordered_map<const lua_State*, ScriptDebugger*> DebuggersLookup;
			DebuggersLookup debuggersLookup;

			RBX::DataModel *dataModel;
			BreakOnErrorMode breakOnErrorMode;

			boost::scoped_ptr<ISpecialBreakpoint> specialBreakpoint;
			ExecutionMode                         executionMode;
			std::list<Lua::WeakThreadRef>         pausedThreads, resumingPausedThreads, errorThreads;
			bool                                  resuming;
			bool                                  scriptAutoResume;

		public:			
			DebuggerManager();
			~DebuggerManager();

			static DebuggerManager& singleton();

			void setDataModel(RBX::DataModel *pDataModel);
			RBX::DataModel* getDataModel();

			void enableDebugging();
			bool getEnabled() const { return enabled; }

			BreakOnErrorMode getBreakOnErrorMode() const { return breakOnErrorMode; }
			void setBreakOnErrorMode(BreakOnErrorMode mode);

            static Reflection::Variant readWatchValue(std::string expression, int stackFrame, lua_State* L);

			const Debuggers& getDebuggers()
			{
				return debuggers;
			}
			shared_ptr<const Instances> getDebuggers_Reflection();

			ScriptDebugger* findDebugger(lua_State* L);
			ScriptDebugger* findDebugger(Instance* script);

			shared_ptr<ScriptDebugger> addDebugger(Instance* script);
			shared_ptr<Instance> addDebugger_Reflection(shared_ptr<Instance> script);
			void addDebugger(shared_ptr<ScriptDebugger> debugger);

			void populateForLookup(lua_State* L, ScriptDebugger* debugger);

			void pause();
			void resume();
			void stepOver();
			void stepInto();
			void stepOut();

			void reset();
			void setScriptAutoResume(bool state) { scriptAutoResume = state; }

			static void hook(lua_State* L, lua_Debug *ar);

			rbx::signal<void(shared_ptr<Instance>)> debuggerAdded;
			rbx::signal<void(shared_ptr<Instance>)> debuggerRemoved;

		protected:
			/*override*/ bool askForbidChild(const Instance* instance) const;
			/*override*/ void verifyAddChild(const Instance* newChild) const;
			/*override*/ void onChildAdded(Instance*  child);
			/*override*/ void onChildRemoved(Instance*  child);
			/*override*/ void onChildChanged(Instance* instance, const PropertyChanged& event);
		
			void addScriptDebugger(Instance* instance);
			void onErrorSignal(lua_State* L);
			void onHook(lua_State* L, lua_Debug *ar);

			void addUnaddedDebuggerForAddedDescendant(shared_ptr<RBX::Instance> instance);
		};

		class DebuggerBreakpoint;
		class DebuggerWatch;

		extern const char* const sScriptDebugger;
		// Debugs an RBX::Script
		class ScriptDebugger
			: public DescribedCreatable<ScriptDebugger, Instance, sScriptDebugger, Reflection::ClassDescriptor::PERSISTENT_HIDDEN, Security::LocalUser>
		{
		public:
			typedef boost::unordered_map<int, DebuggerBreakpoint*> Breakpoints;
			typedef std::vector<DebuggerWatch*> Watches;
			struct PausedThreadData;
			typedef boost::unordered_map<long, PausedThreadData> PausedThreads;
		private:
			typedef DescribedCreatable<ScriptDebugger, Instance, sScriptDebugger, Reflection::ClassDescriptor::PERSISTENT_HIDDEN, Security::LocalUser> Super;

			Breakpoints breakpoints;
			Watches watches;
			
			boost::scoped_ptr<ISpecialBreakpoint> specialBreakpoint;

			shared_ptr<Instance> script;
			rbx::signals::scoped_connection scriptStartedConnection;
			rbx::signals::scoped_connection scriptStoppedConnection;
			rbx::signals::scoped_connection scriptParentChangedConnection;
			rbx::signals::scoped_connection scriptClonedConnection;
			Lua::WeakThreadRef rootThread; // The root thread of script. Set when the Script starts and reset when it stops

			typedef boost::function<void(lua_State* L, lua_Debug *ar)> HookFunction;
			HookFunction hookFunction;	// used to overload the hook function

			Lua::WeakThreadRef pausedThread; // the thread that a breakpoint hit
			Lua::WeakThreadRef errorThread; // the thread that has error
			lua_Debug *breakpointHookData;	// set for a short period of time during the hook when we encounter a breakpoint			
			void*  globalRawScriptPtr;
			Table* prevFuncTable;
			void*  prevRawScriptPtr;			
			int currentLine; // the current line when a breakpoint is hit
			bool ignoreDebuggerBreak; // whether to ignore breakpoint at the current line
			long currentThreadID;
			PausedThreads pausedThreads;
			bool rootThreadResumed;

		public:
			ScriptDebugger()
				:currentLine(0) 
				,breakpointHookData(NULL)
				,globalRawScriptPtr(NULL)
				,prevFuncTable(NULL)
				,prevRawScriptPtr(NULL)
				,ignoreDebuggerBreak(false)
				,currentThreadID(0)
				,rootThreadResumed(false)
			{}
			ScriptDebugger(Instance& script);
			~ScriptDebugger();

			Instance* getScript() const { return script.get(); }

			void setScript(Script* value);
			void setScript(ModuleScript* value);

			std::string getScriptPath() const;
			void setScriptPath(std::string scriptPath);

			void setIgnoreDebuggerBreak(bool state) { ignoreDebuggerBreak = state; }

			DebuggerBreakpoint* findBreakpoint(int line);
			shared_ptr<DebuggerBreakpoint> setBreakpoint(int line);
			shared_ptr<Instance> setBreakpoint_Reflection(int line);
			const Breakpoints& getBreakpoints()
			{
				return breakpoints;
			}
			shared_ptr<const Instances> getBreakpoints_Reflection();

			shared_ptr<DebuggerWatch> addWatch(std::string expression);
			shared_ptr<Instance> addWatch_Reflection(std::string expression);
			const Watches& getWatches()
			{
				return watches;
			}
			shared_ptr<const Instances> getWatches_Reflection();
			Reflection::Variant getWatchValue(DebuggerWatch* watch, int stackFrame = 0);
			Reflection::Variant getWatchValue_Reflection(shared_ptr<Instance> watch);

			Reflection::Variant getKeyValue(std::string key, int stackFrame);

			bool isDebugging() const
			{
				return !rootThread.empty();
			}

			bool isPaused() const;

			bool hasError() const
			{
				return !errorThread.empty();
			}

			int getCurrentLine() const
			{
				return currentLine;
			}
			void pause();
			void resume();
			void resumeTo(int line);
			void stepOver();
			void stepInto();
			void stepOut();

			struct FunctionInfo
			{
				boost::shared_ptr<RBX::Instance> script;
				int frame;
				std::string name;
				std::string what;
				std::string namewhat;
				std::string short_src;
				int currentline;
				int linedefined;
				int lastlinedefined;
			};
			typedef std::vector<FunctionInfo> Stack;
			Stack getStack();
			shared_ptr<const Reflection::ValueArray> getStack_Reflection();
			shared_ptr<const Reflection::ValueMap> getLocals(int stackIndex);
			shared_ptr<const Reflection::ValueMap> getUpvalues(int stackIndex);
			shared_ptr<const Reflection::ValueMap> getGlobals();
			void setLocal(std::string name, Reflection::Variant value, int stackFrame = 0);
			void setUpvalue(std::string name, Reflection::Variant value, int stackFrame = 0);
			void setGlobal(std::string name, Reflection::Variant value);

			void handleError(lua_State* L);
			void updateHook();

			ScriptContext::Result resumeThread(lua_State* L, bool evalLineHookForCurrentLine = false);
			bool handleHook(lua_State* L, lua_Debug *ar);
			bool onLineHook(lua_State* L, lua_Debug *ar);
			void debuggerBreak(lua_State* L, lua_Debug *ar);

			struct PausedThreadData
			{
				int pausedLine;
				Lua::WeakThreadRef thread;

				bool hasError;
				Stack callStack;
				std::string errorMessage;

				PausedThreadData()
				:hasError(false)
				,pausedLine(0)
				{
				}
			};
			const PausedThreads& getPausedThreads() { return pausedThreads; }
			
			bool isPausedThread(long threadID);
			bool isErrorThread(long threadID);

			bool isRootThread(long threadID);
			bool isRootThreadResumed() { return rootThreadResumed; }

			void setCurrentThread(long threadID);
			long getCurrentThread() { return currentThreadID; }

			rbx::signal<void(int)> encounteredBreak;
			rbx::signal<void()> resuming;
			
			rbx::signal<void(shared_ptr<Instance>)> breakpointAdded;
			rbx::signal<void(shared_ptr<Instance>)> breakpointRemoved;
			rbx::signal<void(shared_ptr<Instance>)> watchAdded;
			rbx::signal<void(shared_ptr<Instance>)> watchRemoved;
			rbx::signal<void(int, std::string, Stack)>   scriptErrorDetected;

		protected:
			/*override*/ bool askForbidChild(const Instance* instance) const;
			/*override*/ void verifySetParent(const Instance* newParent) const;
			/*override*/ void verifyAddChild(const Instance* newChild) const;
			/*override*/ void onChildAdded(Instance* child);
			/*override*/ void onChildRemoved(Instance* child);

		private:
			void onScriptStarting(lua_State* L);
			void onScriptStopped();
			void onScriptParentChanged(shared_ptr<RBX::Instance> newParent);
			void onScriptCloned(boost::shared_ptr<Instance> clonedScript);
			bool shouldBreak(DebuggerBreakpoint* bp, lua_State* L);
			bool hasDifferentScriptInstances(lua_State* L);
			void handleError(std::string errorMessage, const Stack& stack = Stack());
			void setScript(Instance* value);
			boost::shared_ptr<ScriptDebugger> createClone(boost::shared_ptr<Instance> clonedScript);

			template<class R>
			void withPausedThreadHook(lua_State* L, lua_Debug *ar, boost::function<R(lua_State* L, lua_Debug *ar)> f, R& r, shared_ptr<std::string>& error);

			// TODO: template specialization for R=void
			template<class R>
			R withPausedThread(boost::function<R(lua_State* L, lua_Debug *ar)> f);
			static shared_ptr<Reflection::ValueMap> readLocals(int stackIndex, lua_State* L);
			static shared_ptr<Reflection::ValueMap> readUpvalues(int stackIndex, lua_State* L);
			static shared_ptr<Reflection::ValueMap> readGlobals(lua_State* L);
			static Stack readStack(lua_State* L);
			static RBX::Instance* getScriptForLuaState(lua_State* L);
			static void updateRootThread(ScriptDebugger* scriptDebugger, lua_State *L);
			static void setLuaHook(ScriptDebugger* scriptDebugger, int hookMask, lua_State *L);			
		};

		extern const char* const sDebuggerBreakpoint;
		class DebuggerBreakpoint
			: public DescribedCreatable<DebuggerBreakpoint, Instance, sDebuggerBreakpoint, Reflection::ClassDescriptor::PERSISTENT_HIDDEN, Security::LocalUser>
		{
			bool enabled;
			int line;
			std::string condition;
		public:
			DebuggerBreakpoint();
			DebuggerBreakpoint(int line);
			~DebuggerBreakpoint();

			int getLine() const { return line; }

			bool isEnabled() const { return enabled; }

			const std::string& getCondition() const { return condition; }

			static Reflection::BoundProp<bool> prop_Enabled;
			static Reflection::BoundProp<std::string> prop_Condition;

		protected:
			/*override*/ void verifySetParent(const Instance* newParent) const;
			/*override*/ bool askForbidChild(const Instance* instance) const { return true; }
			/*override*/ void verifyAddChild(const Instance* newChild) const
			{
				throw std::runtime_error("DebuggerBreakpoint can have no children");
			}
		private:
			void setLine(int line);
			static Reflection::BoundProp<int> prop_Line_Data;

		};

		extern const char* const sDebuggerWatch;
		class DebuggerWatch
			: public DescribedCreatable<DebuggerWatch, Instance, sDebuggerWatch, Reflection::ClassDescriptor::PERSISTENT, Security::LocalUser>
		{
			std::string expression;
		public:
			DebuggerWatch() {}
			DebuggerWatch(std::string expression);
			const std::string& getCondition() const { return expression; }
			void checkExpressionSyntax();

			const std::string& getExpression() const { return expression; }

			static Reflection::BoundProp<std::string> prop_Expression;

		protected:
			/*override*/ void verifySetParent(const Instance* newParent) const;
			/*override*/ bool askForbidChild(const Instance* instance) const { return true; }
			/*override*/ void verifyAddChild(const Instance* newChild) const
			{
				throw std::runtime_error("DebuggerWatch can have no children");
			}
		};
	}
}

