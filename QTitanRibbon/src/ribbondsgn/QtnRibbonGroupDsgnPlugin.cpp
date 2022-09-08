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

#include <QtDesigner/QExtensionFactory>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerPropertySheetExtension>
#include <QtDesigner/QExtensionManager>

#include "QtnRibbonGroupDsgnPlugin.h"
#include "QtnRibbonBar.h"
#include "QtnRibbonGroup.h"
#include "../src/ribbon/QtnRibbonTabBar.h"


using namespace Qtitan;

/*
static void setObjectNameChildren(RibbonTabBar& widget)
{
    QList<Qtitan::RibbonTab*> child_list = widget.findChildren<Qtitan::RibbonTab*>();
    for(int i = 0, count = child_list.count(); count > i; i++)
    {
        if (Qtitan::RibbonTab* pWd = child_list[i])
            pWd->setObjectName("__qt__passive_RibbonTab");
    }
}
*/

/* DsgnRibbonGroupPlugin */
DsgnRibbonGroupPlugin::DsgnRibbonGroupPlugin(QObject *parent)
    : QObject(parent), m_core(0)
{
    initialized = false;
}

void DsgnRibbonGroupPlugin::initialize(QDesignerFormEditorInterface* formEditor)
{
    if (initialized)
        return;

    initialized = true;
    m_core = formEditor;

    QExtensionManager* manager = formEditor->extensionManager();
    Q_ASSERT(manager != Q_NULL);
    manager->registerExtensions(new DsgnRibbonPageContainerFactory(manager), Q_TYPEID(QDesignerContainerExtension));
}

bool DsgnRibbonGroupPlugin::isInitialized() const
{
    return initialized;
}

QWidget* DsgnRibbonGroupPlugin::createWidget(QWidget* parent)
{
    //if (RibbonPage* ribbonPage = qobject_cast<RibbonPage *>(parent))
    //{
    //    Qtitan::RibbonGroup* result = ribbonPage->addGroup("");
    //    connect(result, SIGNAL(titleChanged(QString)), this, SLOT(titleChanged(QString)));
    //    result->setObjectName("__qt__passive_RibbonGroup");
    //    Q_ASSERT(QDesignerFormWindowInterface::findFormWindow(result) != 0);
    //    QDesignerFormWindowInterface::findFormWindow(ribbonPage)->manageWidget(result);
    //    result->setAcceptDrops(true);
    //    return result;
    //}
    //Qtitan::RibbonGroup* result = new Qtitan::RibbonGroup(parent, "");
    //result->setAcceptDrops(true);
    //return result;

    Qtitan::RibbonGroup* result = new Qtitan::RibbonGroup(parent, "");
    connect(result, SIGNAL(titleChanged(QString)), this, SLOT(titleChanged(QString)));
    result->setObjectName("__qt__passive_RibbonGroup");
    result->setAcceptDrops(true);
    return result;

}

QString DsgnRibbonGroupPlugin::name() const
{
    return "Qtitan::RibbonGroup";
}

QString DsgnRibbonGroupPlugin::group() const
{
    return "Developer Machines: Components";
}

QIcon DsgnRibbonGroupPlugin::icon() const
{
    return QIcon(":res/ribbon_icon.png");
}

QString DsgnRibbonGroupPlugin::toolTip() const
{
    return "Advanced RibbonPage for Qt++";
}

QString DsgnRibbonGroupPlugin::whatsThis() const
{
    return "Advanced RibbonPage for Qt++";
}

bool DsgnRibbonGroupPlugin::isContainer() const
{
    return false;
}

QString DsgnRibbonGroupPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
        " <widget class=\"Qtitan::RibbonGroup\" name=\"ribbonGroup\"/>\n"
        " <customwidgets>\n"
        "   <customwidget>\n"
        "       <class>Qtitan::RibbonGroup</class>\n"
        "       <extends>QWidget</extends>\n"
        "   </customwidget>\n"
        " </customwidgets>\n"
        "</ui>\n";
}

QString DsgnRibbonGroupPlugin::includeFile() const
{
    return "QtnRibbonGroup.h";
}

void DsgnRibbonGroupPlugin::titleChanged(const QString& title)
{
    Q_UNUSED(title);
    if (RibbonGroup* ribbonGroup = qobject_cast<RibbonGroup*>(sender())) 
    {
        if (QDesignerFormWindowInterface* form = QDesignerFormWindowInterface::findFormWindow(ribbonGroup))
        {
            QDesignerFormEditorInterface* editor = form->core();
            QExtensionManager* manager = editor->extensionManager();
            QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(manager, ribbonGroup);
            const int propertyIndex = sheet->indexOf(QLatin1String("title"));
            sheet->setChanged(propertyIndex, true);
        }
    }
}

/* DsgnRibbonPageContainer */
DsgnRibbonPageContainer::DsgnRibbonPageContainer(RibbonPage* widget, QObject* parent)
    : QObject(parent), m_ribbonPage(widget)
{
}

void DsgnRibbonPageContainer::addWidget(QWidget *widget)
{
    Q_ASSERT(m_ribbonPage != Q_NULL);
    RibbonGroup* group = qobject_cast<RibbonGroup *>(widget);
    if (group)
    {
        if (group->title().isEmpty())
        {
            QString title = tr("Group %1").arg(count());
            group->setTitle(title);
        }

        group->setAutoFillBackground(false);
        m_ribbonPage->addGroup(group);
        Q_ASSERT(QDesignerFormWindowInterface::findFormWindow(m_ribbonPage) != 0);
        QDesignerFormWindowInterface::findFormWindow(m_ribbonPage)->manageWidget(group);
    }
}

int DsgnRibbonPageContainer::count() const
{
    Q_ASSERT(m_ribbonPage != Q_NULL);
    return m_ribbonPage->groupCount();
}

int DsgnRibbonPageContainer::currentIndex() const
{
    return 0;
}

void DsgnRibbonPageContainer::insertWidget(int index, QWidget* widget)
{
    Q_UNUSED(index);
    Q_UNUSED(widget);
}

void DsgnRibbonPageContainer::remove(int index)
{
    Q_UNUSED(index);
}

void DsgnRibbonPageContainer::setCurrentIndex(int index)
{
    Q_UNUSED(index);
}

QWidget* DsgnRibbonPageContainer::widget(int index) const
{
    Q_ASSERT(m_ribbonPage != Q_NULL);
    return m_ribbonPage->getGroup(index);
}


/* DsgnRibbonPageContainerFactory */
DsgnRibbonPageContainerFactory::DsgnRibbonPageContainerFactory(QExtensionManager* parent)
    : QExtensionFactory(parent)
{
}

QObject* DsgnRibbonPageContainerFactory::createExtension(QObject* object, const QString& iid, QObject* parent) const
{
    if (iid != Q_TYPEID(QDesignerContainerExtension))
        return 0;

    if (RibbonPage* w = qobject_cast<RibbonPage*>(object))
        return new DsgnRibbonPageContainer(w, parent);

    return 0;
}
