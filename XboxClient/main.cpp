#include <xdk.h>
#include <mmdeviceapi.h>
#include <wrl.h>
#include <ppltasks.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <io.h>


#include "XboxService.h"
#include "XboxUtils.h"
#include "async.h"

#include "v8datamodel/GameSettings.h"
#include "V8DataModel/GameBasicSettings.h"

using namespace Windows::Foundation;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;

using namespace Concurrency;

extern void dprintf( const char* fmt, ... );

FASTFLAGVARIABLE(ForceRetail, true);

ref class ViewProvider sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:
    ViewProvider();
	virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView );
	virtual void SetWindow(Windows::UI::Core::CoreWindow^ wnd);
	virtual void Load( _In_ Platform::String^ entryPoint );
	virtual void Run();
	virtual void Uninitialize();

protected:
	// Event Handlers
	void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
	void OnResourceAvailabilityChanged(Platform::Object^ sender, Platform::Object^ args);
	void OnExiting(Platform::Object^ sender, Platform::Object^ args);
	void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
	void OnResuming(Platform::Object^ sender, Platform::Object^ args);
	void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

private:
	XboxPlatform* plt;
};

ref class ViewProviderFactory sealed : Windows::ApplicationModel::Core::IFrameworkViewSource 
{
public:
	ViewProviderFactory() {}
	virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView() { return ref new ViewProvider; }

};


//////////////////////////////////////////////////////////////////////////

/////////////////////////////
ViewProvider::ViewProvider()
{	
}

void ViewProvider::Initialize(CoreApplicationView^ applicationView)
{
	applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &ViewProvider::OnActivated);
	CoreApplication::ResourceAvailabilityChanged += ref new EventHandler<Platform::Object^>(this, &ViewProvider::OnResourceAvailabilityChanged);
	CoreApplication::Exiting += ref new EventHandler<Platform::Object^>(this, &ViewProvider::OnExiting);
	CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &ViewProvider::OnSuspending);
	CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(this, &ViewProvider::OnResuming);
}

void ViewProvider::SetWindow(Windows::UI::Core::CoreWindow^ wnd)
{
	plt = new XboxPlatform();
}

void ViewProvider::Run()
{
    RBX::StandardOut::singleton()->messageOut.connect( [](const RBX::StandardOutMessage& m) -> void { dprintf( "%s\n", m.message.c_str() ); } );
	
    while (true)
    {
		Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents( Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent );
        plt->tick();
    }
}

// The purpose of this method is to get the application entry point.
void ViewProvider::Load(Platform::String^ entryPoint)
{
}

void ViewProvider::Uninitialize()
{
    delete plt;
}

// Called when the application is activated.
void ViewProvider::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	if (args->Kind == Windows::ApplicationModel::Activation::ActivationKind::Protocol)
	{
		// Tell the game object we had a protocol activation so it can setup state appropriately.
		plt->onProtocolActivated(
			static_cast<IProtocolActivatedEventArgs^>(args)
			);
	}
	//CoreWindow::GetForCurrentThread()->Activate();
}

void ViewProvider::OnExiting(Platform::Object^ sender, Platform::Object^ args)
{
	RBX::GlobalBasicSettings::singleton()->saveState();
	RBX::GlobalAdvancedSettings::singleton()->saveState();
}

