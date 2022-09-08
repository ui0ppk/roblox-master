/**
 * StudioIntellesenseFrame.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 * Created by Tyler Berg on 10/29/2013
 */

#include "stdafx.h"

// Qt Headers
#include <Qt>
#include <QWidget>
#include <QObject>
#include <QResource>
#include <QtCore>
#include <QDesktopWidget>

//Roblox Headers
#include "script/ScriptContext.h"
#include "util/UDim.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "Util/PhysicalProperties.h"

// Roblox Studio Headers
#include "StudioIntellesense.h"
#include "QtUtilities.h"
#include "RobloxIDEDoc.h"
#include "RobloxDocManager.h"
#include "AuthoringSettings.h"
#include "UpdateUIManager.h"
#include "RobloxMainWindow.h"
#include "RobloxScriptDoc.h"

FASTFLAG(GoogleAnalyticsTrackingEnabled)
FASTFLAG(StudioVariableIntellesense)

FASTFLAGVARIABLE(StudioIntellisenseCodeExecutionFixEnabled, true)
FASTFLAGVARIABLE(StudioIntellesenseShowNonDeprecatedMembers, true)
FASTFLAGVARIABLE(StudioIntellisenseAllowUnderscore, false)

#define OB_IMAGE_SIZE               16
#define OBJECT_BROWSER_IMAGES       ":/images/img_classtree.bmp"

#define MAX_CHILDREN                100

#define MAX_ITEMS_SHOWN             5
#define BASE_FRAME_WIDTH            250

#define AUTOCOMPLETE_FRAME_MARGIN   2
#define SCREEN_EDGE_MARGIN          6

namespace Studio {

    Intellesense::Intellesense()
     : m_focusProxy(NULL)
     , m_menuBelow(true)
     , m_cursorHeight(0)
    {
        m_AutoCompleteFrame = new QFrame();
        m_AutoCompleteFrame->resize(QSize(1,1));

        m_AutoCompleteFrame->setAttribute(Qt::WA_ShowWithoutActivating);
        m_AutoCompleteFrame->setFocusPolicy(Qt::NoFocus);
        m_AutoCompleteFrame->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::SubWindow | Qt::FramelessWindowHint);

        m_AutoCompleteFrame->show();
        m_AutoCompleteFrame->setVisible(false);

        m_AutoCompleteMenu = new QListWidget(m_AutoCompleteFrame);
        m_AutoCompleteMenu->setAttribute(Qt::WA_ShowWithoutActivating);
        m_AutoCompleteMenu->setFocusPolicy(Qt::NoFocus);
        m_AutoCompleteMenu->viewport()->setFocusPolicy(Qt::NoFocus);

        connect(m_AutoCompleteMenu, SIGNAL(itemClicked(QListWidgetItem*)),
                this,               SLOT(singleClick(QListWidgetItem*)));

        connect(m_AutoCompleteMenu, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
                this,               SLOT(doubleClick(QListWidgetItem*)));

        loadIntelleSenseMetadata();

