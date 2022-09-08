/**
 * RobloxSettingsDialog.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxSettingsDialog.h"

// Qt Headers
#include <QApplication>
#include <QHeaderView>
#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSettings>

// Roblox Headers
#include "v8datamodel/GlobalSettings.h"
#include "v8datamodel/Selection.h"
#include "ReflectionMetaData.h"

// Roblox Studio Headers
#include "RobloxCustomWidgets.h"
#include "QtUtilities.h"
#include "AuthoringSettings.h"

static const char* StudioSettings = "Studio";
static const char* GeometrySetting = "RobloxSettingsDialog/Geometry";
static const char* SplitterStateSetting = "RobloxSettingsDialog/SplitterState";

RobloxSettingsDialog::RobloxSettingsDialog(QWidget* parent)
:QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
	m_SettingsTypesTable = new QTableWidget(this);
	m_SettingsTree = new SelectionPropertyTree(this);

	//m_SettingsTypesTable->setFrameStyle(QFrame::NoFrame);
	m_SettingsTypesTable->setColumnCount(1);
	QStringList headerLabels("Names");
	m_SettingsTypesTable->setHorizontalHeaderLabels(headerLabels);
	m_SettingsTypesTable->verticalHeader()->hide();
	m_SettingsTypesTable->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
	m_SettingsTypesTable->horizontalHeader()->setClickable(true);
	m_SettingsTypesTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	m_SettingsTypesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_SettingsTypesTable->setAlternatingRowColors(true);
    m_SettingsTypesTable->setSelectionMode(QAbstractItemView::SingleSelection);
	connect(m_SettingsTypesTable, SIGNAL(currentItemChanged(QTableWidgetItem*, QTableWidgetItem*)), this, SLOT(onCategorySelected(QTableWidgetItem*, QTableWidgetItem*)));
	
	//m_SettingsTree->setFrameStyle(QFrame::NoFrame);
	m_SettingsTree->setHeaderHidden(true);
	
	m_hSplitter = new QSplitter(this);
	m_hSplitter->addWidget(m_SettingsTypesTable);
	m_hSplitter->addWidget(m_SettingsTree);
    
    m_hSplitter->setCollapsible(0,false);
    m_hSplitter->setCollapsible(1,false);

	// Add the Reset and close buttons
	QPushButton *closeButton = new QPushButton(tr("Close"));
	closeButton->setAutoDefault(false);
	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

	QPushButton *resetButton = new QPushButton(tr("Reset All Settings"));
	resetButton->setAutoDefault(false);
	connect(resetButton, SIGNAL(clicked()), this, SLOT(onReset()));

	QHBoxLayout *buttonsLayout = new QHBoxLayout;
	buttonsLayout->addWidget(resetButton, 1, Qt::AlignLeft);
	buttonsLayout->addWidget(closeButton, 1, Qt::AlignRight);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(m_hSplitter);
	layout->addLayout(buttonsLayout);
	setLayout(layout);

	populate();

	m_SettingsTypesTable->setMinimumWidth(200);
	m_SettingsTree->setMinimumWidth(400);
	m_SettingsTree->header()->resizeSection(0, 200);
	setMinimumWidth(600);
	setMinimumHeight(370);

	QtUtilities::updateWindowTitle(this);

    // restore window state
    QSettings settings;
    restoreGeometry(settings.value(GeometrySetting).toByteArray());
    
    // restore splitter state
    QList<int> sizes;
    sizes.append(200);
    sizes.append(width() - sizes[0]);
    m_hSplitter->setSizes(sizes);
    m_hSplitter->restoreState(settings.value(SplitterStateSetting).toByteArray());
}

RobloxSettingsDialog::~RobloxSettingsDialog()
{
    // save window state
    QSettings settings;
    settings.setValue(GeometrySetting,saveGeometry());
    settings.setValue(SplitterStateSetting,m_hSplitter->saveState());
}

void RobloxSettingsDialog::onReset()
{
    static bool hasSeenWarning = false;

    if ( !hasSeenWarning )
    {
        QMessageBox::information(
            this,
            "Reset Settings",
            "Resetting settings will not take effect until you restart ROBLOX" );
        hasSeenWarning = true;
    }

	RBX::GlobalAdvancedSettings::singleton()->eraseSettingsStore();
}

bool RobloxSettingsDialog::close()
{
    static bool hasSeenWarning = false;

    if ( !hasSeenWarning )
    {
        QMessageBox::information(
            this,
            "Save Changes",
            "Some setting changes will not take effect until you restart ROBLOX" );
        hasSeenWarning = true;
    }

	RBX::GlobalAdvancedSettings::singleton()->saveState();

    return QDialog::close();
}

void RobloxSettingsDialog::populate()
{
	boost::shared_ptr<RBX::Instance> container = RBX::shared_from(RBX::GlobalAdvancedSettings::singleton().get());
	if(!container)
		return;

	m_descendantAddedConnection.disconnect();
	m_descendantRemovingConnection.disconnect();
	m_SettingsTypesTable->clearContents();

	container->visitDescendants(boost::bind(&RobloxSettingsDialog::onDescendantAdded, this, _1));
	m_descendantAddedConnection = container->getOrCreateDescendantAddedSignal()->connect(boost::bind(&RobloxSettingsDialog::onDescendantAdded, this, _1));
	m_descendantRemovingConnection = container->getOrCreateDescendantRemovingSignal()->connect(boost::bind(&RobloxSettingsDialog::onDescendantRemoving, this, _1));

	m_SettingsTypesTable->setSortingEnabled(true);
	m_SettingsTypesTable->horizontalHeader()->stretchLastSection();

	// Set settings property tree selection
	RBX::Selection* sel = RBX::ServiceProvider::create<RBX::Selection>(RBX::GlobalAdvancedSettings::singleton().get());
	m_SettingsTree->setSelection(sel);

    QList<QTableWidgetItem*> studioItems = m_SettingsTypesTable->findItems(StudioSettings,Qt::MatchExactly);
    if ( !studioItems.isEmpty() )
        m_SettingsTypesTable->setCurrentItem(studioItems[0]);
}

void RobloxSettingsDialog::onDescendantAdded(boost::shared_ptr<RBX::Instance> child)
{
	if(!child)
		return;

	const RBX::Reflection::ClassDescriptor* pClassDescriptor = &child->getDescriptor();
	if(pClassDescriptor)
	{
		if(!hasProperties(pClassDescriptor))
			return;
	}

	QString itemName(child->getName().c_str());
	SettingsItem* pWidgetItem = new SettingsItem(child, itemName);
	int row = m_SettingsTypesTable->rowCount();
	m_SettingsTypesTable->insertRow(row);
	m_SettingsTypesTable->setItem(row, 0, pWidgetItem);
}

bool RobloxSettingsDialog::hasProperties(const RBX::Reflection::ClassDescriptor* pClassDescriptor)
{
	if(!pClassDescriptor)
		return false;

	bool retVal = false;
	RBX::Reflection::ClassDescriptor::ClassDescriptors::const_iterator iter = pClassDescriptor->derivedClasses_begin();
	for(; iter != pClassDescriptor->derivedClasses_end(); iter++)
	{
		RBX::Reflection::ClassDescriptor* pDerived = *iter;
		if(pDerived)
			retVal = hasProperties(pDerived);

		if(retVal)
			break;
	}

	if(!retVal)
	{
		RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::PropertyDescriptor>::Collection::const_iterator iter = pClassDescriptor->begin<RBX::Reflection::PropertyDescriptor>();
		for(; iter != pClassDescriptor->end<RBX::Reflection::PropertyDescriptor>(); iter++)
		{
			const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor = *iter;
			if(pPropertyDescriptor)
			{
				if ( pPropertyDescriptor->owner != *pClassDescriptor )
					continue;

				if ( !pPropertyDescriptor->isPublic() || pPropertyDescriptor->security != RBX::Security::None )
				{
					continue;
                }

				RBX::Reflection::Metadata::Member* pMemeber = RBX::Reflection::Metadata::Reflection::singleton()->get(*pPropertyDescriptor);
				if(RBX::Reflection::Metadata::Item::isDeprecated(pMemeber, *pPropertyDescriptor))
					continue;

				retVal = true;
				break;
			}
		}
	}

	return retVal;
}

void RobloxSettingsDialog::onDescendantRemoving(boost::shared_ptr<RBX::Instance> child)
{
	if(!child)
		return;

	QString itemName(child->getName().c_str());
	QList<QTableWidgetItem *> tableItems = m_SettingsTypesTable->findItems(itemName, Qt::MatchExactly);
	if(tableItems.empty())
		return;
	QTableWidgetItem* item = tableItems[0];
	if(item != NULL)
		m_SettingsTypesTable->removeRow(item->row());
}

void RobloxSettingsDialog::onCategorySelected(QTableWidgetItem* current, QTableWidgetItem* previous)
{
	if(!current)
		return;

	SettingsItem* item = dynamic_cast<SettingsItem*>(current);
	if(!item)
		return;

	RBX::Selection* sel = m_SettingsTree->getSelection();
	if(!sel)
		return;
	
	sel->clearSelection();
	sel->addToSelection(item->getInstance().get());
}


SelectionPropertyTree::SelectionPropertyTree(QWidget* parent)
: PropertyTreeWidget(parent)
{
}

SelectionPropertyTree::~SelectionPropertyTree()
{
#ifdef Q_WS_MAC
    // On Mac, intermittent crash during deletion of GlobalAdvancedSetting requires us to explictly remove selection (if any)
    if (m_selection)
        m_selection->clearSelection();
#endif
	setSelection(NULL);
}

void SelectionPropertyTree::addSelectionInstance(boost::shared_ptr<RBX::Instance> pInstance)
{
	if (!pInstance)
		return;
	
	m_selectedInstances.push_back(pInstance.get());
	addInstance(pInstance.get());
}

void SelectionPropertyTree::removeSelectionInstance(boost::shared_ptr<RBX::Instance> pInstance)
{
	if (!pInstance)
		return;
	
	InstanceList::iterator iter = std::find(m_selectedInstances.begin(), m_selectedInstances.end(), pInstance.get());
	if (iter != m_selectedInstances.end())
		m_selectedInstances.erase(iter);
	removeInstance(pInstance.get());
}


void SelectionPropertyTree::setSelection(RBX::Selection* value)
{
	m_selectionChangedConnection.disconnect();
	reinitialize();
	m_selectedInstances.clear();
	
	m_selection = shared_from(value);
	if (m_selection)
	{
		std::for_each(m_selection->begin(), m_selection->end(), boost::bind(&SelectionPropertyTree::addSelectionInstance, this, _1));
		m_selectionChangedConnection = m_selection->selectionChanged.connect(boost::bind(&SelectionPropertyTree::onSelectionChanged, this, _1));
	}

	requestUpdate();	
}

void SelectionPropertyTree::onSelectionChanged(const RBX::SelectionChanged& evt)
{
	QMutexLocker lock(&m_propertyWidgetMutex);
	if (evt.addedItem)
    {
        static bool hasShownWarning = false;

        if ( evt.addedItem->getName() != StudioSettings && !hasShownWarning )
        {
            QMessageBox::warning(
                this,
                "Warning",
                "<p>Changing settings may make ROBLOX unstable.</p>"
                "<p>We recommend that you don't change any non-Studio settings "
                "unless instructed to do so by a ROBLOX engineer.</p>" );
            hasShownWarning = true;
        }

		addSelectionInstance(evt.addedItem);
    }
	if (evt.removedItem)
		removeSelectionInstance(evt.removedItem);

	requestUpdate();
}

void SelectionPropertyTree::requestUpdate()
{
	QApplication::postEvent(this, new RobloxCustomEvent(PROPERTY_WIDGET_UPDATE));
}

void SelectionPropertyTree::initProperty(const RBX::Reflection::ClassDescriptor* pClassDescriptor, 
										  const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor)
{
	if (!pPropertyDescriptor)
		return;

	// Filter Instance properties
	if (pPropertyDescriptor->owner != RBX::Instance::classDescriptor())
	{
		PropertyTreeWidget::initProperty(pClassDescriptor, pPropertyDescriptor);
	}
}

SettingsItem::SettingsItem(boost::shared_ptr<RBX::Instance> inst, QString name)
: QTableWidgetItem(name)
, m_instance(inst)
{}
