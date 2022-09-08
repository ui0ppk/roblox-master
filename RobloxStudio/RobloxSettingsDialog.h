/**
 * RobloxSettingsDialog.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
    #define QT_NO_KEYWORDS
#endif

// Qt Headers
#include <QDialog>
#include <QTableWidget>

// Roblox Studio Headers
#include "RobloxPropertyWidget.h"

namespace RBX {
	class Instance;
	class Selection;
	class SelectionChanged;
}

class QSplitter;

class SettingsItem : public QTableWidgetItem
{	
public:
	SettingsItem(boost::shared_ptr<RBX::Instance> inst, QString name);
	boost::shared_ptr<RBX::Instance> getInstance() { return m_instance; }

private:
	boost::shared_ptr<RBX::Instance> m_instance;
};

class SelectionPropertyTree : public PropertyTreeWidget
{
public:
	SelectionPropertyTree(QWidget* parent = 0);
	~SelectionPropertyTree();
	void setSelection(RBX::Selection* selection);
	RBX::Selection* getSelection() { return m_selection.get(); }

	void requestUpdate();
	
private:
	void initProperty(const RBX::Reflection::ClassDescriptor* pClassDescriptor, 
		              const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor);
	void onSelectionChanged(const RBX::SelectionChanged& evt);
	void addSelectionInstance(boost::shared_ptr<RBX::Instance> pInstance);
	void removeSelectionInstance(boost::shared_ptr<RBX::Instance> pInstance);

	boost::shared_ptr<RBX::Selection>		m_selection;
	rbx::signals::scoped_connection			m_selectionChangedConnection;

	InstanceList							m_selectedInstances;

	QMutex                                  m_propertyWidgetMutex;

};


class RobloxSettingsDialog : public QDialog
{

Q_OBJECT
	
public:
	RobloxSettingsDialog(QWidget* parent);
    virtual ~RobloxSettingsDialog();

private Q_SLOTS:
	void onCategorySelected(QTableWidgetItem* current, QTableWidgetItem* previous);
	void onReset();

	/*override*/bool close();

private:
	void populate();

	bool hasProperties(const RBX::Reflection::ClassDescriptor* pDescriptor);

	void onDescendantAdded(boost::shared_ptr<RBX::Instance> child);
	void onDescendantRemoving(boost::shared_ptr<RBX::Instance> child);

	rbx::signals::scoped_connection m_descendantAddedConnection;
	rbx::signals::scoped_connection m_descendantRemovingConnection;

	QSplitter				*m_hSplitter;
	QTableWidget			*m_SettingsTypesTable;
	SelectionPropertyTree	*m_SettingsTree;

};	
