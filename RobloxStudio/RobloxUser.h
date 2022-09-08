/**
 * RobloxUser.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

// Qt headers
#include <QObject>
#include "util/HttpAsync.h"

class RobloxUser : public QObject
{
	Q_OBJECT

Q_SIGNALS:
	void userLoaded();

public:
	static RobloxUser& singleton();	
	void init();
	int getUserId();

public Q_SLOTS:
	void onAuthenticationChanged(bool);

private:
	RobloxUser();
	~RobloxUser();
	void getWebkitUserId();
	void currentUserReplied(RBX::HttpFuture future);
	int m_webKitUserId; // -1 means not initialized, 0 means not authenticated, > 0 means logged in as that userid
    boost::shared_future<void> m_webKitUserIDQuery;
};
