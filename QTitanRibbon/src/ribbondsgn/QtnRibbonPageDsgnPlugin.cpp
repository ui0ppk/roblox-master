/****************************************************************************
**
** Qtitan Library by Developer Machines (Microsoft-Ribbon implementation for Qt.C++)
** 
** Copyright (c) 2009-2013 Developer Machines (http://www.devmachines.com)
**           ALL RIGHTS RESERVED
** 
**  The entire contents of this file is protected by copyright law and
**  international treaties. Unauthorized reproduction, reverse-engineering
**  and distribution of all or any portion of the code contained in this
**  file is strictly prohibited and may result in severe civil and 
**  criminal penalties and will be prosecuted to the maximum extent 
**  possible under the law.
**
**  RESTRICTIONS
**
**  THE SOURCE CODE CONTAINED WITHIN THIS FILE AND ALL RELATED
**  FILES OR ANY PORTION OF ITS CONTENTS SHALL AT NO TIME BE
**  COPIED, TRANSFERRED, SOLD, DISTRIBUTED, OR OTHERWISE MADE
**  AVAILABLE TO OTHER INDIVIDUALS WITHOUT WRITTEN CONSENT
**  AND PERMISSION FROM DEVELOPER MACHINES
**
**  CONSULT THE END USER LICENSE AGREEMENT FOR INFORMATION ON
**  ADDITIONAL RESTRICTIONS.
**
****************************************************************************/
#include <QStyle>
#include <QContextMenuEvent>
#include <QtDesigner/QExtensionFactory>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerPropertySheetExtension>
#include <QtDesigner/QExtensionManager>

#include "QtnRibbonPageDsgnPlugin.h"
#include "QtnRibbonBar.h"
#include "../src/ribbon/QtnRibbonTabBar.h"


using namespace Qtitan;

/* RibbonPageEventFilterDsgn */
RibbonPageEventFilterDsgn::RibbonPageEventFilterDsgn(RibbonPage* page)
    : QObject(page)
    , m_pRibbonPage(page)
{
    m_addRibbonGroup = new QAction(tr("Insert Group"), this);
    connect(m_addRibbonGroup, SIGNAL(triggered()), this, SLOT(slotAddNewRibbonGroup()));

    m_deleteRibbonGroup = new QAction(tr("Delete current group"), this);
    connect(m_deleteRibbonGroup, SIGNAL(triggered()), this, SLOT(slotRemoveCurrentPage()));
}

RibbonPageEventFilterDsgn::~RibbonPageEventFilterDsgn()
{
}

bool RibbonPageEventFilterDsgn::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != m_pRibbonPage)
        return QObject::eventFilter(watched, event);

    switch (event->type()) 
    {
        case QEvent::ContextMenu:
            return handleContextMenuEvent(static_cast<QContextMenuEvent*>(event));
        default:
            break;
    }

    return QObject::eventFilter(watched, event);
}

bool RibbonPageEventFilterDsgn::handleContextMenuEvent(QContextMenuEvent* event)
{
    event->accept();
    m_pRibbonPage->update();
    QMenu menu;
    menu.addAction(m_addRibbonGroup);
    if (((RibbonPage*)m_pRibbonPage)->groupCount() > 0)
        menu.addAction(m_deleteRibbonGroup);
    menu.exec(event->globalPos());
    return true;
}

void RibbonPageEventFilterDsgn::slotAddNewRibbonGroup()
{
    Q_ASSERT(formWindow() != 0);

    QDesignerFormWindowInterface* fw = formWindow();
    Q_UNUSED(fw);

    //AddRibbonPageCommand* cmd = new AddRibbonPageCommand(fw);
    //cmd->init(qobject_cast<RibbonBar*>(m_pRibbonBar));
    //cmd->redo();
//    fw->commandHistory()->push(cmd);
}

void RibbonPageEventFilterDsgn::slotRemoveCurrentPage()
{
}

QDesignerFormWindowInterface* RibbonPageEventFilterDsgn::formWindow() const
{
    return QDesignerFormWindowInterface::findFormWindow(m_pRibbonPage);
}


/* DsgnRibbonPagePlugin */
DsgnRibbonPagePlugin::DsgnRibbonPagePlugin(QObject *parent)
    : QObject(parent), m_core(0)
{
    initialized = false;
}

void DsgnRibbonPagePlugin::initialize(QDesignerFormEditorInterface* core)
{
    if (initialized)
        return;

    initialized = true;
    m_core = core;
}

bool DsgnRibbonPagePlugin::isInitialized() const
{
    return initialized;
}

QWidget* DsgnRibbonPagePlugin::createWidget(QWidget* parent)
{
    RibbonPage* result = new RibbonPage(parent, "");
    connect(result, SIGNAL(titleChanged(QString)), this, SLOT(titleChanged(QString)));
    result->setObjectName("__qt__passive_RibbonPage");
    RibbonPageEventFilterDsgn* filter = new RibbonPageEventFilterDsgn(result);
    result->installEventFilter(filter);
    result->setAcceptDrops(true);
    return result;
}

QString DsgnRibbonPagePlugin::name() const
{
    return "Qtitan::RibbonPage";
}

QString DsgnRibbonPagePlugin::group() const
{
    return "Developer Machines: Components";
}

QIcon DsgnRibbonPagePlugin::icon() const
{
    return QIcon(":res/ribbon_icon.png");
}

QString DsgnRibbonPagePlugin::toolTip() const
{
    return "Advanced RibbonPage for Nokia-Qt";
}

QString DsgnRibbonPagePlugin::whatsThis() const
{
    return "Advanced RibbonPage for Nokia-Qt";
}

bool DsgnRibbonPagePlugin::isContainer() const
{
    return false;
}

QString DsgnRibbonPagePlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
        " <widget class=\"Qtitan::RibbonPage\" name=\"ribbonPage\"/>\n"
        " <customwidgets>\n"
        "   <customwidget>\n"
        "       <class>Qtitan::RibbonPage</class>\n"
        "       <extends>QWidget</extends>\n"
        "       <addpagemethod>addGroup</addpagemethod>\n"
        "   </customwidget>\n"
        " </customwidgets>\n"
        "</ui>\n";
}

QString DsgnRibbonPagePlugin::includeFile() const
{
    return "QtnRibbonPage.h";
}

void DsgnRibbonPagePlugin::titleChanged(const QString& title)
{
    Q_UNUSED(title);
    if (RibbonPage* ribbonPage = qobject_cast<RibbonPage*>(sender())) 
    {
        if (QDesignerFormWindowInterface* form = QDesignerFormWindowInterface::findFormWindow(ribbonPage))
        {
            QDesignerFormEditorInterface* editor = form->core();
            QExtensionManager* manager = editor->extensionManager();
            QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(manager, ribbonPage);
            const int propertyIndex = sheet->indexOf(QLatin1String("title"));
            sheet->setChanged(propertyIndex, true);
        }
    }
}




