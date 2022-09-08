/**
 * RobloxMouseConfig.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include <QWidget>
#include "V8DataModel/InputObject.h"

class UserInput;
class QComboBox;

//

class RobloxMouseConfig
{
public:
    RobloxMouseConfig();
    
    static RobloxMouseConfig& singleton();

    void loadMouseConfig();
    void saveMouseConfig();

    enum MouseButtonsAndModifiers
    {
        MOUSEBUTTON_LEFT =      1ul << 0,
        MOUSEBUTTON_MIDDLE =    1ul << 1,
        MOUSEBUTTON_RIGHT =     1ul << 2,
        MOUSEMODIFIER_SHIFT =   1ul << 3,
        MOUSEMODIFIER_ALT =     1ul << 4,
        MOUSEMODIFIER_CONTROL = 1ul << 5
    };

    unsigned int contextMenuConfig() const
        { return m_contextMenuConfig; }
        
    void setContextMenuConfig(int config)
        { m_contextMenuConfig = config; }

    bool canOpenContextMenu(RBX::InputObject::UserInputType inputType,
                            RBX::InputObject::UserInputState inputState);

private:
    unsigned int m_contextMenuConfig;
};


class RobloxMouseConfigWidget : public QWidget
{
    Q_OBJECT
public:
    RobloxMouseConfigWidget(QWidget* parent = 0);

Q_SIGNALS:
    void dataChanged();

public Q_SLOTS:
    void accept();
    void cancel();
    void restoreAllDefaults();

private Q_SLOTS:
    void contextMenuOptionSelected(const QString& option);

private:
    void initialize();
    
    void updateContextMenuOption();
    
    unsigned int m_contextMenuOverride;
    QComboBox* m_contextMenuOption;
};