        qApp->installEventFilter(this);
    }

    Intellesense& Intellesense::singleton()
    {
        static Intellesense* studioIntellesenseFrame = new Intellesense();
        return *studioIntellesenseFrame;
    }

    bool Intellesense::activate(QString &text, int row, int &cursorIndex, QKeyEvent* keyEvent,
		QRect positionRange, bool searchDatamodel, QWidget *focusWidget,
		const boost::optional<std::vector<RBX::ScriptAnalyzer::IntellesenseResult> >& result)
    {
        m_focusProxy = focusWidget;

        m_cursorHeight = positionRange.height();

        int key = keyEvent->key();
        Qt::KeyboardModifiers modifiers = keyEvent->modifiers();

        if (modifiers == Qt::ControlModifier)
            return false;

        if (key == Qt::Key_Period && modifiers != Qt::ShiftModifier)
        {
            text.insert(cursorIndex, '.');
            cursorIndex++;
            int itemFlags = populateItemLists(text, row, cursorIndex, searchDatamodel, result);
            handleAutoCompleteMenu(positionRange, itemFlags, getCurrentWord(text, cursorIndex));
            return false;
        }
        else if (key == Qt::Key_Colon || (key == Qt::Key_Semicolon && modifiers == Qt::ShiftModifier))
        {
            text.insert(cursorIndex, ':');
            cursorIndex++;
            populateItemLists(text, row, cursorIndex, searchDatamodel, result);
            handleAutoCompleteMenu(positionRange, FUNCTION_ITEM, getCurrentWord(text, cursorIndex));
            return false;
        }
        else if (((QChar)key).isLetter() || (FFlag::StudioVariableIntellesense && ((QChar)key).isNumber()) ||
			(FFlag::StudioIntellisenseAllowUnderscore && key == Qt::Key_Minus && modifiers == Qt::ShiftModifier))
        {
            QChar keyEventCharacter = (QChar)key; //Remove this with removal of FFlag::StudioIntellisenseAllowUnderscore
            if (modifiers != Qt::ShiftModifier)
                keyEventCharacter = keyEventCharacter.toLower();

            text.insert(cursorIndex, FFlag::StudioIntellisenseAllowUnderscore ? keyEvent->text() : keyEventCharacter);
            cursorIndex++;
            int itemFlags = populateItemLists(text, row, cursorIndex, searchDatamodel, result);
            handleAutoCompleteMenu(positionRange, itemFlags, getCurrentWord(text, cursorIndex));
            return false;
        }
        else if (key == Qt::Key_Backtab)
        {
            int itemFlags = populateItemLists(text, row, cursorIndex, searchDatamodel, result);
            return handleAutoCompleteMenu(positionRange, itemFlags, getCurrentWord(text, cursorIndex));
        }

        if (!isActive())
            return false;

        if (key == Qt::Key_Up)
        {
            //Move selection up
            if (m_AutoCompleteMenu->currentRow() > 0)
                m_AutoCompleteMenu->setCurrentRow(m_AutoCompleteMenu->currentRow() - 1);
            showToolTip();
            return true;
        }
        else if (key == Qt::Key_Down)
        {
            //Move selection down
            if (m_AutoCompleteMenu->currentRow() < m_AutoCompleteMenu->count() - 1)
                m_AutoCompleteMenu->setCurrentRow(qMin(m_AutoCompleteMenu->count() - 1, m_AutoCompleteMenu->currentRow() + 1));
            showToolTip();
            return true;
        }
        else if (key == Qt::Key_Tab || key == Qt::Key_Return)
        {
            replaceTextWithCurrentItem(text, cursorIndex);
            return true;
        }
        else if (key == Qt::Key_Backspace)
        {
            cursorIndex--;
            text = text.remove(cursorIndex, 1);
            positionRange.setX(positionRange.x() - 12);
            int itemFlags = populateItemLists(text, row, cursorIndex, searchDatamodel, result);
            handleAutoCompleteMenu(positionRange, itemFlags, getCurrentWord(text, cursorIndex));
            return false;
        }
        else if (key == Qt::Key_Shift || key == Qt::Key_Alt)
        {
            return false;
        }
        deactivate();
        return false;
    }

    void Intellesense::deactivate()
    {
        m_focusProxy = NULL;
        m_AutoCompleteFrame->setVisible(false);
        IntellesenseTooltip::singleton().hideText();
    }

    bool Intellesense::isActive()
    {
        return m_AutoCompleteFrame->isVisible();
    }

	void escapeSequence(QString& str)
	{
		int stringSize = str.size();

		while (stringSize--)
		{
			switch (str.at(stringSize).toAscii())
			{
			case '"': case '\\': case '\n': case '\r':
				str.insert(stringSize, "\\");
				break;
			case '\0':
				str.remove(stringSize, 1);
				str.insert(stringSize, "\\000");
				break;
			}
		}
	}

    bool Intellesense::replaceTextWithCurrentItem(QString& text, int& cursorIndex)
    {
        if (FFlag::GoogleAnalyticsTrackingEnabled)
            RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "IntellesenseTextReplace");

        QListWidgetItem* currentItem = m_AutoCompleteMenu->currentItem();

        if (!currentItem)
            return false;

        eItemType typeOfItem = (eItemType)(currentItem->data(TYPE_ROLE).toInt());
        
        if (typeOfItem == NO_ITEM)
        {
            int itemFlags = populateItemLists(text, cursorIndex, true, true, boost::none);
            handleAutoCompleteMenu(QRect(), itemFlags, "", m_AutoCompleteMenu->currentRow());
            return false;
        }

        deactivate();

        QString replacementText = currentItem->text();

		bool invalidFormat = false;

		if (replacementText.contains(QRegExp("[^a-zA-Z0-9_]")) || replacementText.contains(QRegExp("^[0-9]+$")))
			invalidFormat = true;

        if (typeOfItem == FUNCTION_ITEM)
            replacementText.append("()");

		if (invalidFormat)
		{
			escapeSequence(replacementText);
			replacementText = QString("[\"%1\"]").arg(replacementText);
		}

        //Replace
        int offset = cursorIndex - text.size() - 1;
        QString previousText = text.left(text.lastIndexOf(QRegExp(FFlag::StudioVariableIntellesense ? "[^a-zA-Z0-9_]" : "[\\.\\:]"), offset) + 1);
        QString postText = text.right(text.size() - cursorIndex);

		if (invalidFormat)
			previousText = previousText.left(previousText.size() - 1);

        text = previousText + replacementText + postText;

        cursorIndex = previousText.length() + replacementText.length();

        if (typeOfItem == FUNCTION_ITEM)
            cursorIndex--;

        return true;
    }

    void Intellesense::setStyleSheet(const QString& styleSheet)
    {
        m_AutoCompleteMenu->setStyleSheet(styleSheet);
        m_AutoCompleteFrame->setStyleSheet(styleSheet);
        IntellesenseTooltip::singleton().setStyleSheet(styleSheet);
        QWidget::setStyleSheet(styleSheet);
    }

    QString Intellesense::getPreviousWord(const QString& text, int index)
    {
        int pos = index + 1;
        QStringList stringList = text.split(QRegExp(FFlag::StudioIntellisenseAllowUnderscore ? "[^a-zA-Z\\d\\.\\:\\_]" : "[^a-zA-Z\\d\\.\\:]"));

        for (QStringList::iterator i = stringList.begin(); i != stringList.end(); ++i)
        {
            pos -= i->size() + 1;

            if (pos <= 0)
                return i->left(i->lastIndexOf(QRegExp("[\\.\\:]")));
        }
        return QString("");
    }

    QString Intellesense::getCurrentWord(const QString& text, int index)
    {
        int pos = index + 1;
        QStringList stringList = text.split(QRegExp(FFlag::StudioIntellisenseAllowUnderscore ? "[^a-zA-Z\\d\\_]" : "[^a-zA-Z\\d]"));

        for (QStringList::iterator i = stringList.begin(); i != stringList.end(); ++i)
        {
            pos -= i->size() + 1;

            if (pos <= 0)
                return *i;
        }
        return QString("");
    }

	bool Intellesense::isAfterDeclarationKeyword(const QString& text, int index)
	{
		QStringList stringList = text.split(QRegExp("\\s"));

		bool previousWordIsKeyword = false;
		int pos = 0;

		for (QStringList::iterator iter = stringList.begin(); iter != stringList.end(); ++iter)
		{
			QString token = *iter;
			pos += token.length();

			if (index <= pos)
				return previousWordIsKeyword;

			previousWordIsKeyword = (token == "local" || token == "function");

			++pos;
		}
		return false;
	}

    void Intellesense::loadIntelleSenseMetadata()
    {
        QResource metaDataResource(":/IntelleSenseMetadata.xml");
        QByteArray metaDataByteArray((const char*)metaDataResource.data(),(int)metaDataResource.size());

        metaDataByteArray = qUncompress(metaDataByteArray);

        m_intelleSenseMetadata = QDomDocument("IntelleSenseMetadata");
        m_intelleSenseMetadata.setContent(metaDataByteArray, false);
    }

    bool Intellesense::alphabeticalQListWidgetItemSort(QListWidgetItem i, QListWidgetItem j)
    {
        return i.text().compare(j.text()) < 0;
    }

    bool Intellesense::isEnumerator(QString previousWord)
    {
        if (previousWord == "Enum")
        {
            for (std::vector<const RBX::Reflection::EnumDescriptor*>::const_iterator i = RBX::Reflection::EnumDescriptor::enumsBegin(); 
                 i != RBX::Reflection::EnumDescriptor::enumsEnd();
                 ++i)
            {
				RBX::Reflection::Metadata::Item* metadata = RBX::Reflection::Metadata::Reflection::singleton()->get(**i);
				if (RBX::Reflection::Metadata::Item::isDeprecated(metadata, **i))
				{
					continue;
				}
                QListWidgetItem listItem = QListWidgetItem(getItemIcon(ENUM_ITEM), (*i)->name.c_str());
                listItem.setData(TYPE_ROLE, ENUM_ITEM);
                m_enumeratorList.push_back(listItem);
            }

            std::sort(m_enumeratorList.begin(), m_enumeratorList.end(), &Intellesense::alphabeticalQListWidgetItemSort);
            return true;
        }
        else if (previousWord.startsWith("Enum."))
        {
            for (std::vector<const RBX::Reflection::EnumDescriptor*>::const_iterator i = RBX::Reflection::EnumDescriptor::enumsBegin();
                 i != RBX::Reflection::EnumDescriptor::enumsEnd();
                 ++i)
            {
                if (previousWord.right(previousWord.length() - 5) == (*i)->name.c_str())
                {
                    std::for_each(
                        (*i)->begin(), (*i)->end(),
                        boost::bind(&Intellesense::populateEnumerator, this, _1));
                    return true;
                }
            }
        }
        return false;
    }

	void Intellesense::clearAllItemLists()
	{
		m_staticFunctionList.clear();
		m_enumeratorList.clear();
		m_propertyList.clear();
		m_functionList.clear();
		m_eventList.clear();
		m_callbackList.clear();
		m_childList.clear();
	}

    int Intellesense::populateItemLists(QString text, int row, int index, bool searchDatamodel, const boost::optional<std::vector<RBX::ScriptAnalyzer::IntellesenseResult> >& result)
    {
		if (FFlag::StudioVariableIntellesense)
			clearAllItemLists();

        QString previousWord = getPreviousWord(text, index);

		if (FFlag::StudioVariableIntellesense && isAfterDeclarationKeyword(text, index))
			return 0;

		bool gettingService = false;
		QRegExp service(FFlag::StudioIntellisenseCodeExecutionFixEnabled ? "([gG]ame:(?:Find|Get)Service\\([^)]+\\)[.a-zA-Z\\d]*)([.:][a-zA-Z\\d]*)$" : "([gG]ame:(?:Find|Get)Service\\(.+\\))[.:][a-zA-Z\\d]*$");
		gettingService = service.indexIn(text) > -1;
		if (gettingService) {
			previousWord = service.cap(1);
			searchDatamodel = true;
		}
		
        if (previousWord.size() < 1)
            return 0;
        
        //getting last intellesense triggering character
        QChar previousDivider = text.at(qMax(text.lastIndexOf(QRegExp("[^a-zA-Z\\d\\_]"), index - text.size() - 1), 0));
        if (previousDivider != '.' && previousDivider != ':')
		{
			if (FFlag::StudioVariableIntellesense && result)
				return repopulateUserVariables(result.get(), row, index);
			
			return 0;
		}
		
		if (!FFlag::StudioVariableIntellesense)
		{
			m_staticFunctionList.clear();
			m_enumeratorList.clear();
		}
        

        if (previousDivider == '.')
        {
            if (repopulateItemMetadata(previousWord, true, "LuaLibrary") || repopulateItemMetadata(previousWord, true))
                return FUNCTION_ITEM;
            else if (isEnumerator(previousWord))
                return ENUM_ITEM;
        }

		if (!FFlag::StudioVariableIntellesense)
		{
			m_propertyList.clear();
			m_functionList.clear();
			m_eventList.clear();
			m_callbackList.clear();
			m_childList.clear();
		}

		if (!gettingService) {
			if (!previousWord.at(0).isLetterOrNumber() || !previousWord.at(previousWord.size() - 1).isLetterOrNumber())
				return 0;
		}

		shared_ptr<RBX::LuaSourceContainer> script = shared_ptr<RBX::LuaSourceContainer>();
		if (!searchDatamodel && previousWord.startsWith("script", Qt::CaseSensitive))
		{
			script = getCurrentScript();
			searchDatamodel = true;
		}
		
		if (previousWord.startsWith("game", Qt::CaseInsensitive) || previousWord.startsWith("workspace", Qt::CaseInsensitive))
			searchDatamodel = true;

		if (!searchDatamodel)
			return 0;

		RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();

        if (playDoc && populateItemListsFromVariant(playDoc->evaluateCommandBarItem(previousWord.toStdString().c_str(), script), false))
        {
            if (previousDivider == '.')
                return PROPERTY_ITEM | EVENT_ITEM | CALLBACK_ITEM | CHILD_ITEM;
            else if (previousDivider == ':')
                return FUNCTION_ITEM;
        }

        return 0;
    }

	QString Intellesense::getCurrentScriptPath()
	{
		if (RobloxScriptDoc* scriptDoc = dynamic_cast<RobloxScriptDoc *>(RobloxDocManager::Instance().getCurrentDoc()))
		{
			shared_ptr<RBX::Instance> currentInstance = scriptDoc->getCurrentScript().toInstance();

			if (!currentInstance)
				return "";

			QString str = currentInstance->getName().c_str();

			while (currentInstance->getParent() && currentInstance->getParent()->getParent())
			{
				currentInstance = shared_from(currentInstance->getParent());

				if (!currentInstance)
					return "";

				str = str.prepend(QString(currentInstance->getName().c_str()).append("."));
			}
			str = str.prepend("game.");
			return str;
		}

		return "";
	}

	shared_ptr<RBX::LuaSourceContainer> Intellesense::getCurrentScript()
	{
		if (RobloxScriptDoc* scriptDoc = dynamic_cast<RobloxScriptDoc *>(RobloxDocManager::Instance().getCurrentDoc()))
		{
			return scriptDoc->getCurrentScript().toInstance();
		}
		return shared_ptr<RBX::LuaSourceContainer>();
	}

    template<bool includeType>
    static void catenateArg(int& count, QString& text, const RBX::Reflection::SignatureDescriptor::Item& item)
    {
        if (count>0)
            text += ", ";
        if (includeType)
        {
            text += item.type->name.c_str();
            text += " ";
        }

        text += item.name->c_str();

        if (includeType)
            if (item.hasDefaultValue() && item.defaultValue.isString())
            {
                // A default value has been defined
                text += " = ";
                text += item.defaultValue.get<std::string>().c_str();
            }
            ++count;
    }

    void Intellesense::addProperty(QString propertyName)
    {
        addProperty(propertyName, "");
    }

    void Intellesense::addProperty(QString propertyName, QString tooltip)
    {
        QListWidgetItem listItem = QListWidgetItem(getItemIcon(PROPERTY_ITEM), propertyName);

        listItem.setData(TYPE_ROLE, PROPERTY_ITEM);

        if (!tooltip.isNull())
            listItem.setToolTip(tooltip);

        m_propertyList.push_back(listItem);
    }

    void Intellesense::populateProperty(const RBX::Reflection::PropertyDescriptor* d)
    {
        if (!d || !d->isPublic() || !d->isScriptable())
            return;

        RBX::Reflection::Metadata::Item* metadata = RBX::Reflection::Metadata::Reflection::singleton()->get(*d);
        if (AuthoringSettings::singleton().isHiddenInBrowser(metadata, *d))
            return;

        QString toolTip = "";

        toolTip += d->type.name.toString().c_str();
        toolTip += " ";
        toolTip += d->name.toString().c_str();

        QListWidgetItem listItem = QListWidgetItem(getItemIcon(PROPERTY_ITEM), QString(d->name.c_str()));

        listItem.setData(TYPE_ROLE, PROPERTY_ITEM);

        if (!toolTip.isNull())
            listItem.setToolTip(toolTip);

        m_propertyList.push_back(listItem);
    }

    void Intellesense::populateFunction(const RBX::Reflection::FunctionDescriptor* d)
    {
        if (!d)
            return;

        RBX::Reflection::Metadata::Item* metadata = RBX::Reflection::Metadata::Reflection::singleton()->get(*d);

		// TODO - allow Get in browser, currently just adding a special case here.
        if (AuthoringSettings::singleton().isHiddenInBrowser(metadata, *d) && d->name.toString().compare("Get") != 0)
            return;

        QString toolTip = "";
        const RBX::Reflection::SignatureDescriptor& signature = d->getSignature();
        toolTip = signature.resultType->name.toString().c_str();
        toolTip += " ";
        toolTip += d->name.toString().c_str();
        toolTip += "(";
        int count = 0;
        std::for_each(
            signature.arguments.begin(), 
            signature.arguments.end(), 
            boost::bind(&catenateArg<true>, boost::ref(count), boost::ref(toolTip), _1)
            );
        toolTip += ")";

        if (metadata)
        {
            toolTip += "\n";
            toolTip += metadata->description.c_str();
        }

        QListWidgetItem listItem = QListWidgetItem(getItemIcon(FUNCTION_ITEM), QString(d->name.c_str()));

        listItem.setData(TYPE_ROLE, FUNCTION_ITEM);

        if (!toolTip.isNull())
            listItem.setToolTip(toolTip);

        m_functionList.push_back(listItem);
    }

    void Intellesense::populateYieldFunction(const RBX::Reflection::YieldFunctionDescriptor* d)
    {
        if (!d)
            return;

        RBX::Reflection::Metadata::Item* metadata = RBX::Reflection::Metadata::Reflection::singleton()->get(*d);

        if (AuthoringSettings::singleton().isHiddenInBrowser(metadata, *d))
            return;

        QString toolTip = "";
        const RBX::Reflection::SignatureDescriptor& signature = d->getSignature();
        toolTip = signature.resultType->name.toString().c_str();
        toolTip += " ";
        toolTip += d->name.toString().c_str();
        toolTip += "(";
        int count = 0;
        std::for_each(
            signature.arguments.begin(), 
            signature.arguments.end(), 
            boost::bind(&catenateArg<true>, boost::ref(count), boost::ref(toolTip), _1)
            );
        toolTip += ")";

        QListWidgetItem listItem = QListWidgetItem(getItemIcon(FUNCTION_ITEM), QString(d->name.c_str()));

        listItem.setData(TYPE_ROLE, FUNCTION_ITEM);

        if (!toolTip.isNull())
            listItem.setToolTip(toolTip);

        m_functionList.push_back(listItem);
    }

    void Intellesense::populateEvent(const RBX::Reflection::EventDescriptor* d)
    {
        if (!d || !d->isPublic() || !d->isScriptable())
            return;

        RBX::Reflection::Metadata::Item* metadata = RBX::Reflection::Metadata::Reflection::singleton()->get(*d);
        if (AuthoringSettings::singleton().isHiddenInBrowser(metadata, *d))
            return;

        const RBX::Reflection::SignatureDescriptor& signature = d->getSignature();

        QString toolTip = "event ";
        toolTip += d->name.toString().c_str();
        toolTip += "(";
        int count = 0;
        std::for_each(
            signature.arguments.begin(), 
            signature.arguments.end(), 
            boost::bind(&catenateArg<true>, boost::ref(count), boost::ref(toolTip), _1)
            );
        toolTip += ")";

        QListWidgetItem listItem = QListWidgetItem(getItemIcon(EVENT_ITEM), QString(d->name.c_str()));

        listItem.setData(TYPE_ROLE, EVENT_ITEM);

        if (!toolTip.isNull())
            listItem.setToolTip(toolTip);

        m_eventList.push_back(listItem);
    }

    void Intellesense::populateCallBack(const RBX::Reflection::CallbackDescriptor* d)
    {
        if (!d)
            return;

        RBX::Reflection::Metadata::Item* metadata = RBX::Reflection::Metadata::Reflection::singleton()->get(*d);
        if (AuthoringSettings::singleton().isHiddenInBrowser(metadata, *d))
            return;

        QListWidgetItem listItem = QListWidgetItem(getItemIcon(CALLBACK_ITEM), d->name.c_str());

        listItem.setData(TYPE_ROLE, CALLBACK_ITEM);

        listItem.setToolTip("callback");

        m_callbackList.push_back(listItem);
    }

    void Intellesense::populateEnumerator(const RBX::Reflection::EnumDescriptor::Item* d)
    {
		RBX::Reflection::Metadata::Item* metadata = RBX::Reflection::Metadata::Reflection::singleton()->get(*d);
		if (RBX::Reflection::Metadata::Item::isDeprecated(metadata, *d))
		{
			return;
		}
        QListWidgetItem listItem = QListWidgetItem(getItemIcon(ENUM_ITEM), d->name.c_str());
        listItem.setData(TYPE_ROLE, ENUM_ITEM);
        listItem.setToolTip(QString("%1: %2").arg(d->value).arg(d->name.c_str()));

        m_enumeratorList.push_back(listItem);
    }

    void Intellesense::showToolTip(QListWidgetItem* listItem)
    {
        if (!listItem)
            listItem = m_AutoCompleteMenu->currentItem();

        if (!listItem)
            return;

        QString tooltip = listItem->toolTip();

        if (!tooltip.isEmpty() && isActive())
        {
            tooltip.replace("%1", "</b><br><br>");
            tooltip.replace("\n", "</b><br><br>");
            tooltip.prepend("<html><b>");
            tooltip.append("</html>");

            QPoint bottomLeftAlt;
            QPoint topLeftAlt;
            if (m_menuBelow)
            {
                bottomLeftAlt = m_AutoCompleteFrame->pos() - QPoint(0, m_cursorHeight);
                topLeftAlt = m_AutoCompleteFrame->pos() + QPoint(0, m_AutoCompleteFrame->height() + AUTOCOMPLETE_FRAME_MARGIN);
            }
            else
            {
                bottomLeftAlt = m_AutoCompleteFrame->pos() - QPoint(0, AUTOCOMPLETE_FRAME_MARGIN);
                topLeftAlt = QPoint();
            }

            IntellesenseTooltip::singleton().showText(
                m_AutoCompleteFrame->pos() + QPoint(m_AutoCompleteFrame->width(), 0),
                tooltip,
                m_focusProxy,
                bottomLeftAlt,
                topLeftAlt
                );
        }
        else
        {
            IntellesenseTooltip::singleton().hideText();
        }
    }

    void Intellesense::singleClick(QListWidgetItem* listItem)
    {
        resetFocus();
        showToolTip();
    }

    void Intellesense::doubleClick(QListWidgetItem* listItem)
    {
        resetFocus();

        bool deactivateFrame = true;
        QListWidgetItem* currentItem = m_AutoCompleteMenu->currentItem();
        if (currentItem && (eItemType)(currentItem->data(TYPE_ROLE).toInt()) == NO_ITEM)
            deactivateFrame = false;

        Q_EMIT doubleClickSignal(listItem);

        if (deactivateFrame)
            deactivate();
    }

    void Intellesense::resetFocus()
    {
        if (isActive() && m_focusProxy)
        {
            m_focusProxy->activateWindow();
            m_focusProxy->setFocus(Qt::MouseFocusReason);
        }
    }

	int Intellesense::repopulateUserVariables(const std::vector<RBX::ScriptAnalyzer::IntellesenseResult>& result, unsigned int row, unsigned int column)
	{
		int itemTypes = NO_ITEM;

		for (auto it : result)
		{
			if (!it.name.empty())
			{
				bool isAfter = it.location.end.line < row || (it.location.end.line == row && it.location.end.column <= column);

				eItemType itemType = !it.isLocal || isAfter ? (it.isFunction ? FUNCTION_ITEM : PROPERTY_ITEM) : NO_ITEM;

				if (itemType != NO_ITEM)
				{
					QList<QListWidgetItem>* itemList = it.isFunction ? &m_functionList : &m_propertyList;

					QListWidgetItem listItem = QListWidgetItem(getItemIcon(itemType), it.name.c_str());
					listItem.setData(TYPE_ROLE, QVariant(itemType));
					itemList->push_front(listItem);
					itemTypes |= itemType;
				}
 			}

			if (it.children.size() > 0)
			{
				if (((it.location.begin.line < row) || 
					(it.location.begin.line == row && it.location.begin.column <= column)) &&
					(it.location.end.line > row || 
					(it.location.end.line == row && it.location.end.column >= column)))
				{
					itemTypes |= repopulateUserVariables(it.children, row, column);
				}
			}
		}
		return itemTypes;
	}

    bool Intellesense::repopulateItemMetadata(QString itemString, bool showStatic, QString parentTag)
    {
        QDomNodeList nodeList = m_intelleSenseMetadata.documentElement().elementsByTagName(parentTag.toStdString().c_str());

        for (int i = 0; i < nodeList.count(); ++i)
        {
            QDomElement domElement = nodeList.at(i).toElement();
            if (!domElement.attribute("name").compare(itemString))
            {
                for (QDomNode fNode = domElement.firstChild(); !fNode.isNull(); fNode = fNode.nextSibling())
                {
                    QDomElement metadataElement = fNode.toElement();

                    if (showStatic != metadataElement.tagName().startsWith("Static"))
                        continue;

                    QString elementName = metadataElement.attribute("name");
                    if (elementName.isNull())
                        continue;

                    // Set role
                    eItemType itemType = PROPERTY_ITEM;

                    if (metadataElement.tagName().contains("Function"))
                        itemType = FUNCTION_ITEM;

                    QListWidgetItem listItem = QListWidgetItem(getItemIcon(itemType), elementName);
                    listItem.setData(TYPE_ROLE, QVariant(itemType));

                    // Set tooltip
                    QString toolTip = metadataElement.attribute("tooltip");

                    if (!toolTip.isNull())
                        listItem.setToolTip(toolTip);

                    if (showStatic)
                        m_staticFunctionList.push_back(listItem);
                    else if (itemType == PROPERTY_ITEM)
                        m_propertyList.push_back(listItem);
                    else
                        m_functionList.push_back(listItem);
                }
                return true;
            }
        }
        return false;
    }

    bool Intellesense::populateItemListsFromVariant(const RBX::Reflection::Variant varValue, bool showAllChildren)
    {
        if(varValue.isType<boost::shared_ptr<RBX::Instance> >())
        {
            boost::shared_ptr<RBX::Instance> pInstance = varValue.get<boost::shared_ptr<RBX::Instance> >();

            std::for_each(
                pInstance->getDescriptor().begin<RBX::Reflection::PropertyDescriptor>(), 
                pInstance->getDescriptor().end<RBX::Reflection::PropertyDescriptor>(),
                boost::bind(&Intellesense::populateProperty, this, _1)
                );

            std::for_each(
                pInstance->getDescriptor().begin<RBX::Reflection::FunctionDescriptor>(), 
                pInstance->getDescriptor().end<RBX::Reflection::FunctionDescriptor>(),
                boost::bind(&Intellesense::populateFunction, this, _1)
                );

            std::for_each(
                pInstance->getDescriptor().begin<RBX::Reflection::YieldFunctionDescriptor>(), 
                pInstance->getDescriptor().end<RBX::Reflection::YieldFunctionDescriptor>(),
                boost::bind(&Intellesense::populateYieldFunction, this, _1)
                );

            std::for_each(
                pInstance->getDescriptor().begin<RBX::Reflection::EventDescriptor>(), 
                pInstance->getDescriptor().end<RBX::Reflection::EventDescriptor>(),
                boost::bind(&Intellesense::populateEvent, this, _1)
                );

            std::for_each(
                pInstance->getDescriptor().begin<RBX::Reflection::CallbackDescriptor>(), 
                pInstance->getDescriptor().end<RBX::Reflection::CallbackDescriptor>(),
                boost::bind(&Intellesense::populateCallBack, this, _1)
                );

            if (pInstance->getChildren())
            {
                RBX::Instances::const_iterator iter = pInstance->getChildren()->begin();
                for (int i = 0; iter != pInstance->getChildren()->end() && (i < MAX_CHILDREN || showAllChildren); ++iter, ++i)
                {
                    QListWidgetItem childItem = QListWidgetItem(getItemIcon(CHILD_ITEM), iter->get()->getName().c_str());
                    childItem.setData(TYPE_ROLE, QVariant(CHILD_ITEM));
                    childItem.setToolTip(QString("%1 %2").arg(iter->get()->getClassNameStr().c_str()).arg(iter->get()->getName().c_str()));
                    
                    m_childList.push_back(childItem);
                }
                if (iter != pInstance->getChildren()->end())
                {
                    int remaining = pInstance->numChildren() - MAX_CHILDREN;
                    QListWidgetItem remainingChildren = QListWidgetItem(QString("%1 more children...").arg(remaining));
                    remainingChildren.setData(TYPE_ROLE, QVariant(NO_ITEM));
                    m_childList.push_back(remainingChildren);
                }
            }
            return true;
        }
        else if (varValue.isType<RBX::Vector2>())
            return repopulateItemMetadata("Vector2");
        else if (varValue.isType<RBX::Vector2int16>())
            return repopulateItemMetadata("Vector2int16");
        else if (varValue.isType<RBX::Vector3>())
            return repopulateItemMetadata("Vector3");
        else if (varValue.isType<RBX::Vector3int16>())
            return repopulateItemMetadata("Vector3int16");
        else if (varValue.isType<G3D::CoordinateFrame>())
            return repopulateItemMetadata("CFrame");
        else if (varValue.isType<RBX::Region3>())
            return repopulateItemMetadata("Region3");
        else if (varValue.isType<RBX::Region3int16>())
            return repopulateItemMetadata("Region3int16");
        else if (varValue.isType<RBX::BrickColor>())
            return repopulateItemMetadata("BrickColor");
        else if (varValue.isType<RBX::Color3>())
            return repopulateItemMetadata("Color3");
        else if (varValue.isType<RBX::RbxRay>())
            return repopulateItemMetadata("Ray");
        else if (varValue.isType<RBX::UDim>())
            return repopulateItemMetadata("UDim");
        else if (varValue.isType<RBX::UDim2>())
            return repopulateItemMetadata("UDim2");
		else if (varValue.isType<RBX::Rect2D>())
			return repopulateItemMetadata("Rect");
		else if (varValue.isType<RBX::PhysicalProperties>())
			return repopulateItemMetadata("PhysicalProperties");
        else if (varValue.isType<shared_ptr<const RBX::Reflection::ValueTable> >())
        {
            shared_ptr<const RBX::Reflection::ValueTable> valMap = varValue.get<shared_ptr<const RBX::Reflection::ValueTable> >();

            for (RBX::Reflection::ValueTable::const_iterator iter = valMap->begin(); iter != valMap->end(); ++iter)
                addProperty(iter->first.c_str());

            return true;
        }
        else if (varValue.isType<shared_ptr<const RBX::Reflection::ValueArray> >())
        {
            //Can't access data through . operator in value array
        }
        return false;
    }

    void Intellesense::repopulateListWidgetHelper(QList<QListWidgetItem>& populateVector, QList<QListWidgetItem> *startsWithVector, int itemType, QString matchingWord)
    {
        int insertPos = 0;

        matchingWord = matchingWord.toLower();

        for (QList<QListWidgetItem>::iterator i = populateVector.end() - 1; i != populateVector.begin() - 1; --i)
        {
            QListWidgetItem* listItem = new QListWidgetItem(*i);
            QString itemString = listItem->text().toLower();

			if (FFlag::StudioIntellesenseShowNonDeprecatedMembers)
			{
				if (m_AutoCompleteMenu->findItems(listItem->text(), Qt::MatchExactly).size() > 0)
					continue;
			}
			else
			{
				if (m_AutoCompleteMenu->findItems(itemString, Qt::MatchFixedString).size() > 0)
					continue;
			}
			
            if (itemString.startsWith(matchingWord) && !matchingWord.isEmpty())
            {
				if (startsWithVector)
				{
					startsWithVector->append(*listItem);
				}
				else
				{
					m_AutoCompleteMenu->insertItem(0, listItem);
					insertPos++;
				}
            }
            else if (itemString.contains(matchingWord))
            {
                m_AutoCompleteMenu->insertItem(insertPos, listItem);
            }
        }
    }

    void Intellesense::repopulateListWidget(int itemFlags, QString currentWord, int lineIndex)
    {
        m_AutoCompleteMenu->clear();

        if (m_staticFunctionList.size() > 0)
        {
            itemFlags = 0;
            int insertPos = 0;

            currentWord = currentWord.toLower();

            for (QList<QListWidgetItem>::iterator i = m_staticFunctionList.end() - 1; i != m_staticFunctionList.begin() - 1; --i)
            {
                QListWidgetItem* listItem = new QListWidgetItem(*i);
                QString itemString = listItem->text().toLower();

                if (itemString.startsWith(currentWord))
                {
                    m_AutoCompleteMenu->insertItem(0, listItem);
                    insertPos++;
                }
                else if (itemString.contains(currentWord))
                {
                    m_AutoCompleteMenu->insertItem(insertPos, listItem);
                }
            }
        }

		QList<QListWidgetItem> *topOfList = new QList<QListWidgetItem>();

        if (m_enumeratorList.size() > 0)
        {
            itemFlags = 0;
            repopulateListWidgetHelper(m_enumeratorList, topOfList, ENUM_ITEM, currentWord);
        }

        if (itemFlags & CHILD_ITEM)
            repopulateListWidgetHelper(m_childList, topOfList, CHILD_ITEM, currentWord);

        if (itemFlags & CALLBACK_ITEM)
            repopulateListWidgetHelper(m_callbackList, topOfList, CALLBACK_ITEM, currentWord);

        if (itemFlags & EVENT_ITEM)
            repopulateListWidgetHelper(m_eventList, topOfList, EVENT_ITEM, currentWord);

        if (itemFlags & FUNCTION_ITEM)
            repopulateListWidgetHelper(m_functionList, topOfList, FUNCTION_ITEM, currentWord);

        if (itemFlags & PROPERTY_ITEM)
            repopulateListWidgetHelper(m_propertyList, topOfList, PROPERTY_ITEM, currentWord);

		if (topOfList->count() > 0)
		{
			std::sort(topOfList->begin(), topOfList->end(), &Intellesense::alphabeticalQListWidgetItemSort);
			repopulateListWidgetHelper(*topOfList, NULL, PROPERTY_ITEM, "");
		}

        if(m_AutoCompleteMenu->count() < 1 || (m_AutoCompleteMenu->count() == 1 && m_AutoCompleteMenu->item(0)->text() == currentWord))
        {
            deactivate();
            return;
        }

        m_AutoCompleteMenu->setCurrentRow(lineIndex);
    }

    bool Intellesense::handleAutoCompleteMenu(const QRect& positionRange, int itemFlags, QString currentWord, int lineIndex)
    {
        if (!itemFlags)
        {
            deactivate();
            return false;
        }

        repopulateListWidget(itemFlags, currentWord, lineIndex);

        if(m_AutoCompleteMenu->count() < 1 || (m_AutoCompleteMenu->count() == 1 && m_AutoCompleteMenu->item(0)->text() == currentWord))
        {
            deactivate();
            return false;
        }

        m_menuBelow = true;
        
        int itemheight = m_AutoCompleteMenu->visualItemRect(m_AutoCompleteMenu->item(0)).height();

        int menuHeight = (qMin(m_AutoCompleteMenu->count(), MAX_ITEMS_SHOWN) * itemheight) + AUTOCOMPLETE_FRAME_MARGIN;
        
#ifdef _WIN32
        menuHeight += AUTOCOMPLETE_FRAME_MARGIN;
#endif

        if(m_AutoCompleteMenu->count() < MAX_ITEMS_SHOWN)
            m_AutoCompleteMenu->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        else
            m_AutoCompleteMenu->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        
        m_AutoCompleteMenu->resize(BASE_FRAME_WIDTH, menuHeight);

        int xPos = m_AutoCompleteFrame->pos().x();
        int yPos = m_AutoCompleteFrame->pos().y();

        if (!positionRange.isNull())
        {
            xPos = positionRange.x();
            yPos = positionRange.y();

            QRect screenBounds = QApplication::desktop()->screenGeometry(m_focusProxy);

            yPos += positionRange.height();

            if (yPos + menuHeight > screenBounds.y() + screenBounds.height())
            {
                yPos -= menuHeight + positionRange.height();
                m_menuBelow = false;
            }
			
            if (xPos + BASE_FRAME_WIDTH > screenBounds.x() + screenBounds.width())
                xPos = screenBounds.x() + screenBounds.width() - SCREEN_EDGE_MARGIN - BASE_FRAME_WIDTH;

            
        }
        m_AutoCompleteFrame->setGeometry(xPos, yPos, BASE_FRAME_WIDTH, menuHeight);

        m_AutoCompleteFrame->setVisible(true);

        showToolTip();
        return true;
    }

    bool Intellesense::eventFilter(QObject *watched, QEvent *evt)
    {
        if (isActive())
        {
            if (evt->type() == QEvent::MouseButtonPress)
            {
                m_AutoCompleteMenu->viewport()->repaint();
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(evt);
                if (mouseEvent && !m_AutoCompleteFrame->geometry().contains(mouseEvent->globalPos()))
                    deactivate();
            }
            else if (evt->type() == QEvent::KeyPress ||
                     evt->type() == QEvent::MouseMove ||
                     evt->type() == QEvent::Wheel)
                m_AutoCompleteMenu->viewport()->repaint();//bug with qt does not repaint qlistwidget on mac correctly
            else if (evt->type() == QEvent::ApplicationDeactivate)
                deactivate();
        }
        return false;
    }

    QIcon Intellesense::getItemIcon(eItemType itemType)
    {
        int position = 0;
        switch(itemType)
        {
            case CHILD_ITEM:
                position = 3;
                break;
            case FUNCTION_ITEM:
                position = 4;
                break;
            case PROPERTY_ITEM:
                position = 6;
                break;
            case EVENT_ITEM:
                position = 8;
                break;
            case ENUM_ITEM:
                position = 9;
                break;
            case CALLBACK_ITEM:
                position = 16;
                break;
            default:
                break;
        }
        return QIcon(QtUtilities::getPixmap(OBJECT_BROWSER_IMAGES, position, OB_IMAGE_SIZE, true));
    }

    IntellesenseTooltip::IntellesenseTooltip()
    {
        resize(QSize(1,1));

        setAttribute(Qt::WA_ShowWithoutActivating);
        setFocusPolicy(Qt::NoFocus);
        setWindowFlags(Qt::WindowStaysOnTopHint | Qt::ToolTip | Qt::FramelessWindowHint);

        show();
        setVisible(false);
        resize(200, 200);

        toolTipLabel = new QLabel(this);
        toolTipLabel->setGeometry(4, 4, width() - SCREEN_EDGE_MARGIN, 20);
        toolTipLabel->setOpenExternalLinks(true);
        toolTipLabel->setWordWrap(true);
        toolTipLabel->setAlignment(Qt::AlignTop);
        toolTipLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    }

    IntellesenseTooltip& IntellesenseTooltip::singleton()
    {
        static IntellesenseTooltip* studioIntellesenseTooltip = new IntellesenseTooltip();
        return *studioIntellesenseTooltip;
    }

    void IntellesenseTooltip::showText(const QPoint& pos, const QString& text, QWidget* focusWidget, const QPoint& bottomLeftAlt, const QPoint& topLeftAlt)//, int altX, const QPoint& altPos)
    {
        int breakPos = text.indexOf("</b><br><br>");

        QString header = text.left(breakPos);
        QString body = text.mid(breakPos);

        header = header.remove(QRegExp("<[^>]*>"));
        body = body.remove(QRegExp("<[^>]*>"));

        QFont boldFont = toolTipLabel->font();
        boldFont.setBold(true);
        QFontMetrics fontMetric = QFontMetrics(boldFont);

        int toolTipWidth = fontMetric.width(header);
        int toolTipHeight = fontMetric.height() * (qCeil((float)fontMetric.width(body) / (float)toolTipWidth) + AUTOCOMPLETE_FRAME_MARGIN);

        if (breakPos == -1)
        {
             toolTipLabel->setText(header);
             toolTipHeight = fontMetric.height();
        }
        else
            toolTipLabel->setText(text);

        toolTipLabel->resize(toolTipWidth, toolTipHeight);
        toolTipLabel->adjustSize();

        QPoint finalPosition = QPoint(pos.x() + AUTOCOMPLETE_FRAME_MARGIN, pos.y() + AUTOCOMPLETE_FRAME_MARGIN);

        QRect screenBounds;
        if (focusWidget)
            screenBounds = QApplication::desktop()->screenGeometry(focusWidget);
        else
            screenBounds = QApplication::desktop()->screenGeometry();

        // Snapping to screen bounds using alternative positions
        if (finalPosition.x() + toolTipLabel->width() + SCREEN_EDGE_MARGIN > screenBounds.x() + screenBounds.width())
        {
            if (!topLeftAlt.isNull() && topLeftAlt.y() + toolTipLabel->height() + SCREEN_EDGE_MARGIN <= screenBounds.y() + screenBounds.height())
                finalPosition = topLeftAlt;
            else if (bottomLeftAlt.y() - toolTipLabel->height() - SCREEN_EDGE_MARGIN > AUTOCOMPLETE_FRAME_MARGIN)
                finalPosition = QPoint(bottomLeftAlt.x(), bottomLeftAlt.y() - toolTipLabel->height() - SCREEN_EDGE_MARGIN);

            if (finalPosition.x() < 0)
                finalPosition.setX(AUTOCOMPLETE_FRAME_MARGIN);

            if (finalPosition.x() + toolTipLabel->width() + SCREEN_EDGE_MARGIN > screenBounds.x() + screenBounds.width())
                finalPosition.setX(screenBounds.x() + screenBounds.width() - toolTipLabel->width() - SCREEN_EDGE_MARGIN - AUTOCOMPLETE_FRAME_MARGIN);
        }
        else
        {
            if (finalPosition.y() < 0)
                finalPosition.setY(AUTOCOMPLETE_FRAME_MARGIN);

            if (finalPosition.y() + toolTipLabel->height() + SCREEN_EDGE_MARGIN > screenBounds.y() + screenBounds.height())
                finalPosition.setY(screenBounds.y() + screenBounds.height() - toolTipLabel->height() - SCREEN_EDGE_MARGIN - AUTOCOMPLETE_FRAME_MARGIN);
        }

        setGeometry(finalPosition.x(), finalPosition.y(), toolTipLabel->width() + SCREEN_EDGE_MARGIN, toolTipLabel->height() + SCREEN_EDGE_MARGIN);
        setVisible(true);
    }

    void IntellesenseTooltip::hideText()
    {
        setVisible(false);
    }

    void IntellesenseTooltip::setStyleSheet(const QString& styleSheet)
    {
        QFrame::setStyleSheet(styleSheet);
    }
} //namespace
