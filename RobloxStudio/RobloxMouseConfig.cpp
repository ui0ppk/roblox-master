/**
 * RobloxMouseConfig.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxMouseConfig.h"

#include "util/NavKeys.h"
#include "V8DataModel/InputObject.h"

#include <QLayout>
#include <QLabel>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QString>
#include <QApplication>
#include <QSettings>

RobloxMouseConfig::RobloxMouseConfig()
    : m_contextMenuConfig(MOUSEBUTTON_RIGHT)
{

}

void RobloxMouseConfig::loadMouseConfig()
{
    QSettings settings;
    settings.beginGroup("Roblox Studio Mouse Mapping");
    if (!settings.childKeys().isEmpty())
        m_contextMenuConfig = settings.value("contextMenu").toUInt();
    settings.endGroup();
}

void RobloxMouseConfig::saveMouseConfig()
{
    QSettings settings;
    settings.beginGroup("Roblox Studio Mouse Mapping");
    settings.remove("");
    settings.endGroup();
    
    settings.beginGroup("Roblox Studio Mouse Mapping");
    settings.setValue("contextMenu", m_contextMenuConfig);
    settings.endGroup();
}

RobloxMouseConfig& RobloxMouseConfig::singleton()
{
    static RobloxMouseConfig* config = new RobloxMouseConfig();
    return *config;
}

bool RobloxMouseConfig::canOpenContextMenu(RBX::InputObject::UserInputType inputType,
                                           RBX::InputObject::UserInputState inputState)
{
    if (m_contextMenuConfig == 0)
        return false;
    
    bool buttonDown = 0;

    if ((m_contextMenuConfig & MOUSEBUTTON_RIGHT) == MOUSEBUTTON_RIGHT &&
        inputState == RBX::InputObject::INPUT_STATE_END && 
        inputType == RBX::InputObject::TYPE_MOUSEBUTTON2)
        buttonDown = true;

    Qt::KeyboardModifiers modifiers = QApplication::queryKeyboardModifiers();

    if (buttonDown &&
        m_contextMenuConfig == MOUSEBUTTON_RIGHT &&
        !(inputState == RBX::InputObject::INPUT_STATE_END && 
          inputType == RBX::InputObject::TYPE_MOUSEBUTTON1) &&
        (modifiers & Qt::ShiftModifier) == 0 &&
        (modifiers & Qt::AltModifier) == 0 &&
        (modifiers & Qt::ControlModifier) == 0
        )
        return true;

    if (buttonDown && m_contextMenuConfig & MOUSEMODIFIER_SHIFT)
    {
        if (modifiers & Qt::ShiftModifier)
            return true;
    }
    if (buttonDown && m_contextMenuConfig & MOUSEMODIFIER_ALT)
    {
        if (modifiers & Qt::AltModifier)
            return true;
    }
    if (buttonDown && m_contextMenuConfig & MOUSEMODIFIER_CONTROL)
    {
        if (modifiers & Qt::ControlModifier)
            return true;
    }

    return false;
}

RobloxMouseConfigWidget::RobloxMouseConfigWidget(QWidget* parent)
    : QWidget(parent)
    , m_contextMenuOption(NULL)
{
    initialize();
}

void RobloxMouseConfigWidget::initialize()
{
    m_contextMenuOverride = RobloxMouseConfig::singleton().contextMenuConfig();

    QLayout* layout = new QVBoxLayout;
    
    QWidget* widget = new QWidget();
    QHBoxLayout* mouseConfigItemLayout = new QHBoxLayout;
    
    QLabel* label = new QLabel("Context Menu");
    label->setContentsMargins(0, 0, 18, 0);
    label->setAlignment(Qt::AlignRight);
    mouseConfigItemLayout->addWidget(label);
    
    m_contextMenuOption = new QComboBox();
    m_contextMenuOption->addItem("Off");
    m_contextMenuOption->addItem("Right Click");
    m_contextMenuOption->addItem("Right Click + Shift");
    m_contextMenuOption->addItem("Right Click + Alt");
    m_contextMenuOption->addItem("Right Click + Ctrl");
    
    updateContextMenuOption();
    
    connect(m_contextMenuOption, SIGNAL(currentIndexChanged(const QString&)),
            this,                     SLOT(contextMenuOptionSelected(const QString&)));
    
    mouseConfigItemLayout->addWidget(m_contextMenuOption);
    widget->setLayout(mouseConfigItemLayout);
    layout->addWidget(widget);
    
    setLayout(layout);
    
}

void RobloxMouseConfigWidget::updateContextMenuOption()
{
    if (m_contextMenuOverride == 0)
    {
        m_contextMenuOption->setCurrentIndex(0);
    }
    else if (m_contextMenuOverride == RobloxMouseConfig::MOUSEBUTTON_RIGHT)
    {
        m_contextMenuOption->setCurrentIndex(1);
    }
    else if (m_contextMenuOverride & RobloxMouseConfig::MOUSEBUTTON_RIGHT &&
             m_contextMenuOverride & RobloxMouseConfig::MOUSEMODIFIER_SHIFT)
    {
        m_contextMenuOption->setCurrentIndex(2);
    }
    else if (m_contextMenuOverride & RobloxMouseConfig::MOUSEBUTTON_RIGHT &&
             m_contextMenuOverride & RobloxMouseConfig::MOUSEMODIFIER_ALT)
    {
        m_contextMenuOption->setCurrentIndex(3);
    }
    else if (m_contextMenuOverride & RobloxMouseConfig::MOUSEBUTTON_RIGHT &&
             m_contextMenuOverride & RobloxMouseConfig::MOUSEMODIFIER_CONTROL)
    {
        m_contextMenuOption->setCurrentIndex(4);
    }
}

void RobloxMouseConfigWidget::contextMenuOptionSelected(const QString& option)
{
    if (option == "Off")
    {
        m_contextMenuOverride = 0;
    }
    if (option == "Right Click")
    {
        m_contextMenuOverride = RobloxMouseConfig::MOUSEBUTTON_RIGHT;
    }
    else if (option == "Right Click + Shift")
    {
        m_contextMenuOverride = RobloxMouseConfig::MOUSEBUTTON_RIGHT |
                                RobloxMouseConfig::MOUSEMODIFIER_SHIFT;
    }
    else if (option == "Right Click + Alt")
    {
        m_contextMenuOverride = RobloxMouseConfig::MOUSEBUTTON_RIGHT |
                                RobloxMouseConfig::MOUSEMODIFIER_ALT;
    }
    else if (option == "Right Click + Ctrl")
    {
        m_contextMenuOverride = RobloxMouseConfig::MOUSEBUTTON_RIGHT |
                                RobloxMouseConfig::MOUSEMODIFIER_CONTROL;
    }
    
    Q_EMIT(dataChanged());
}

void RobloxMouseConfigWidget::accept()
{
    RobloxMouseConfig::singleton().setContextMenuConfig(m_contextMenuOverride);
    RobloxMouseConfig::singleton().saveMouseConfig();
}

void RobloxMouseConfigWidget::cancel()
{
    m_contextMenuOverride = RobloxMouseConfig::singleton().contextMenuConfig();
    updateContextMenuOption();
}

void RobloxMouseConfigWidget::restoreAllDefaults()
{
	m_contextMenuOverride = RobloxMouseConfig::MOUSEBUTTON_RIGHT;
    updateContextMenuOption();
    Q_EMIT(dataChanged());
}

