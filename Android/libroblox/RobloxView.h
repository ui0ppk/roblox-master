#pragma once

#include "boost/shared_ptr.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/thread.hpp"
#include "v8datamodel/game.h"
#include "Util/KeyCode.h"
#include "G3D/Vector2.h"
#include "rbx/signal.h"
#include "v8tree/Verb.h"

namespace RBX
{
	class DataModel;
	class ViewBase;
	class FunctionMarshaller;
	
	namespace Tasks
	{
		class Sequence;
	}
	
	namespace Reflection
	{
		class PropertyDescriptor;
	}

	class LeaveGameVerb : public RBX::Verb
	{
	public :
		LeaveGameVerb(RBX::VerbContainer* container);
		virtual void doIt(RBX::IDataState* dataState);
	};
}

class RobloxView
{
	boost::scoped_ptr<class RBX::LeaveGameVerb> leaveGameVerb;
	boost::shared_ptr<RBX::ViewBase> view;
	boost::shared_ptr<RBX::Game> game;
    
	RBX::FunctionMarshaller* marshaller;
	
	rbx::signals::scoped_connection placeIDChangeConnection;
    
	boost::shared_ptr<RBX::Tasks::Sequence> sequence;

	class RenderJob;
	boost::shared_ptr<RenderJob> renderJob;

	static boost::shared_ptr<RobloxView> rbxView;

	void doTeleport(std::string url, std::string ticket, std::string script);
	
	void onPlaceIDChanged(const RBX::Reflection::PropertyDescriptor* desc);
public:

    RobloxView(void* wnd, unsigned int width, unsigned int height);
	~RobloxView(void);

    // request rendering stop as the app goes to background
    void requestStopRenderingForBackgroundMode();
    void requestResumeRendering();
    
    void newGameDidStart();
	
	void setBounds(unsigned int width, unsigned int height);

    static RobloxView *create_view(shared_ptr<RBX::Game> game, void* wnd, unsigned int width, unsigned int height);
	
    boost::shared_ptr<RBX::DataModel> getDataModel() { return game->getDataModel(); }
    boost::shared_ptr<RBX::Game> getGame() { return game; }
    boost::shared_ptr<RBX::ViewBase> getView() { return view; }
    
    void restartDataModel();

    G3D::Vector2 getBounds() { return G3D::Vector2(width,height); }

    void replaceGame(shared_ptr<RBX::Game> game);

	void shutDownGraphics(shared_ptr<RBX::Game> game);
	void startUpGraphics(shared_ptr<RBX::Game> game, void *wnd, unsigned int width, unsigned int height);

private:
	unsigned int width;
	unsigned int height;
    
	void defineConcurrencyRules();
    
    void setupNewDataModel();
    
    void doRestartDataModel();
    
    static void bindWorkspace(boost::shared_ptr<RBX::ViewBase> view, boost::shared_ptr<RBX::DataModel> const dataModel, bool buildGUI = true);
    void completeViewPrep(shared_ptr<RBX::Game> game);
};