void ViewProvider::OnResourceAvailabilityChanged(Platform::Object^ sender, Platform::Object^ args)
{
	// check to see if going into constrained or leaving constrained
	if(CoreApplication::ResourceAvailability == Windows::ApplicationModel::Core::ResourceAvailability::Constrained)
	{
		dprintf("entering constrained state\n");
		plt->xbEventPlayerSessionPause();
		plt->stopEventHandlers();
	}
	else
	{
		dprintf("leaving constrained state\n");
		if(!plt->xboxLiveContext) return;

		plt->startEventHandlers();
		plt->xbEventPlayerSessionResume();
		// leaving constrained state for running 
		// check for inventory 
		async(plt->xboxLiveContext->InventoryService->GetInventoryItemsAsync( Microsoft::Xbox::Services::Marketplace::MediaItemType::GameConsumable )).complete(
			[this](Marketplace::InventoryItemsResult^ inventoryResult){
				try
				{
					//  Because we may be compiling a list of multiple content types from separate
					//  calls, we just append the results to the passed in vector.
					if(inventoryResult->Items->Size > 0)
					{
						for(int i = 0; i < inventoryResult->Items->Size;i++)
						{
							if(inventoryResult->Items->GetAt(i)->ConsumableBalance > 0)
							{
								plt->sendConsumeAllRequest(true);
								break;
							}
						}
					}
				}
				catch (Platform::Exception^ ex)
				{

				}
		}).join();
	}

}

// Called when the application is suspending.
void ViewProvider::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	dprintf("Suspending...\n");
    try
    {
	plt->xbEventPlayerSessionEnd();

	// this needs to be the last thing we do on suspend
	plt->suspendViewXbox();
}
    catch(Platform::Exception ^ex)
    {
        dprintf("Suspending view failed 0x%x\n", ex->HResult);
    }
}

// Called when the application is resuming from suspended.
void ViewProvider::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
    try
    {
	// this needs to be the first thing we do on resume
	plt->resumeViewXbox();

	// Note: that resume changes the suspend state to constrained state
	//		 then changes the constrained state to running state
	dprintf("Resuming...\n");
	
	if(	plt->currentUser && plt->xboxLiveContext && plt->currentUser->XboxUserId)	
	{
		//current user is signed in
		auto users = Windows::Xbox::System::User::Users;
		    
		for(int i = 0; i < users->Size; i++)
		{
                    if(users->GetAt(i) == plt->currentUser)
			{
				if(users->GetAt(i)->IsSignedIn)
				{
					if(	isStringEqual(users->GetAt(i)->XboxUserId, Windows::Xbox::ApplicationModel::Core::CoreApplicationContext::CurrentUser->XboxUserId))
					{
                        dprintf("resuming for returning player\n");

                        //checking if user's controller has changed
                        for(auto controller : Windows::Xbox::ApplicationModel::Core::CoreApplicationContext::CurrentUser->Controllers)
                        {
                            if (controller == plt->currentController)
                            {
                                // normal resume
                                plt->xbEventPlayerSessionStart();
                                plt->onNormalResume();
                                return;
                            }
                        }
					}
                    plt->returnToEngagementScreen(RBX::ReturnToEngage_ControllerChange);
                    return;
				}
                break;
			}
		}
		// if the current user is no longer signed in, what then?
        dprintf("resuming for absense of player\n");
        plt->returnToEngagementScreen(RBX::ReturnToEngage_SignOut);   
	}
    }
    catch(Platform::Exception ^ex)
    {
        dprintf("Resuming view failed 0x%x\n", ex->HResult);
    }
}


[Platform::MTAThread]
int main(Platform::Array< Platform::String^ >^) 
{
    // !!! Do not remove the following line, this makes sure mmdevapi.dll is linked
    SetWasapiThreadAffinityMask(SetWasapiThreadAffinityMask(1));
    Windows::ApplicationModel::Core::CoreApplication::Run( ref new ViewProviderFactory() );
}


extern int isRetail()
{
#ifdef _NOOPT
    return 0;
#else
    // let's hope the docs and the xbox dev forums are correct on this one
    // that the d:\ partition is "not available" on retail consoles
    static int x = 0 != _access("d:\\", 0);
    return x || FFlag::ForceRetail; // fflag is just in case if the d:\ drive is not enough, but the flag won't come in until the intro is initialized, so there might be some debug output happening
#endif
}


extern void dprintf( const char* fmt, ... )
{
    if( isRetail() ) return;

    va_list va;
    va_start( va, fmt );
    char buffer[4096];
    vsnprintf( buffer, sizeof(buffer), fmt, va );
    va_end(va);
    OutputDebugStringA(buffer);
}


