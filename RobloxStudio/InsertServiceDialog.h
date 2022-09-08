/**
 * InsertServiceDialog.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include <QDialog>

#include <boost/shared_ptr.hpp>
#include "V8DataModel/DataModel.h"

#include "RobloxMainWindow.h"

class InsertObjectListWidget;
class QListWidgetItem;
namespace RBX
{
    class SelectionChanged;
}

class InsertServiceDialog : public QDialog
{
	Q_OBJECT 

public:
	InsertServiceDialog(QWidget *pParentWidget);
	virtual ~InsertServiceDialog();
	
	void setDataModel(boost::shared_ptr<RBX::DataModel> pDataModel);

    bool isAvailable();

public Q_SLOTS:
	void updateWidget(bool state);
	/*override*/ void setVisible(bool visible);

private Q_SLOTS:
	void onAccepted();
	void onItemInsertRequested(QListWidgetItem* item);
	void onItemSelectionChanged();
	void redrawDialog();

private:	
	void onInstanceSelectionChanged(const RBX::SelectionChanged& evt);
	void requestDialogRedraw();

    void recreateWidget();
	
	boost::shared_ptr<RBX::DataModel>	m_pDataModel;
		
	InsertObjectListWidget			   *m_pInsertObjectListWidget;
	QPushButton						   *m_pInsertButton;
	
	bool							    m_bInitializationRequired;
	bool								m_bRedrawRequested;
};
