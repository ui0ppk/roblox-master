#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "Script/ThreadRef.h"

namespace RBX {

	// A generic mechanism for displaying stats (like 3D FPS, network traffic, etc.)
	extern const char *const sSettings;
	extern const char *const sSettingsItem;

	class Settings
		: public DescribedNonCreatable<Settings, ServiceProvider, sSettings>
	{
	private:
		typedef DescribedNonCreatable<Settings, ServiceProvider, sSettings> Super;
		struct InvalidDescendentDetector {
			bool anyInvalid;
			InvalidDescendentDetector();
			static bool invalid(const Instance* instance);
			void operator()(shared_ptr<Instance> descendant);
		};
		struct InvalidDescendentCollector {
			std::vector< shared_ptr<Instance> > invalidInstances;
			InvalidDescendentCollector();
			void operator()(shared_ptr<Instance> descendant);
		};
	protected:
		std::string settingsFile;
		bool settingsErased;

		///////////////////////////////
		// overrides
		virtual void verifyAddDescendant(const Instance* newParent,
			const Instance* instanceGettingNewParent) const override;

	public:
		Settings(const std::string& settingsFile);
		void setSaveTarget(const std::string& optGlobalSettingsFile) {
			settingsFile = optGlobalSettingsFile;
		}

		void loadState(const std::string& optGlobalSettingsFile);
		void saveState();
		void eraseSettingsStore();

		void removeInvalidChildren();
		bool useSubmitTaskForLuaListeners() const override { return true; }
	};
	
	extern const char *const sGlobalBasicSettings;
	class GlobalBasicSettings
		: public DescribedNonCreatable<GlobalBasicSettings, Settings, sGlobalBasicSettings>
	{
		typedef DescribedNonCreatable<GlobalBasicSettings, Settings, sGlobalBasicSettings> Super;
	public:
		GlobalBasicSettings();

		class Item : public NonFactoryProduct< Instance, sSettingsItem>
		{
		public:
			virtual void reset() {}
			bool useSubmitTaskForLuaListeners() const override { return true; }
		protected:
			bool askAddChild(const Instance* instance) const override
			{
				return dynamic_cast<const Item*>(instance)!=NULL;
			}
		};

		boost::mutex mutex;
		static shared_ptr<GlobalBasicSettings> singleton();
		void reset();
		bool isUserFeatureEnabled(std::string name);
		void verifySetParent(const Instance* instance) const override;
	};

	extern const char *const sGlobalAdvancedSettings;

	class GlobalAdvancedSettings
		: public DescribedNonCreatable<GlobalAdvancedSettings, Settings, sGlobalAdvancedSettings>
	{
		typedef DescribedNonCreatable<GlobalAdvancedSettings, Settings, sGlobalAdvancedSettings> Super;
	public:
		GlobalAdvancedSettings();
		 ~GlobalAdvancedSettings();

		class Item : public NonFactoryProduct< Instance, sSettingsItem>
		{
		public:
			bool useSubmitTaskForLuaListeners() const override { return true; }
		protected:
			bool askAddChild(const Instance* instance) const override
			{
				return dynamic_cast<const Item*>(instance)!=NULL;
			}
		};

		shared_ptr<const RBX::Reflection::ValueTable> getFVariables();
		std::string getFVariable(std::string flag);
		bool getFFlag(std::string name);

		boost::mutex mutex;
		static shared_ptr<GlobalAdvancedSettings> singleton();
		static GlobalAdvancedSettings* raw_singleton();
		void verifySetParent(const Instance* instance) const override;
	};

	template<class Class, const char* const & sClassName>
	class GlobalAdvancedSettingsItem
		 : public RBX::DescribedCreatable<Class, RBX::GlobalAdvancedSettings::Item, sClassName>
		 , public Service
	{
		typedef RBX::DescribedCreatable<Class, RBX::GlobalAdvancedSettings::Item, sClassName> Super;
		static GlobalAdvancedSettingsItem* sing;
	protected:
		GlobalAdvancedSettingsItem()
		{
			Super::setName(sClassName);
			if (sing)
				throw RBX::runtime_error("singleton %s already exists", sClassName);
			sing = this;
		}
		~GlobalAdvancedSettingsItem()
		{
			sing = NULL;
		}

	public:
		static Class& singleton()
		{
			// shortcut
			if (sing)
				return *boost::polymorphic_downcast<Class*>(sing);

			RBX::GlobalAdvancedSettings* gs = RBX::GlobalAdvancedSettings::singleton().get();
			boost::mutex::scoped_lock lock(gs->mutex);
			if (!sing)
			{
				// "s" won't get collected when we leave scope because the parent will hold a ref to it 
				shared_ptr<Class> s = Class::createInstance();
				s->setParent(gs);
				RBXASSERT(s.get()==sing);
			}
			return *boost::polymorphic_downcast<Class*>(sing);
		}
	};
		
	template<class Class, const char* const & sClassName>
	GlobalAdvancedSettingsItem<Class, sClassName>* GlobalAdvancedSettingsItem<Class, sClassName>::sing = NULL;

	template<class Class, const char* const & sClassName>
	class GlobalBasicSettingsItem
		 : public RBX::DescribedCreatable<Class, RBX::GlobalBasicSettings::Item, sClassName>
		 , public Service
	{
		typedef RBX::DescribedCreatable<Class, RBX::GlobalBasicSettings::Item, sClassName> Super;
		static GlobalBasicSettingsItem* sing;
	protected:
		GlobalBasicSettingsItem()
		{
			Super::setName(sClassName);
			if (sing)
				throw RBX::runtime_error("singleton %s already exists", sClassName);
			sing = this;
		}
		~GlobalBasicSettingsItem()
		{
			sing = NULL;
		}


	public:
		static Class& singleton()
		{
			// shortcut
			if (sing)
				return *boost::polymorphic_downcast<Class*>(sing);

			RBX::GlobalBasicSettings* gs = RBX::GlobalBasicSettings::singleton().get();
			boost::mutex::scoped_lock lock(gs->mutex);
			if (!sing)
			{
				// "s" won't get collected when we leave scope because the parent will hold a ref to it 
				shared_ptr<Class> s = Class::createInstance();
				s->setParent(gs);
				RBXASSERT(s.get()==sing);
			}
			return *boost::polymorphic_downcast<Class*>(sing);
		}

		virtual void resetSettings()
		{}
	};
		
	template<class Class, const char* const & sClassName>
	GlobalBasicSettingsItem<Class, sClassName>* GlobalBasicSettingsItem<Class, sClassName>::sing = NULL;

} // namespace
