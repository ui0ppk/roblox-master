/**
 * RobloxTaskScheduler.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxTaskScheduler.h"

// Qt Headers
#include <QTimer>
#include <QHeaderView>

// Roblox Headers
#include "rbx/TaskScheduler.h"
#include "rbx/TaskScheduler.Job.h"

// Roblox Studio Headers
#include "QtUtilities.h"

static const int UpdateInterval = 1000;
static const int sMaxColumnCount = 7;

TSJobItem* ArbiterItem::getOrCreateTSJobItem(boost::shared_ptr<const RBX::TaskScheduler::Job> pJob)
{
	int currentIndex = 0;
	TSJobItem *pTSItem = NULL;
	while (currentIndex < childCount())
	{
		//static cast since we know all of the inserted child items are of type TSJobItem
		pTSItem = static_cast<TSJobItem*>(child(currentIndex));
		if (pTSItem && pTSItem->getJob() == pJob)
			return pTSItem;
		++currentIndex;
	}

	//we couldn't find an item, so create one
	return new TSJobItem(this, pJob);
}

TSJobItem::TSJobItem(ArbiterItem* pParentItem, boost::shared_ptr<const RBX::TaskScheduler::Job> pJob)
: m_pJob(pJob)
{
	pParentItem->addChild(this);
	setText(0, m_pJob->name.c_str());
}

void TSJobItem::updateValues()
{
	bool isSleeping = m_pJob->getSleepingTime() > RBX::Time::Interval(5);
	if (!isSleeping)
	{
		double error = m_pJob->averageError();
		setText(1, RBX::format("%.2f", error).c_str());

		double priority = m_pJob->getPriority();
		setText(2, RBX::format("%.1f", priority).c_str());

		double dutyCycle = m_pJob->averageDutyCycle();
		setText(3, RBX::format("%.1f%%", 100.0 * dutyCycle).c_str());

		double sps = m_pJob->averageStepsPerSecond();
		setText(4, RBX::format("%.1f/s", sps).c_str());

		double cv = m_pJob->getStepStats().stepInterval().coefficient_of_variation();
		setText(5, RBX::format("%.1f%%", 100.0 * cv).c_str());

		double step = m_pJob->averageStepTime() * 1000;
		setText(6, RBX::format("%.3fms", step).c_str());
	}
	else
	{
		// start the count from 1 as the 0 column is job's name and should not be modified
		for (int ii=1; ii < sMaxColumnCount; ++ii)
			setText(ii, "");
	}

	setIcon(0, QIcon(QtUtilities::getPixmap(":/images/TASKSCHEDULERIMAGES.bmp", (int)m_pJob->getState(), 16, true)));
}

RobloxTaskScheduler::RobloxTaskScheduler()
: m_pTimer(new QTimer(this))
{
	setColumnCount(sMaxColumnCount);
	QStringList headerLabels;
	headerLabels << "Name" << "Error" << "Priority" << "Activity" << "Rate" << "CV" << "Time";
	setHeaderLabels(headerLabels);

	setSortingEnabled(true);

	header()->setDefaultSectionSize(50);
	header()->resizeSection(0, 100);
	header()->setSortIndicator(0, Qt::AscendingOrder);

    m_pTimer->setInterval(UpdateInterval);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateValues()));
}

RobloxTaskScheduler::~RobloxTaskScheduler()
{}

void RobloxTaskScheduler::updateValues()
{
    // if the window is hidden, stop the update timer
    if ( !isVisible() )
    {
        m_pTimer->stop();
        return;
    }

	setUpdatesEnabled(false);

	Jobs jobs;
	RBX::TaskScheduler::singleton().getJobsInfo(jobs);

	std::set<TSJobItem *> currentTSJobItems;
	ArbiterItem* pArbiterItem = NULL;
	TSJobItem *pTSJobItem = NULL;

	for(Jobs::iterator iter = jobs.begin(); iter!=jobs.end(); ++iter)
	{
		boost::shared_ptr<const RBX::TaskScheduler::Job> pJob = (*iter);
		if(pJob)
		{
			boost::shared_ptr<RBX::TaskScheduler::Arbiter> pArbiter(pJob->getArbiter());
			if (!pArbiter)
				continue;

			pArbiterItem = getOrCreateArbiterItem(pArbiter);
			RBXASSERT(pArbiterItem);
			
			QString arbiterName = pArbiter->arbiterName().c_str();
			if(pArbiter->isThrottled())
				arbiterName += " (throttled)";

			if (arbiterName != pArbiterItem->text(0))
				pArbiterItem->setText(0, arbiterName);

			pTSJobItem = pArbiterItem->getOrCreateTSJobItem(pJob);
			RBXASSERT(pTSJobItem);
			
			pTSJobItem->updateValues();
			currentTSJobItems.insert(pTSJobItem);
		}
	}

	syncTSJobItems(currentTSJobItems);
	sortItems(header()->sortIndicatorSection(), header()->sortIndicatorOrder());
	
	setUpdatesEnabled(true);
}

void RobloxTaskScheduler::setVisible(bool visible)
{
	if(visible)
	{
		m_pTimer->start();
	}
	else
	{
		m_pTimer->stop();
	}
	RobloxReportView::setVisible(visible);
}

ArbiterItem* RobloxTaskScheduler::getOrCreateArbiterItem(boost::shared_ptr<RBX::TaskScheduler::Arbiter> pArbiter)
{
	ArbiterItemsMap::const_iterator iter = m_arbiterMap.find(pArbiter);
	if (iter != m_arbiterMap.end())
		return iter->second;

	ArbiterItem *pArbiterItem = new ArbiterItem();
	addCategoryItem(pArbiterItem);
	pArbiterItem->setExpanded(true);
	//add it into map
	m_arbiterMap[pArbiter] = pArbiterItem;

	return pArbiterItem;
}

void RobloxTaskScheduler::syncTSJobItems(const std::set<TSJobItem *> &currentTSJobItems)
{
	//delete items which are not there in current items collection
	// -- not removing iterator as we are going to copy current items collection
	std::set<TSJobItem *>::iterator iter = m_TSJobItems.begin();
	while (iter != m_TSJobItems.end())
	{
		if (currentTSJobItems.find(*iter) == currentTSJobItems.end())
			delete *iter;
		++iter;
	}

	//remove top level items if they do not have any children
	ArbiterItemsMap::iterator iter1 = m_arbiterMap.begin();
	while (iter1 != m_arbiterMap.end())
	{
		if (iter1->second->childCount() < 1)
		{
			delete iter1->second;
			m_arbiterMap.erase(iter1++);
		}
		else
		{
			++iter1;
		}
	}

	m_TSJobItems = currentTSJobItems;
}
