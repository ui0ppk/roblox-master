/**
 * RobloxTaskScheduler.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Roblox Headers
#include "rbx/TaskScheduler.h"

// Roblox Studio Headers
#include "RobloxReportView.h"

class TSJobItem;

class ArbiterItem: public RobloxCategoryItem
{
public:
	ArbiterItem() {}
	TSJobItem* getOrCreateTSJobItem(boost::shared_ptr<const RBX::TaskScheduler::Job> pJob);
};

class TSJobItem: public QTreeWidgetItem
{
public:
	TSJobItem(ArbiterItem* pParentItem, boost::shared_ptr<const RBX::TaskScheduler::Job> pJob);

	boost::shared_ptr<const RBX::TaskScheduler::Job> getJob() { return m_pJob; }
	void updateValues();

private:
	boost::shared_ptr<const RBX::TaskScheduler::Job>	m_pJob;
};

class RobloxTaskScheduler : public RobloxReportView
{
	Q_OBJECT
public:
	RobloxTaskScheduler(); 
	virtual ~RobloxTaskScheduler();

public Q_SLOTS:
	void setVisible(bool visible);

private Q_SLOTS:
	void updateValues();

private:
	typedef std::vector<boost::shared_ptr<const RBX::TaskScheduler::Job> > Jobs;
	typedef std::map<boost::shared_ptr<RBX::TaskScheduler::Arbiter>, ArbiterItem*> ArbiterItemsMap;
	typedef std::set<TSJobItem*> TSJobItemCollection;

	ArbiterItem* getOrCreateArbiterItem(boost::shared_ptr<RBX::TaskScheduler::Arbiter> pArbiter);
	void syncTSJobItems(const TSJobItemCollection &currentTaskSchedulerItems);

	ArbiterItemsMap				 m_arbiterMap;
	TSJobItemCollection          m_TSJobItems;

	QTimer						*m_pTimer;
};


