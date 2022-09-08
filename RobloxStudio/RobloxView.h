/**
 * RobloxView.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
    #define QT_NO_KEYWORDS
#endif

// Standard C/C++ Headers
#include <exception>

// Qt Headers
#include <QObject>
#include <QString>
#include <QPoint>
#include <QDateTime>

// 3rd Party Headers
#include "G3D/Vector2.h"

// Roblox Headers
#include "util/KeyCode.h"
#include "util/IMetric.h"
#include "util/InsertMode.h"
#include "V8DataModel/InputObject.h"
#include "v8tree/Instance.h"

// Purpose:
// Exception that means a popup should be created with the stored message.
class CreatePopupException : public std::exception
{
	public:
		CreatePopupException(const QString & message)
		{
			m_message = message;
		}

		virtual ~CreatePopupException() throw()
		{

		}

		QString m_message;
};


namespace RBX {
	class DataModel;
	class ViewBase;
	class FunctionMarshaller;
	class Game;
	class Instance;
    class ContentProvider;
	namespace Tasks {
		class Sequence;
	}
	namespace Profiling	{
		class CodeProfiler;
	}
}

class QOgreWidget;
class UserInput;
class RenderStatsItem;
class RobloxMainWindow;

class RobloxView : public QObject, public RBX::IMetric
{
    Q_OBJECT

public:

	static boost::shared_ptr<RBX::ViewBase> createGameView(QOgreWidget *pQtWrapperWindow);

	RobloxView(QOgreWidget *pQtWrapperWidget, boost::shared_ptr<RBX::Game> game, boost::shared_ptr<RBX::ViewBase> view, bool vr = false);
    virtual ~RobloxView(void);
    
    void init();
	
	//event handling
	void handleMouse(RBX::InputObject::UserInputType event,
                     RBX::InputObject::UserInputState state,
                     int x,
                     int y,
                     RBX::ModCode modifiers);
    
	void handleKey(RBX::InputObject::UserInputType event,
                   RBX::InputObject::UserInputState state,
                   RBX::KeyCode keyCode,
                   RBX::ModCode modifiers,
                   const char keyWithModifier,
				   bool processed = false);
    
	void handleScrollWheel(float delta, int x, int y);
	void handleFocus(bool focus);
	void handleMouseInside(bool inside);

	// used to set our engine's key state back to all keys up (used when ogre window loses focus)
	void resetKeyState();

	// Handle the Drop from HTML Toolbox JavaScript to Ogre Widget
	shared_ptr<const RBX::Instances> handleDropOperation(const QString&, int x, int y, bool &mouseCommandInvoked);
	void handleDropOperation(boost::shared_ptr<RBX::Instance> pInstanceToInsert, int x,  int y, bool &mouseCommandInvoked);
	void cancelDropOperation(bool resetMouseCommand);
	
	//view data handling
	void getCursorPos(G3D::Vector2 *pPos);
	void setCursorPos(G3D::Vector2 *pPos, bool isLMB, bool force = false, bool updatePos = true);

	void setCursorImage(bool forceShowCursor = false);

	bool hasApplicationFocus();

	void setBounds(unsigned int width, unsigned int height);
	unsigned int getWidth()  { return m_Width;  }
	unsigned int getHeight() { return m_Height; }

	//user input processing
	void processWrapMouse();
		
	//view update
	void requestUpdateView();
	void updateView();

	//IMetric override
	double getMetricValue(const std::string& metric) const;
	std::string getMetric(const std::string& metric) const;	

    void buildGui(bool buildInGameGui);
	void onEvent_playerAdded(shared_ptr<RBX::Instance> playerAdded);

	RBX::ViewBase& getViewBase() { return *m_pViewBase.get(); }
    
    virtual bool eventFilter(QObject * watched, QEvent * evt);

	static bool RbxViewCyclicFlagsEnabled();

private Q_SLOTS:

    void onInsertPart();
    void onInsertObject();
    void handleContextMenu();
    
private:

	class WrapMouseJob;
	class RenderJob;
	class RenderRequestJob;
	class RenderJobCyclic;

	void bindToWorkspace();
	void configureStats();
	bool isValidCursorPos(G3D::Vector2 *pPos, bool isLMB, bool force);

	bool setCursorImageInternal_deprecated(const std::string fileName);
	bool setCursorImageInternal(const RBX::ContentId& contentId, shared_ptr<const std::string> imageContent);

	void setDefaultCursorImage();

	void insertInstances(int x, int y, const RBX::Instances &instances, bool &mouseCommandInvoked, RBX::InsertMode insertMode = RBX::INSERT_TO_3D_VIEW);

    bool rightClickContextMenuAllowed();

	void handleMouseIconEnabledEvent(bool iconEnabled);
	void handleMouseCursorChange();
    void handleContextMenu(int x,int y);

	void handleTextBoxGainFocus();
	void handleTextBoxReleaseFocus();

    void updateHoverOver();

	void doRender(const double timeStamp);

	void loadContent(boost::shared_ptr<RBX::ContentProvider> cp, RBX::ContentId contentId, RBX::Instances& instances, bool& hasError);

	RBX::Instances                                      m_DraggedItems;
	
	boost::shared_ptr<RBX::ViewBase>					m_pViewBase;
	boost::scoped_ptr<UserInput>						m_pUserInput;
	
	boost::shared_ptr<RBX::DataModel> const				m_pDataModel;

	RBX::FunctionMarshaller								*m_pMarshaller;

	QOgreWidget											*m_pQtWrapperWidget;

	boost::shared_ptr<RenderJob>						m_pRenderJob;
	boost::shared_ptr<RenderRequestJob>					m_pRenderRequestJob;
	boost::shared_ptr<WrapMouseJob>						m_pWrapMouseJob;
	boost::shared_ptr<RenderJobCyclic>					m_pRenderJobCyclic;

	boost::shared_ptr<RenderStatsItem>					m_renderStatsItem;		// Used for displaying RenderStats in RBX::Stats
	boost::scoped_ptr<RBX::Profiling::CodeProfiler>		m_profilingSection;

	rbx::signals::scoped_connection						mouseCursorEnabledConnection;
	rbx::signals::scoped_connection						mouseCursorChangeConnection;
    rbx::signals::scoped_connection                     m_textBoxGainFocus;
    rbx::signals::scoped_connection                     m_textBoxReleaseFocus;
	rbx::signals::scoped_connection                     m_itemAddedToPlayersConnection;

	G3D::Vector2										m_previousCursorPosFraction;
	G3D::Vector2										m_previousCursorPos;
	G3D::Vector2										m_mousePressedPos;
    bool                                                m_bIgnoreCursorChange;
    bool                                                m_rightClickContextAllowed;
    bool                                                m_rightClickMenuPending;

	bool												m_mouseCursorHidden;
	
	unsigned int										m_Width;
	unsigned int										m_Height;

    RBX::Time                                           m_hoverOverLastTime;
    double                                              m_hoverOverAccum;

    QPoint                                              m_rightClickPosition;

    QDateTime                                           m_lastRightClickBlockingEventDate;
    QDateTime                                           m_rightClickMouseDownDate;

	bool												m_DataModelHashNeeded;
	int													m_updateViewStepId;
};
