/**
 * StudioIntellesenseFrame.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 * Created by Tyler Berg on 10/29/2013
 */

#pragma once

// Qt Headers
#include <Qt>
#include <QWidget>
#include <QObject>
#include <QDomDocument>
#include <QRect>
#include <QListWidgetItem>
#include <QIcon>
#include <QFrame>
#include <QListWidget>
#include <QLabel>

// Roblox Headers
#include "reflection/EnumConverter.h"
#include "Util/BrickColor.h"
#include "reflection/type.h"
#include "script/ScriptAnalyzer.h"

namespace RBX
{
    class BrickColor;
    class LuaSourceContainer;
    namespace Reflection
    {
        class Descriptor;
        class PropertyDescriptor;
        class FunctionDescriptor;
        class YieldFunctionDescriptor;
        class EventDescriptor;
        class CallbackDescriptor;
        class EnumDescriptor;
    }
}

namespace Studio {
    class Intellesense : public QWidget
    {
        Q_OBJECT

    public:
        Intellesense();
        static Intellesense& singleton();

        bool activate(QString &text, int row, int &cursorIndex, QKeyEvent* keyEvent,
			QRect positionRange, bool searchDatamodel, QWidget *focusWidget,
			const boost::optional<std::vector<RBX::ScriptAnalyzer::IntellesenseResult> >& result);
        void deactivate();

        bool isActive();

        bool replaceTextWithCurrentItem(QString& text, int& cursorIndex);

        virtual void setStyleSheet(const QString& styleSheet);

    Q_SIGNALS:
        void doubleClickSignal(QListWidgetItem* listItem);

    protected Q_SLOTS:
        void doubleClick(QListWidgetItem* listItem);
        void singleClick(QListWidgetItem* listItem);

        void showToolTip(QListWidgetItem* listItem = NULL);

    private:

        //TODO: Add TOOLTIP_ROLE to handle polymorphic functions in the tooltip
        enum eItemRole
        {
            TYPE_ROLE      = Qt::UserRole + 1
        };

        enum eItemType
        {
            NO_ITEM       = 0x00,
            FUNCTION_ITEM = 0x01,
            PROPERTY_ITEM = 0x02,
            EVENT_ITEM    = 0x04,
            ENUM_ITEM     = 0x08,
            CALLBACK_ITEM = 0x10,
            CHILD_ITEM    = 0x20
        };

        void resetFocus();

        QIcon getItemIcon(eItemType itemType);
		
        QString getPreviousWord(const QString& text, int index);
        QString getCurrentWord(const QString& text, int index);

		bool isAfterDeclarationKeyword(const QString& text, int index);

        void loadIntelleSenseMetadata();

		int repopulateUserVariables(const std::vector<RBX::ScriptAnalyzer::IntellesenseResult>& result, unsigned int row, unsigned int column);

        bool repopulateItemMetadata(QString itemString, bool showStatic = false, QString parentTag = "ItemStruct");

		void clearAllItemLists();

        static bool alphabeticalQListWidgetItemSort(QListWidgetItem i, QListWidgetItem j);
        bool isEnumerator(QString previous);

        void addProperty(QString propertyName);
        void addProperty(QString propertyName, QString tooltip);

        void populateProperty(const RBX::Reflection::PropertyDescriptor* d);
        void populateFunction(const RBX::Reflection::FunctionDescriptor* d);
        void populateYieldFunction(const RBX::Reflection::YieldFunctionDescriptor* d);
        void populateEvent(const RBX::Reflection::EventDescriptor* d);
        void populateCallBack(const RBX::Reflection::CallbackDescriptor* d);
        void populateEnumerator(const RBX::Reflection::EnumDescriptor::Item* d);

        int populateItemLists(QString text, int row, int index, bool searchDatamodel, const boost::optional<std::vector<RBX::ScriptAnalyzer::IntellesenseResult> >& result);
        bool populateItemListsFromVariant(const RBX::Reflection::Variant varValue, bool showAllChildren = false);

        void repopulateListWidget(int itemFlags, QString currentWord, int lineIndex = 0);
        void repopulateListWidgetHelper(QList<QListWidgetItem>& populateVector, QList<QListWidgetItem> *startsWithVector, int itemType, QString matchingWord);

        bool handleAutoCompleteMenu(const QRect& positionRange, int itemFlags, QString currentWord, int lineIndex = 0);

		QString getCurrentScriptPath();
		shared_ptr<RBX::LuaSourceContainer> getCurrentScript();

        virtual bool eventFilter(QObject *watched, QEvent *evt);

        QListWidget*            m_AutoCompleteMenu;
        QFrame*                 m_AutoCompleteFrame;

        bool                    m_menuBelow;
        int                     m_cursorHeight;

        QList<QListWidgetItem>  m_staticFunctionList;
        QList<QListWidgetItem>  m_propertyList;
        QList<QListWidgetItem>  m_functionList;
        QList<QListWidgetItem>  m_eventList;
        QList<QListWidgetItem>  m_callbackList;
        QList<QListWidgetItem>  m_enumeratorList;
        QList<QListWidgetItem>  m_childList;

        QDomDocument            m_intelleSenseMetadata;

        QWidget*                m_focusProxy;
    };

    class IntellesenseTooltip : QFrame
    {
        Q_OBJECT

    public:
        IntellesenseTooltip();
        static IntellesenseTooltip& singleton();
        
        void showText(const QPoint& pos, const QString& text, QWidget* focusWidget, const QPoint& bottomLeftAlt, const QPoint& topLeftAlt);
        void hideText();

        virtual void setStyleSheet(const QString& styleSheet);

    private:

        QLabel*                 toolTipLabel;

    };
} // namespace
