#pragma once

#include "V8Tree/Service.h"
#include "Gui/Widget.h"
#include "Gui/GuiDraw.h"

namespace RBX {

	class PVInstance;
	class Verb;
	class PlayerHopper;
	class MouseCommand;
	class Mouse;

	namespace Network {
		class Player;
	}

	extern const char *const sBackpackItem;
	class BackpackItem					 				// common root of Tool, HopperBin - stuff that can go in the hopper and on the player
		: public DescribedNonCreatable<BackpackItem, Widget, sBackpackItem>
	{
	private:
		typedef DescribedNonCreatable<BackpackItem, Widget, sBackpackItem> Super;
		
		GuiDrawImage guiImageDraw;
		TextureId textureId;

		GuiDrawImage window;

		bool inBackpack();

		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ bool askAddChild(const Instance* instance) const;
		

	protected:

		//Instance
		/*override*/ void setName(const std::string& value);

		// GuiItem
		/*override*/ Vector2 getSize(Canvas canvas) const;
		/*override*/ bool isEnabled() {return inBackpack();}

		int getBinId() const;

	public:
		void setTextureId(const TextureId& value);
		const TextureId getTextureId() const;

		virtual bool drawEnabled() const {return true;}
		virtual bool drawSelected() const {return false;}

		virtual void onLocalClicked() {}
		virtual void onLocalOtherClicked() {}
	};

	//////////////////////////////////////////////////////////////////////////////

	extern const char *const sHopperBin;
	class HopperBin 
		: public DescribedCreatable<HopperBin, BackpackItem, sHopperBin>
	{
	private:
		typedef DescribedCreatable<HopperBin, BackpackItem, sHopperBin> Super;

	public:
		// Warning - these enums affect XML read - only append
		typedef enum BinType {	SCRIPT_BIN = 0,
								GAME_TOOL = 1,
								GRAB_TOOL = 2,
								CLONE_TOOL = 3,
								HAMMER_TOOL = 4} BinType;

		bool					active;				// I have a pending deselect event to fire
	private:
		BinType					binType;
		bool					replicationInitialized;

		void onSelectScript();
		void onSelectCommand();

		// GuiItem
		/*override*/ int getCursor();

		// BackpackItem
		/*override*/ bool drawSelected() const	{return active;}
		
		void selectedConnectionShimFunction();
		void reverseSelectedConnectionShimFunction(shared_ptr<Instance>& instance);

		rbx::signals::connection selectedConnectionShim;

	public:
		HopperBin();

		rbx::remote_signal<void()> replicatedSelectedSignal;
		rbx::remote_signal<void(shared_ptr<Instance>)> selectedSignal;
		rbx::signal<void()> deselectedSignal;

		BinType getBinType() const {return binType;}
		void setBinType(const BinType value);

		void disable();

		// BackpackItem		
		/*override*/ void onLocalClicked();
		/*override*/ void onLocalOtherClicked();

		/*override*/ void onAncestorChanged(const AncestorChanged& event);

		// deprecated, for reading legacy stuff
		void setLegacyCommand(const std::string& text);
		void setLegacyTextureName(const std::string& value);

		void dataChanged(const Reflection::PropertyDescriptor&);
	};

	//////////////////////////////////////////////////////////////////////////////
	//
	// Generic / tree view of the hopper
	// Used to build the Hopper Service
	// Draws dim

	class Hopper 
		: public RelativePanel
	{
	private:
		typedef RelativePanel Super;
	protected:
		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ bool askAddChild(const Instance* instance) const;

	public:
		Hopper();
	};

	//////////////////////////////////////////////////////////////////////////////

	extern const char *const sStarterPackService;
	class StarterPackService 
        : public DescribedNonCreatable<StarterPackService, Hopper, sStarterPackService>
		, public Service
	{
	private:
		typedef DescribedNonCreatable<StarterPackService, Hopper, sStarterPackService> Super;

	public:
		StarterPackService();
	};


	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	// Legacy 1-30-07
	// Renamed this class to StarterPack for clarity, and also
	// because StarterPack was around for a while which caused both to be used
	extern const char *const sLegacyHopperService;
	class LegacyHopperService 
		: public DescribedNonCreatable<LegacyHopperService, Hopper, sLegacyHopperService>
		, public Service
	{
	private:
		typedef DescribedNonCreatable<LegacyHopperService, Hopper, sLegacyHopperService> Super;
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
	public:
		LegacyHopperService();
		~LegacyHopperService();
	};


	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	// StarterGear
	// - the gear I bring with me in game
	//

	extern const char *const sStarterGear;
	class StarterGear : public DescribedCreatable<StarterGear, Instance, sStarterGear>
	{
	private:
		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ bool askAddChild(const Instance* instance) const;

	public:
		StarterGear();
		/*override*/ bool canClientCreate() { return true; }
	};

}	// namespace 