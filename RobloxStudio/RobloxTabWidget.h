/**
 * RobloxTabWidget.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QTabWidget>

class RobloxTabWidget : public QTabWidget
{
    public:

        RobloxTabWidget(QWidget* parent)
            : QTabWidget(parent)
        {
        }

        QTabBar& getTabBar() { return *tabBar(); }
};


