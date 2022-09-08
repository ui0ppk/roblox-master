/**
 * IRobloxDoc.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

class QString;
class QWidget;
class IExternalHandler;
class RobloxMainWindow;
class QIcon;

enum EDockWindowID
{
    eDW_TOOLBOX,
    eDW_BASIC_OBJECTS,
    eDW_OBJECT_EXPLORER,
    eDW_PROPERTIES,
	eDW_OUTPUT,
    eDW_CONTEXTUAL_HELP,
    eDW_SCRIPT_REVIEW,
    eDW_TASK_SCHEDULER,
    eDW_DIAGNOSTICS,
	eDW_FIND,
    eDW_GAME_EXPLORER,
	eDW_TUTORIALS,
	eDW_SCRIPT_ANALYSIS,
	eDW_TEAM_CREATE,
    eDW_MAX
};

class IRobloxDoc
{
public:

	enum RBXDocType
	{
		IDE, 
        BROWSER, 
        SCRIPT, 
        OBJECTBROWSER
	};
	
	enum RBXCloseRequest
	{
		REQUEST_HANDLED,
        LOCAL_SAVE_NEEDED, 
        NO_SAVE_NEEDED, 
        CLOSE_CANCELED
	};
	
	virtual bool open(RobloxMainWindow *pMainWindow, const QString &fileName) = 0;
	virtual bool close() = 0;
	
	virtual RBXCloseRequest requestClose() = 0;

	virtual RBXDocType docType() = 0;

	virtual QString fileName() const = 0;
	virtual QString displayName() const = 0;
	virtual QString windowTitle() const = 0;
	virtual QString keyName() const = 0;
    virtual const QIcon& titleIcon() const = 0;
    virtual const QString& titleTooltip() const = 0;
	
	virtual QWidget* getViewer() = 0;
	
	virtual bool isModified() = 0;
	
	virtual bool save() = 0;
	virtual bool saveAs(const QString &fileName) = 0;

	virtual QString saveFileFilters() = 0;
	
	virtual void activate() = 0;
	virtual void deActivate() = 0;

	virtual void updateUI(bool state) = 0;

	virtual void addExternalHandler(IExternalHandler* extHandler) = 0;
	virtual void removeExternalHandler(const QString &handlerId) = 0;
	virtual IExternalHandler* getExternalHandler(const QString &handlerId) = 0;
		
	virtual bool handleAction(const QString &actionID, bool isChecked = false) = 0;
	virtual bool actionState(const QString &actionID, bool &enableState, bool &checkedState) = 0;

	virtual bool handleDrop(const QString &fileName) = 0;

	virtual bool handlePluginAction(void *pNotifier, void *pAction) = 0;

	virtual void handleScriptCommand(const QString &execCommand) = 0;
};
