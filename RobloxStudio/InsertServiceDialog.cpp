/**
 * InsertServiceDialog.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "InsertServiceDialog.h"

#include "V8DataModel/ChangeHistory.h"
#include "V8DataModel/DataModel.h"

// Qt Headers
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTimer>
#include <QGridLayout>

// Roblox Headers
#include "CommonInsertWidget.h"
#include "InsertObjectListWidgetItem.h"
#include "InsertObjectListWidget.h"

InsertServiceDialog::InsertServiceDialog(QWidget *pParentWidget)
: QDialog(pParentWidget, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
, m_bRedrawRequested(false)
, m_bInitializationRequired(true)
{
	setWindowTitle("Insert Service");
	setObjectName("Insert Service");

	m_pInsertObjectListWidget = new InsertObjectListWidget(this);
	m_pInsertObjectListWidget->setAutoScroll(true);
	m_pInsertObjectListWidget->setDragEnabled(true);
	m_pInsertObjectListWidget->setDragDropMode(QAbstractItemView::DragOnly);
	m_pInsertObjectListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	m_pInsertObjectListWidget->setDropIndicatorShown(true);
	m_pInsertObjectListWidget->setObjectName("InsertServiceListWidget");
	m_pInsertObjectListWidget->setWrapping(true);
	m_pInsertObjectListWidget->setResizeMode(QListView::Adjust);
	m_pInsertObjectListWidget->setAlternatingRowColors(true);

	m_pInsertButton = new QPushButton("Insert");
	m_pInsertButton->setEnabled(false);

	QDialogButtonBox* pDlgBtnBox = new QDialogButtonBox;
	pDlgBtnBox->addButton(m_pInsertButton, QDialogButtonBox::AcceptRole);
	pDlgBtnBox->addButton("Done", QDialogButtonBox::RejectRole);

	QGridLayout *pGridLayout = new QGridLayout();
	pGridLayout->setContentsMargins(0, 0, 0, 4);
	pGridLayout->setHorizontalSpacing(4);

	pGridLayout->addWidget(m_pInsertObjectListWidget, 0, 0);
	pGridLayout->addWidget(pDlgBtnBox, 1, 0);

	connect(pDlgBtnBox, SIGNAL(accepted()), this, SLOT(onAccepted()));
	connect(pDlgBtnBox, SIGNAL(rejected()), this, SLOT(reject()));

	connect(m_pInsertObjectListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onItemInsertRequested(QListWidgetItem*)));
	connect(m_pInsertObjectListWidget, SIGNAL(enterKeyPressed(QListWidgetItem*)), this, SLOT(onItemInsertRequested(QListWidgetItem*)));
	connect(m_pInsertObjectListWidget, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));

	setLayout(pGridLayout);

	resize(200,250);
}

InsertServiceDialog::~InsertServiceDialog()
{
	m_pInsertObjectListWidget->clear();	
}

void InsertServiceDialog::setDataModel(boost::shared_ptr<RBX::DataModel> pDataModel)
{
	if(m_pDataModel == pDataModel)
		return;

	m_pInsertObjectListWidget->clear();	
	m_bInitializationRequired = true;

	m_pDataModel = pDataModel;

	if (m_pDataModel && isVisible())
	{
		requestDialogRedraw();
	}
}

void InsertServiceDialog::updateWidget(bool state)
{
	if (state)
		requestDialogRedraw();
}

void InsertServiceDialog::setVisible(bool visible)
{
	updateWidget(visible);
	QWidget::setVisible(visible);
}

void InsertServiceDialog::recreateWidget()
{
    m_pInsertObjectListWidget->clear();
    m_pInsertObjectListWidget->setUpdatesEnabled(false);
    InsertObjectWidget::createWidgets(true, m_pInsertObjectListWidget, &RBX::Reflection::ClassDescriptor::rootDescriptor(), m_pDataModel);
    m_pInsertObjectListWidget->sortItems();
    m_pInsertObjectListWidget->setUpdatesEnabled(true);
}

bool InsertServiceDialog::isAvailable()
{
    m_bInitializationRequired = false;
    recreateWidget();

    if (m_pInsertObjectListWidget->count() > 0)
        return true;

    return false;
}

void InsertServiceDialog::onItemInsertRequested(QListWidgetItem* pListWidgetItem)
{
	if(!pListWidgetItem)
		return;

	InsertObjectListWidgetItem* pInsertListWidgetItem = dynamic_cast<InsertObjectListWidgetItem*>(pListWidgetItem);
	if(!pInsertListWidgetItem)
		return;

	try
	{
		RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);

		// The user requested a Service. Don't use the one we had already created.
		// Instead, have the ServiceProvider create it if necessary
		boost::shared_ptr<RBX::Instance> pService = RBX::ServiceProvider::create(m_pDataModel.get(), pInsertListWidgetItem->getInstance()->getClassName());
		if (!pService)
			throw std::runtime_error(QString("Failed to create Service object of type %s").arg(pInsertListWidgetItem->getInstance()->getClassNameStr().c_str()).toStdString());

		// TODO -- currently this undo/redo doesn't work
		//for undo redo -- 
		std::string action = RBX::format("Insert %s", pService->getName().c_str());
		RBX::ChangeHistoryService::requestWaypoint(action.c_str(), m_pDataModel.get());
	}
	catch(std::bad_alloc&)
	{
		throw;
	}
	catch(std::exception& e)
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
	}

    if (!isAvailable())
    {
        RobloxMainWindow* mainWindow = static_cast<RobloxMainWindow*>(this->parentWidget());
        mainWindow->insertServiceAction->setEnabled(false);
    }

	accept();
}

void InsertServiceDialog::requestDialogRedraw()
{
	if(m_bRedrawRequested || !m_pDataModel)
		return;

	QTimer::singleShot(0, this, SLOT(redrawDialog()));
	m_bRedrawRequested = true;
}

void InsertServiceDialog::redrawDialog()
{
	if (m_pDataModel)
	{
        if (m_bInitializationRequired)
        {
            m_bInitializationRequired = false;
            recreateWidget();
        }

		int numChild = m_pInsertObjectListWidget->count();
		int currentChild = 0;
		InsertObjectListWidgetItem *pCurrentItem = NULL;
		while (currentChild < numChild)
		{		
			pCurrentItem = static_cast<InsertObjectListWidgetItem*>(m_pInsertObjectListWidget->item(currentChild));
			if (pCurrentItem)
			{
				RBXASSERT(dynamic_cast<RBX::Service*>(pCurrentItem->getInstance().get()));
				//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "InsertService: current object = %s and visible = %d", pCurrentItem->getInstance()->getClassNameStr().c_str(), visible);
				pCurrentItem->setHidden(false);
			}
			++currentChild;
		}
	}
	m_bRedrawRequested = false;
}

void InsertServiceDialog::onAccepted()
{
	if(m_pInsertObjectListWidget->currentItem())
		onItemInsertRequested(m_pInsertObjectListWidget->currentItem());
}

void InsertServiceDialog::onItemSelectionChanged()
{
	if(m_pInsertObjectListWidget->currentItem() && !m_pInsertButton->isEnabled())
	{
		m_pInsertButton->setEnabled(true);
	}
}


