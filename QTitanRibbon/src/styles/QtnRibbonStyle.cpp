/****************************************************************************
**
** Qtitan Library by Developer Machines (Microsoft-Ribbon implementation for Qt.C++)
** 
** Copyright (c) 2009-2014 Developer Machines (http://www.devmachines.com)
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
#include <QToolTip>
#include <QPaintEngine>
#include <QApplication>
#include <QCoreApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QLabel>
#include <QTabBar>
#include <QBitmap>
#include <QMdiArea>
#include <QStackedWidget>
#include <qevent.h>
#include <QLibrary>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <qdrawutil.h>
#endif /* Q_OS_WIN*/

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif /* Q_OS_WIN*/

#include "../src/ribbon/QtnRibbonButton.h"
#include "../src/ribbon/QtnRibbonTabBar.h"
#include "../src/ribbon/QtnRibbonPrivate.h"
#include "../src/ribbon/QtnOfficeFrameHelper.h"
#include "../src/ribbon/QtnRibbonBarPrivate.h"
#include "../src/ribbon/QtnRibbonBackstageView.h"
#include "../src/ribbon/QtnRibbonGroupPrivate.h"

#include "QtnRibbonStyle.h"
#include "QtnRibbonStylePrivate.h"
#include "QtnRibbonStatusBar.h"
#include "QtnStyleHelpers.h"
#include "QtnCommonStylePrivate.h"
#include "QtitanRibbon.h"
#include "QtnRibbonQuickAccessBar.h"
#include "QtnRibbonGallery.h"

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wswitch"
#endif

using namespace Qtitan;


/*!
    \enum Qtitan::RibbonStyle::QPixelMetric

    This enum describes the various available pixel metrics of Qtitan RibbonStyle. A pixel
    metric is a style dependent size represented by a single pixel
    value.
    \value PM_RibbonReducedGroupWidth
    \value PM_RibbonHeightGroup
    \value PM_RibbonHeightCaptionGroup
    \value PM_RibbonTabsHeight
*/

/*!
    \enum Qtitan::RibbonStyle::QPrimitiveElement

    This enum describes the various primitive elements of Qtitan RibbonStyle. A
    primitive element is a common GUI element, such as a checkbox
    indicator or button bevel.

    \value PE_RibbonPopupBarButton
    \value PE_RibbonFileButton
    \value PE_RibbonOptionButton
    \value PE_RibbonGroupScrollButton
    \value PE_RibbonSliderButton
    \value PE_RibbonTab
    \value PE_RibbonContextHeaders
    \value PE_RibbonQuickAccessButton
    \value PE_RibbonFillRect
    \value PE_RibbonRect
    \value PE_RibbonKeyTip
    \value PE_Backstage
*/

/*!
    \enum Qtitan::RibbonStyle::QContentsType

    \value CT_RibbonSliderButton
    \value CT_RibbonGroupButton

*/

/*!
    \enum Qtitan::RibbonStyle::QControlElement

    \value CE_RibbonTabShapeLabel
    \value CE_RibbonFileButtonLabel
    \value CE_RibbonBar
    \value CE_RibbonGroups
    \value CE_Group
    \value CE_ReducedGroup
    \value CE_PopupSizeGrip
*/

/*!
    \enum Qtitan::RibbonStyle::QStyleHint

    \value SH_FlatFrame
    \value SH_RibbonBackstageHideTabs
*/

static const int windowsSepHeight        =  9; // height of the separator
static const int windowsItemFrame        =  2; // menu item frame width
static const int windowsItemHMargin      =  3; // menu item hor text margin
static const int windowsCheckMarkWidth   = 16; // checkmarks width on windows
static const int windowsItemVMargin      =  2; // menu item ver text margin
static const int windowsRightBorder      = 15; // right border on windows
static const int windowsTabSpacing       = 12; // space between text and tab
static const int windowsArrowHMargin     =  6; // arrow horizontal margin
static const int splitActionPopupWidth   = 20; // Split button drop down width in popups


#ifdef Q_OS_WIN
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
QT_BEGIN_NAMESPACE
    Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);
QT_END_NAMESPACE
#endif // QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#endif // Q_OS_WIN


static bool use2000style = true;


/* RibbonStylePrivate */
double RibbonStylePrivate::logPixel = 96.0;
RibbonStylePrivate::RibbonStylePrivate()
{
    m_baseHeight = -1;
    m_tabsHeight = 22;
    m_rowHeight = 22;
    m_fileButtonImageCount = 3;
    m_flatFrame = false;
    m_destroyKeyTips = false;
    m_completeKey = false;
    m_blockKeyTips = false;
}

void RibbonStylePrivate::initialization()
{
//    QTN_P(RibbonStyle);
    m_tabsHeight = m_rowHeight > 22 ? m_rowHeight + 2 : 23;
    refreshMetrics(Q_NULL);
    updateColors();
}


#ifdef Q_OS_WIN
static QFont qtn_LOGFONTtoQFont(LOGFONT& lf, bool dpiaware)
{
    QString family = QString::fromWCharArray(lf.lfFaceName);
    QFont qf(family);
    qf.setItalic(lf.lfItalic);
    if (lf.lfWeight != FW_DONTCARE) 
    {
        int weight;
        if (lf.lfWeight < 400)
            weight = QFont::Light;
        else if (lf.lfWeight < 600)
            weight = QFont::Normal;
        else if (lf.lfWeight < 700)
            weight = QFont::DemiBold;
        else if (lf.lfWeight < 800)
            weight = QFont::Bold;
        else
            weight = QFont::Black;
        qf.setWeight(weight);
    }
    int lfh = qAbs(lf.lfHeight);

    double factor = 64.0;
    if (QSysInfo::windowsVersion() <= QSysInfo::WV_XP) 
        factor = 72.0;

    HDC displayDC = GetDC(0);
    double currentlogPixel = (double)GetDeviceCaps(displayDC, LOGPIXELSY);
    double delta = RibbonStylePrivate::logPixel/currentlogPixel;
    double scale = factor*(dpiaware ? 1 : delta);

    qf.setPointSizeF((double)lfh * scale / currentlogPixel);

    ReleaseDC(0, displayDC);

    qf.setUnderline(false);
    qf.setOverline(false);
    qf.setStrikeOut(false);

    return qf;
}
#endif 

void RibbonStylePrivate::refreshMetrics(QWidget* widget)
{
    QTN_P(RibbonStyle);

#ifdef Q_OS_WIN
    NONCLIENTMETRICS ncm;
    ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICS, lfMessageFont) + sizeof(LOGFONT);
    ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize , &ncm, 0);
    m_fontRegular = widget ? widget->font() : qtn_LOGFONTtoQFont(ncm.lfMenuFont, p.isDPIAware());

    QFontMetrics fm(m_fontRegular);
    m_baseHeight = fm.overlinePos();

    if (m_baseHeight <= 12) 
        m_baseHeight = 11;

    m_rowHeight = qMax(22, m_baseHeight * 195 / (int)logPixel - 3);

    QTabBar tab;
    tab.setFont(qobject_cast<Qtitan::RibbonTabBar*>(widget) ? widget->parentWidget()->font() : m_fontRegular);
    tab.setAttribute(Qt::WA_MacSmallSize);
    tab.insertTab(0, QObject::tr("TEXT"));

    int hSpace = p.proxy()->pixelMetric(QStyle::PM_TabBarTabHSpace, Q_NULL, Q_NULL);
    if (!p.isDPIAware())
        hSpace = (double)hSpace/((double)(DrawHelpers::getDPIToPercent()/100.0));

    int hframe = qMax(hSpace, tab.sizeHint().height());
    int heightTab = qMax(22, hframe + 1);
    m_tabsHeight = heightTab > 22 ? heightTab : 23;
#else 
    QMenuBar menu;
    menu.setAttribute(Qt::WA_MacSmallSize);
    m_fontRegular = widget ? widget->font() : QApplication::font(&menu);

    QLineEdit ed;
    ed.setAttribute(Qt::WA_MacSmallSize);
    ed.setFont(m_fontRegular);
    QSize sz = ed.sizeHint();
    m_rowHeight = qMax(22, sz.height());

    QTabBar tab;
    tab.setFont(qobject_cast<Qtitan::RibbonTabBar*>(widget) ? widget->parentWidget()->font() : m_fontRegular);
    tab.setAttribute(Qt::WA_MacSmallSize);
    tab.insertTab(0, QObject::tr("TEXT"));

    int hframe = qMax(p.proxy()->pixelMetric(QStyle::PM_TabBarTabHSpace, Q_NULL, Q_NULL), tab.sizeHint().height());
    int heightTab = qMax(22, hframe + 1);
    m_tabsHeight = heightTab > 22 ? heightTab : 23;
#endif 
}

void RibbonStylePrivate::updateColors()
{
    QTN_P(RibbonStyle);

    if (RibbonStyle::getStyleVersion() > 0)
    {
        m_clrFileButtonText = QColor(0, 0, 0);
    }
    else
    {
        m_clrFileButtonText = QColor(255, 255, 255);
    }

    m_clrRibbonGrayText = m_clrMenuBarGrayText;

    StyleHelper& helper = p.helper();

    // [Ribbon]
    m_clrRibbonGroupCaptionText = helper.getColor("Ribbon", "GroupCaptionText");
    m_clrRibbonFace = helper.getColor("Ribbon", "RibbonFace");
    m_clrRibbonSeparator = helper.getColor("Ribbon", "RibbonSeparator");
    m_clrRibbonLight = helper.getColor("Ribbon", "RibbonLight");    

    m_clrRibbonText = helper.getColor("Ribbon", "RibbonText");
    m_clrGroupClientText  = helper.getColor("Ribbon", "GroupClientText");
    m_clrGroupClientGrayText = QColor(141, 141, 141);
    m_clrRibbonInactiveFace = helper.getColor("Ribbon", "RibbonInactiveFace", m_clrRibbonFace);

    m_clrMinimizedFrameEdgeHighLight = helper.getColor("Ribbon", "MinimizedFrameEdgeHighLight");
    m_clrMinimizedFrameEdgeShadow = helper.getColor("Ribbon", "MinimizedFrameEdgeShadow");
    m_flatFrame = (bool)helper.getInteger("Window", "FlatFrame", 0);
    m_fileButtonImageCount = helper.getInteger("Ribbon", "FileButtonImageCount", 3);

    m_paintManeger->modifyColors();
}

void RibbonStylePrivate::tabLayout(const QStyleOptionTabV3* opt, const QWidget* widget, QRect* textRect, QRect* iconRect) const
{
    QTN_P(const RibbonStyle);
    const QStyle* proxyStyle = p.proxy();

    Q_ASSERT(textRect);
    Q_ASSERT(iconRect);
    QRect tr = opt->rect;
    bool verticalTabs = opt->shape == QTabBar::RoundedEast
        || opt->shape == QTabBar::RoundedWest
        || opt->shape == QTabBar::TriangularEast
        || opt->shape == QTabBar::TriangularWest;

    bool botton = opt->shape == QTabBar::RoundedSouth;

    if (verticalTabs)
        tr.setRect(0, 0, tr.height(), tr.width()); //0, 0 as we will have a translate transform

    int verticalShift = proxyStyle->pixelMetric(QStyle::PM_TabBarTabShiftVertical, opt, widget);
    int horizontalShift = proxyStyle->pixelMetric(QStyle::PM_TabBarTabShiftHorizontal, opt, widget);

    int xShift = botton ? 0 : 5;
    int hpadding = proxyStyle->pixelMetric(QStyle::PM_TabBarTabHSpace, opt, widget) / 2 - xShift;
    int vpadding = proxyStyle->pixelMetric(QStyle::PM_TabBarTabVSpace, opt, widget) / 2;

    if (opt->shape == QTabBar::RoundedSouth || opt->shape == QTabBar::TriangularSouth)
        verticalShift = -verticalShift;

    tr.adjust(hpadding, verticalShift - vpadding, horizontalShift - hpadding, vpadding);

    bool selected = opt->state & QStyle::State_Selected;
    if (selected)
    {
        tr.setTop(tr.top() - verticalShift);
        tr.setRight(tr.right() - horizontalShift);
    }

    // left widget
    if (!opt->leftButtonSize.isEmpty())
    {
        tr.setLeft(tr.left() + 4 +
            (verticalTabs ? opt->leftButtonSize.height() : opt->leftButtonSize.width()));
    }
    // right widget
    if (!opt->rightButtonSize.isEmpty())
    {
        tr.setRight(tr.right() - 4 -
            (verticalTabs ? opt->rightButtonSize.height() : opt->rightButtonSize.width()));
    }

    // icon
    if (!opt->icon.isNull())
    {
        QSize iconSize = opt->iconSize;
        if (!iconSize.isValid())
        {
            int iconExtent = proxyStyle->pixelMetric(QStyle::PM_SmallIconSize);
            iconSize = QSize(iconExtent, iconExtent);
        }
        QSize tabIconSize = opt->icon.actualSize(iconSize,
            (opt->state & QStyle::State_Enabled) ? QIcon::Normal : QIcon::Disabled,
            (opt->state & QStyle::State_Selected) ? QIcon::On : QIcon::Off  );

        *iconRect = QRect(tr.left(), tr.center().y() - tabIconSize.height() / 2,
            tabIconSize.width(), tabIconSize .height());

        xShift = botton ? 6 : 0;

        if (!verticalTabs && botton && !((opt->position == QStyleOptionTab::OnlyOneTab || opt->position == QStyleOptionTab::Beginning)))
            xShift = 0;

        if (opt->shape == QTabBar::RoundedEast && opt->position != QStyleOptionTab::OnlyOneTab && opt->position != QStyleOptionTab::Beginning)
        {
//            iconRect->translate(xShift-11, 0);
        }
        else
            iconRect->translate(xShift, 0);

        if (!verticalTabs)
            *iconRect = proxyStyle->visualRect(opt->direction, opt->rect, *iconRect);

        xShift = botton ? /*2 +*/ tabIconSize.width() : 4;

        if (!verticalTabs && botton && !((opt->position == QStyleOptionTab::OnlyOneTab || opt->position == QStyleOptionTab::Beginning)))
            xShift -= 7;

        tr.translate(xShift, 0);
    }

    if (!verticalTabs)
        tr = proxyStyle->visualRect(opt->direction, opt->rect, tr);
    
    *textRect = tr;
}

void RibbonStylePrivate::tabHoverUpdate(QTabBar* tabBar, QEvent* event)
{
    if (event->type() == QEvent::HoverMove || event->type() == QEvent::HoverEnter)
    {
        QHoverEvent* he = static_cast<QHoverEvent*>(event);
        int index = tabBar->tabAt(he->pos());
        if (index != -1)
        {
            QRect rect = tabBar->tabRect(index);
            switch (tabBar->shape()) 
            {
                case QTabBar::RoundedNorth:
                case QTabBar::TriangularNorth:
                    rect.adjust(0, 0, 11, 0);
                    break;
                case QTabBar::RoundedSouth:
                case QTabBar::TriangularSouth:
//                    if (tab->position != QStyleOptionTab::Beginning)
                        rect.adjust(-11, 0, 0, 0);
                    break;
                case QTabBar::RoundedWest:
                case QTabBar::TriangularWest:
                    rect.adjust(0, -11, 0, 0);
                    break;
                case QTabBar::RoundedEast:
                case QTabBar::TriangularEast:
                    rect.adjust(0, -11, 0, 0);
                    break;
                default:
                    break;
            }

            m_oldHoverRectTab = rect;
            tabBar->update(rect);
        }
    } 
    else if (event->type() == QEvent::HoverLeave)
    {
        tabBar->update(m_oldHoverRectTab);
        m_oldHoverRectTab = QRect();
    }
}

void RibbonStylePrivate::setMacSmallSize(QWidget* widget)
{
    m_macSmallSizeWidgets.insert(widget, widget->testAttribute(Qt::WA_MacSmallSize));
    widget->setAttribute(Qt::WA_MacSmallSize, true);
}

void RibbonStylePrivate::unsetMacSmallSize(QWidget* widget)
{
    if (m_macSmallSizeWidgets.contains(widget)) 
    {
        bool testMacSmallSize = m_macSmallSizeWidgets.value(widget);
        widget->setAttribute(Qt::WA_MacSmallSize, testMacSmallSize);
        m_macSmallSizeWidgets.remove(widget);
    }
}

IRibbonPaintManager* RibbonStylePrivate::ribbonPaintManager() const
{
    QTN_P(const RibbonStyle);
    IRibbonPaintManager* ribbonPaintManager = qobject_cast<IRibbonPaintManager*>(&p.paintManager());
    return ribbonPaintManager;
}

/*! \internal */
void RibbonStylePrivate::makePaintManager()
{
    QTN_P(RibbonStyle)
    if (p.getTheme() == OfficeStyle::Office2013White || p.getTheme() == OfficeStyle::Office2013Gray)
        setPaintManager(*new RibbonPaintManager2013(&p));
    else
        setPaintManager(*new RibbonPaintManager(&p));
}

namespace {
    int ribbonStyleVersion = 0;
}

int RibbonStyle::getStyleVersion()
{
    return ribbonStyleVersion;
}

void RibbonStyle::setStyleVersion(int styleVersion)
{
    ribbonStyleVersion = styleVersion;
}

/* RibbonStyle */
/*!
Constuctor of the RibbonStyle.
*/
RibbonStyle::RibbonStyle()
    : OfficeStyle(*new RibbonStylePrivate)
{
    QTN_D(RibbonStyle);
    d.initialization();
}

/*!
Constructor of the OfficeStyle. The application is usually required to use this constructor only once. 
After this, the call QApplcation::setStyle(...) is not required. Parameter \a mainWindow is not used.
*/
RibbonStyle::RibbonStyle(QMainWindow* mainWindow)
    : OfficeStyle(mainWindow, *new RibbonStylePrivate)
{
//    QTN_INIT_PRIVATE(RibbonStyle);
    QTN_D(RibbonStyle);
    d.initialization();
}

/*!
Destructor of the OfficeStyle.
*/
RibbonStyle::~RibbonStyle()
{
//    QTN_FINI_PRIVATE();
}

static void resetPolished(QWidget* w) 
{
    w->setAttribute(Qt::WA_WState_Polished, true);
    w->setFont(QFont());

    QList<QWidget*> widgets = w->findChildren<QWidget*>();
    foreach (QWidget* wd, widgets)
        resetPolished(wd);
}

/*! \reimp */
void RibbonStyle::polish(QApplication* application)
{
    OfficeStyle::polish(application);

    if (application)
        application->installEventFilter(this);

    QTN_D(RibbonStyle);
    d.refreshMetrics(Q_NULL);
    d.updateColors();
}

/*! \reimp */
void RibbonStyle::unpolish(QApplication* application)
{
    OfficeStyle::unpolish(application);

    if (application)
        application->removeEventFilter(this);
}

/*! \reimp */
void RibbonStyle::polish(QWidget* widget)
{
    OfficeStyle::polish(widget);
    QTN_D(RibbonStyle);

    if (qobject_cast<RibbonBar*>(widget))
    {
        d.setMacSmallSize(widget);
        widget->setFont(d.m_fontRegular);

        QPalette palette = widget->palette();
        palette.setColor(QPalette::Light, d.m_clrMenuBarGrayText);
        palette.setColor(QPalette::ButtonText, d.m_clrRibbonText);
        widget->setPalette(palette);
    }
    else if (qobject_cast<QStatusBar*>(widget))
    {
        widget->setFont(d.m_fontRegular);

        QPalette palette = widget->palette();
        palette.setColor(QPalette::Light, d.m_clrMenuBarGrayText);
        palette.setColor(QPalette::ButtonText, /*d.m_clrRibbonText*/ d.m_clrStatusBarText);
        widget->setPalette(palette);
    }
    else if (qobject_cast<QMenu*>(widget) && getParentWidget<RibbonBar>(widget))
    {
        if (DrawHelpers::getDPIToPercent() > 100)
            widget->setFont(d.m_fontRegular);
    }
    else if (qobject_cast<RibbonGroup*>(widget))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Light, d.m_clrMenuBarGrayText);
        palette.setColor(QPalette::WindowText, d.m_clrRibbonGroupCaptionText);
        palette.setColor(QPalette::ButtonText, d.m_clrGroupClientText);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, d.m_clrGroupClientGrayText);
        widget->setPalette(palette);
    }
    else if (qobject_cast<RibbonDefaultGroupButton*>(widget))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Light, d.m_clrToolBarGrayText);
        palette.setColor(QPalette::WindowText, d.m_clrToolBarText);
        widget->setPalette(palette);
    }
    else if ((qobject_cast<RibbonGroup*>(widget->parentWidget()) &&
             qobject_cast<QToolButton*>(widget)))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Light, d.m_clrToolBarGrayText);
        palette.setColor(QPalette::WindowText, d.m_clrToolBarText);
        widget->setPalette(palette);
    }
    else if (qobject_cast<RibbonSystemPopupBar*>(widget->parentWidget()) && qobject_cast<QToolButton*>(widget))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Light, d.m_clrMenuBarGrayText);
        palette.setColor(QPalette::ButtonText, d.m_clrMenuPopupText);
        widget->setPalette(palette);
    }
    else if (qobject_cast<RibbonSystemButton*>(widget))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::ButtonText, d.m_clrFileButtonText);
        widget->setPalette(palette);
    }
    else if (qobject_cast<OfficePopupMenu*>(widget->parentWidget()) && qobject_cast<QLabel*>(widget))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::WindowText, getTextColor(false, false, true, false, false, TypePopup, BarNone));
        widget->setPalette(palette);
    }
    else if (qobject_cast<QMdiArea*>(widget))
    {
        d.ribbonPaintManager()->setupPalette(widget);
    }
    else if (qobject_cast<QTabBar*>(widget) && qobject_cast<QMdiArea*>(widget->parentWidget()))
    {
        widget->setAutoFillBackground(true);

        QPalette palette = widget->palette();
        palette.setColor(QPalette::Background, helper().getColor(tr("TabManager"), tr("AccessHeader")));
        widget->setPalette(palette);

        ((QTabBar*)widget)->setExpanding(false);
        widget->installEventFilter(this);
    }
    else if (qobject_cast<RibbonKeyTip*>(widget))
    {
        d.ribbonPaintManager()->setupPalette(widget);
    }
}

/*! \reimp */
void RibbonStyle::unpolish(QWidget* widget)
{
    OfficeStyle::unpolish(widget);
    QTN_D(RibbonStyle);

    if (qobject_cast<RibbonBar*>(widget))
        resetPolished(widget); 

    if (getParentWidget<RibbonBar>(widget) || qobject_cast<RibbonBar*>(widget)) 
        d.unsetMacSmallSize(widget);

    if (static_cast<RibbonBar*>((RibbonBar*)widget))
        widget->removeEventFilter(this);

    if (qobject_cast<QTabBar*>(widget) && qobject_cast<QMdiArea*>(widget->parentWidget()))
    {
        widget->removeEventFilter(this);
        ((QTabBar*)widget)->setExpanding(true);
    }
}

/*! \reimp */
void RibbonStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    int qtnElement = element;

    switch (qtnElement)
    {
        case PE_RibbonPopupBarButton :
                drawPopupSystemButton(option, painter, widget);
            break;
        case PE_RibbonFileButton :
                drawSystemButton(option, painter, widget);
            break;
        case PE_RibbonOptionButton :
                drawOptionButton(option, painter, widget);
            break;
        case PE_RibbonGroupScrollButton :
                drawGroupScrollButton(option, painter, widget);
            break;
        case PE_RibbonQuickAccessButton :
                drawQuickAccessButton(option, painter, widget);
            break;
        case PE_RibbonTab :
                drawTabShape(option, painter, widget);
            break;
        case PE_RibbonContextHeaders:
                drawContextHeaders(option, painter);
            break;
        case PE_RibbonFillRect :
                drawFillRect(option, painter, widget);
            break;
        case PE_RibbonRect :
                drawRect(option, painter, widget);
            break;
        case PE_RibbonKeyTip :
                drawKeyTip(option, painter, widget);
            break;
        case PE_Backstage :
                drawBackstage(option, painter, widget);
            break;
        case PE_RibbonBackstageCloseButton :
                drawRibbonBackstageCloseButton(option, painter, widget);
            break;
        case PE_RibbonSliderButton :
                drawRibbonSliderButton(option, painter, widget);
            break;
        default:
            OfficeStyle::drawPrimitive(element, option, painter, widget);
    }
}

/*! \reimp */
void RibbonStyle::drawControl(ControlElement element, const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    switch (element)
    {
        case CE_RibbonTabShapeLabel:
                drawTabShapeLabel(opt, p, widget);
            break;
        case CE_RibbonBar : 
                drawRibbonBar(opt, p, widget);
             break;
        case CE_RibbonFileButtonLabel :
                drawFileButtonLabel(opt, p, widget);
            break;
        case CE_RadioButtonLabel:
        case CE_CheckBoxLabel: 
            {
                if (const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton*>(opt))
                {
                    uint alignment = visualAlignment(btn->direction, Qt::AlignLeft | Qt::AlignVCenter);
                    if (!proxy()->styleHint(SH_UnderlineShortcut, btn, widget))
                        alignment |= Qt::TextHideMnemonic;
                    QPixmap pix;
                    QRect textRect = btn->rect;
                    if (!btn->icon.isNull()) 
                    {
                        pix = btn->icon.pixmap(btn->iconSize, btn->state & State_Enabled ? QIcon::Normal : QIcon::Disabled);
                        proxy()->drawItemPixmap(p, btn->rect, alignment, pix);
                        if (btn->direction == Qt::RightToLeft)
                            textRect.setRight(textRect.right() - btn->iconSize.width() - 4);
                        else
                            textRect.setLeft(textRect.left() + btn->iconSize.width() + 4);
                    }
                    if (!btn->text.isEmpty())
                        proxy()->drawItemText(p, textRect, alignment | Qt::TextShowMnemonic, btn->palette, 
                            btn->state & State_Enabled, btn->text, QPalette::ButtonText);
                }
                break;
            }
        case CE_RibbonGroups: 
                drawRibbonGroups(opt, p, widget);
            break;
        case CE_Group : 
                drawGroup(opt, p, widget);
            break;
        case CE_ReducedGroup : 
                drawReducedGroup(opt, p, widget);
            break;
        case CE_PopupSizeGrip :
                drawPopupResizeGripper(opt, p, widget);
            break;
        default:
                OfficeStyle::drawControl(element, opt, p, widget);
            break;
    }
}

/*! \reimp */
void RibbonStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const
{
    OfficeStyle::drawComplexControl(cc, option, painter, widget);
}

/*! \reimp */
int RibbonStyle::pixelMetric(PixelMetric pm, const QStyleOption* option, const QWidget* widget) const
{
    int ret = 0;
    QTN_D(const RibbonStyle);
    switch(pm) 
    {
        case PM_ButtonIconSize:
        case PM_SmallIconSize:
            {
                if (widget && (qobject_cast<const RibbonStatusBar*>(widget->parentWidget()) || 
                    qobject_cast<const RibbonStatusBarSwitchGroup*>(widget->parentWidget())) ) 
                    ret = int(DrawHelpers::dpiScaled(14.));
                else
                    ret = OfficeStyle::pixelMetric(pm, option, widget);
            }
            break;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        case PM_MenuBarPanelWidth:
                ret = OfficeStyle::pixelMetric(pm, option, widget) - pixelMetric(QStyle::PM_MenuBarVMargin, 0, widget);
            break;
#endif
        case PM_MenuBarVMargin:
            {
                ret = OfficeStyle::pixelMetric(pm, option, widget);
                if (!option)
                {
                    if (const RibbonBar* ribbonBar = qobject_cast<const RibbonBar*>(widget))
                        ret += ribbonBar->topBorder() + ribbonBar->titleBarHeight() + 2;
                }
            }
            break;

        case PM_MenuBarHMargin:
                ret = 2;
            break;

        case PM_TabBarTabHSpace :
            {
                ret = OfficeStyle::pixelMetric(pm, option, widget);

                if (widget && qobject_cast<QMdiArea*>(widget->parentWidget()))
                {
                    if (const QStyleOptionTab* taOption = qstyleoption_cast<const QStyleOptionTab*>(option))
                    {
                        bool verticalTabs = taOption->shape == QTabBar::RoundedEast
                            || taOption->shape == QTabBar::RoundedWest
                            || taOption->shape == QTabBar::TriangularEast
                            || taOption->shape == QTabBar::TriangularWest;

                        bool botton = taOption->shape == QTabBar::RoundedSouth;
                        if (!verticalTabs && botton)
                        {
                            if (taOption->icon.isNull() && (taOption->position == QStyleOptionTab::OnlyOneTab || taOption->position == QStyleOptionTab::Beginning))
                                ret += 11;
                            else if (!taOption->icon.isNull() && !(taOption->position == QStyleOptionTab::OnlyOneTab || taOption->position == QStyleOptionTab::Beginning))
                                ret -= 11;
                        }
                    }
                }
            }
            break;
        case PM_RibbonReducedGroupWidth : 
            if(const QStyleOptionToolButton* but = qstyleoption_cast<const QStyleOptionToolButton*>(option))
            {
                QString strFirstRow, strSecondRow;
                CommonStyle::splitString(but->text, strFirstRow, strSecondRow);
                if(strFirstRow.count() < strSecondRow.count())
                    strFirstRow = strSecondRow;
                    
                QFontMetrics fm(but->fontMetrics);
                int wid = fm.width(strFirstRow) + 12 + 4; // + 4 - Glyph 
                    
                QPixmap soCenter = cached("RibbonGroupButton.png");
                QRect rcSrc = sourceRectImage(soCenter.rect(), 0, 3);
                ret = qMax(ret, qMax(wid, rcSrc.width()));
            }
            break;
        case PM_RibbonHeightGroup :
            {
                ret = d.m_rowHeight*3;
                break;
            }
        case PM_RibbonHeightCaptionGroup :
            {
                ret = option->fontMetrics.height() + 3;
                break;
            }
        case PM_RibbonTabsHeight :
            {
                ret = d.m_tabsHeight;
                break;
            }
        case PM_TitleBarHeight : 
            {
                ret = OfficeStyle::pixelMetric(pm, option, widget);
                break;
            }
        case PM_MenuHMargin :
            {
                if (qobject_cast<const RibbonSystemPopupBar*>(widget)) 
                    ret = 6;
                else if(qobject_cast<const OfficePopupMenu*>(widget))
                    ret = -1;
                else
                    ret = OfficeStyle::pixelMetric( pm, option, widget );
                break;
            }
        case PM_MenuVMargin :
            {
                if (qobject_cast<const RibbonSystemPopupBar*>(widget)) 
                    ret = 19;
                else if(qobject_cast<const RibbonBackstageView*>(widget))
                    ret = 7;
                else if(qobject_cast<const OfficePopupMenu*>(widget))
                    ret = 0;
                else
                    ret = OfficeStyle::pixelMetric(pm, option, widget);
                break;
            }
        case PM_ToolBarIconSize :
            {
                if (!isDPIAware() && getParentWidget<RibbonBar>(widget))
                {
                    ret = OfficeStyle::pixelMetric(pm, option, widget);
                    ret = (double)ret/((double)(DrawHelpers::getDPIToPercent()/100.0));
                }
                else if (qobject_cast<const RibbonSystemPopupBar*>(widget) || qobject_cast<const RibbonPageSystemPopup*>(widget)) 
                    ret = 32;
                else
                    ret = OfficeStyle::pixelMetric( pm, option, widget );
                break;
            }
        case PM_ToolBarFrameWidth :
            {
                if (qobject_cast<const RibbonQuickAccessBar*>(widget)) 
                    ret = 0;
                else
                    ret = OfficeStyle::pixelMetric( pm, option, widget );
                break;
            }
        // Returns the number of pixels to use for the business part of the
        // slider (i.e., the non-tickmark portion). The remaining space is shared
        // equally between the tickmark regions.
        case PM_SliderControlThickness :
            {
                if (const QStyleOptionSlider *sl = qstyleoption_cast<const QStyleOptionSlider *>(option)) 
                {
                    int space = (sl->orientation == Qt::Horizontal) ? sl->rect.height() : sl->rect.width();
                    int ticks = sl->tickPosition;
                    int n = 0;
                    if (ticks & QSlider::TicksAbove)
                        ++n;
                    if (ticks & QSlider::TicksBelow)
                        ++n;
                    if (!n) 
                    {
                        ret = space;
                        break;
                    }

                    int thick = 6;  // Magic constant to get 5 + 16 + 5
                    if (ticks != QSlider::TicksBothSides && ticks != QSlider::NoTicks)
                        thick += proxy()->pixelMetric(PM_SliderLength, sl, widget) / 4;

                    space -= thick;
                    if (space > 0)
                        thick += (space * 2) / (n + 2);
                    ret = thick;
                } else {
                    ret = 0;
                }
           }
            break;
        case PM_ToolBarSeparatorExtent:
            if (widget && (widget->metaObject()->className() == QString("Qtitan::RibbonStatusBar")))
                ret = int(DrawHelpers::dpiScaled(3.));
            else
                ret = OfficeStyle::pixelMetric(pm, option, widget);
            break;

        default:
            ret = OfficeStyle::pixelMetric(pm, option, widget);
        break;
    }
    return ret;
}

/*! \reimp */
int RibbonStyle::styleHint(StyleHint hint, const QStyleOption* opt, const QWidget* widget, QStyleHintReturn *returnData) const
{
    QTN_D(const RibbonStyle);
    int ret = 0;
    if (hint == QStyle::SH_ToolButtonStyle)
    {
        if (const RibbonSystemButton* sysButton = qobject_cast<const RibbonSystemButton*>(widget))
            ret = sysButton->toolButtonStyle();
    }
    else if (hint == (StyleHint)SH_FlatFrame)
    {
        ret = d.m_flatFrame;
    }
    else if (hint == SH_UnderlineShortcut)
    {
        const RibbonBar* ribbonBar = ::getParentWidget<const RibbonBar>(widget);
        if (ribbonBar && !qobject_cast<const QMenu*>(widget))
            ret = 0;
        else if (ribbonBar && ribbonBar->qtn_d().m_keyTips.size() > 0)
            ret = 1;
        else
            ret = OfficeStyle::styleHint(hint, opt, widget, returnData);
    }
    else if (hint == (StyleHint)SH_RibbonBackstageHideTabs)
    {
        return d.ribbonPaintManager()->isRibbonBackstageHideTabs();
    }
    else
        ret = OfficeStyle::styleHint(hint, opt, widget, returnData);
    return ret;
}

/*! \internal */
QPixmap RibbonStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption* opt, const QWidget* widget) const
{
    return OfficeStyle::standardPixmap( standardPixmap, opt, widget );
}

/*! \reimp */
QSize RibbonStyle::sizeFromContents(ContentsType ct, const QStyleOption* opt, const QSize& csz, const QWidget* widget) const
{
    QSize sz(csz);
    switch (ct)
    {
        case CT_ToolButton:
            {
                if (widget && qobject_cast<const RibbonQuickAccessBar*>(widget->parentWidget()))
                {
                    sz = csz;
                    int width = csz.width()-1;
                    int height = csz.height()-2;
                    sz.setWidth(width);
                    sz.setHeight(height);
                } 
                else if (qobject_cast<const RibbonSystemButton*>(widget))
                {
                    sz = OfficeStyle::sizeFromContents(ct, opt, csz, widget);
                }
                else if (widget && (qobject_cast<const RibbonStatusBar*>(widget->parentWidget()) || 
                                    qobject_cast<const RibbonStatusBarSwitchGroup*>(widget->parentWidget())))
                {
                    if (const QStyleOptionToolButton* optToolButton = qstyleoption_cast<const QStyleOptionToolButton*>(opt))
                    {
                        if (!optToolButton->icon.isNull())
                            sz += QSize(5, 4);
                        else
                            sz.setHeight(sz.height() + 5);
                    }
                }
                else
                    sz = OfficeStyle::sizeFromContents(ct, opt, csz, widget);
            }
            break;
        case CT_MenuItem :
            {
                const QMenu* menu = qobject_cast<const RibbonSystemPopupBar*>(widget);
                if (!menu)
                {
                    menu = qobject_cast<const RibbonPageSystemPopup*>(widget);
                    if (menu)
                    {
                        if (const QStyleOptionMenuItem* mi = qstyleoption_cast<const QStyleOptionMenuItem*>(opt))
                        {
                            if (mi->text.count(QString(qtn_PopupLable)) > 0)
                            {
                                sz = OfficeStyle::sizeFromContents(ct, opt, csz, widget);
                                sz.setWidth(1);
                                break;
                            }
                        }
                    }
                }

                if (menu)
                {
                    if (const QStyleOptionMenuItem* mi = qstyleoption_cast<const QStyleOptionMenuItem*>(opt))
                    {
                        int w = sz.width();
                        sz = OfficeStyle::sizeFromContents(ct, opt, csz, widget);

                        if (mi->menuItemType == QStyleOptionMenuItem::Separator)
                        {
                            sz = QSize(10, windowsSepHeight);
                        }
                        else if (mi->icon.isNull()) 
                        {
                            sz.setHeight(sz.height() - 2);

                            int maxheight = 0;
                            QList<QAction*> list = menu->actions();
                            for (int j = 0; j < list.count(); j++)
                            {
                                QAction* action = list.at(j);
                                if (!action->icon().isNull())
                                {
                                    int iconExtent = pixelMetric(PM_LargeIconSize, opt, widget);
                                    maxheight = qMax(sz.height(), action->icon().actualSize(QSize(iconExtent, iconExtent)).height()+ 2 * windowsItemFrame);
                                }
                            }

                            if (maxheight > 0)
                                sz.setHeight(maxheight);

                            w -= 6;
                        }

                        if (mi->menuItemType != QStyleOptionMenuItem::Separator && !mi->icon.isNull()) 
                        {
                            int iconExtent = pixelMetric(PM_LargeIconSize, opt, widget);
                            sz.setHeight(qMax(sz.height(), mi->icon.actualSize(QSize(iconExtent, iconExtent)).height()+ 2 * windowsItemFrame));
                        }
                        int maxpmw = mi->maxIconWidth;
                        int tabSpacing = use2000style ? 20 :windowsTabSpacing;
                        if (mi->text.contains(QLatin1Char('\t')))
                            w += tabSpacing;
                        else if (mi->menuItemType == QStyleOptionMenuItem::SubMenu)
                            w += 2 * windowsArrowHMargin;
                        else if (mi->menuItemType == QStyleOptionMenuItem::DefaultItem) 
                        {
                            // adjust the font and add the difference in size.
                            // it would be better if the font could be adjusted in the initStyleOption qmenu func!!
                            QFontMetrics fm(mi->font);
                            QFont fontBold = mi->font;
                            fontBold.setBold(true);
                            QFontMetrics fmBold(fontBold);
                            w += fmBold.width(mi->text) - fm.width(mi->text);
                        }

                        int checkcol = qMax(maxpmw, windowsCheckMarkWidth); // Windows always shows a check column
                        w += checkcol;
                        w += windowsRightBorder+12;// + 10;
                        sz.setWidth(w);

                        if (mi->menuItemType != QStyleOptionMenuItem::Separator && qobject_cast<const RibbonPageSystemPopup*>(widget))
                        {
                            QFontMetrics fm(mi->font);
                            sz.setHeight(qMax(sz.height(), fm.height()*3 + fm.height()/2 + windowsItemVMargin*2));
                        }
                    }
                }
                else if (qobject_cast<const OfficePopupMenu*>(widget))
                {
                    if (const QStyleOptionMenuItem* mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) 
                    {
                        sz = OfficeStyle::sizeFromContents(ct, opt, csz, widget);
                        if (mi->menuItemType == QStyleOptionMenuItem::Separator)
                            sz.setHeight(csz.height());
                    }
                }
                else
                    sz = OfficeStyle::sizeFromContents(ct, opt, csz, widget);
            }
            break;
        case CT_RibbonSliderButton :
            {
                QPixmap soButton;
                soButton = opt->direction == Qt::RightToLeft ? cached("SliderUp.png") : cached("SliderDown.png");
                sz = sourceRectImage(soButton.rect(), opt->state==QStyle::State_None ? 0 : opt->state&QStyle::State_Sunken? 2 : 1, 3).size();
                if (sz.isNull())
                    sz = QSize(7, 7);
            }
            break;
        case CT_MenuBar:
            if (const RibbonBar* ribbonBar = qobject_cast<const RibbonBar*>(widget) )
            {
                if (!ribbonBar->isVisible())
                {
                    sz = OfficeStyle::sizeFromContents(ct, opt, csz, widget);
                    sz.setHeight(0);
                }
            }
            break;
        default:
            sz = OfficeStyle::sizeFromContents(ct, opt, csz, widget);
            break;
    }

    return sz;
}

/*! \reimp */
QRect RibbonStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex* opt, SubControl sc, const QWidget* widget) const
{
    QRect ret;
    switch (cc)
    {
        case CC_ToolButton:
            if (const QStyleOptionToolButton* tb = qstyleoption_cast<const QStyleOptionToolButton*>(opt)) 
            {
                int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, tb, widget);
                ret = tb->rect;
                switch (sc) 
                {
                case SC_ToolButton:
                    if ((tb->features& (QStyleOptionToolButton::MenuButtonPopup | QStyleOptionToolButton::PopupDelay))== QStyleOptionToolButton::MenuButtonPopup)
                    {
                        if (tb->toolButtonStyle == Qt::ToolButtonTextUnderIcon) 
                        {
                            QRect popupr = subControlRect(cc, opt, QStyle::SC_ToolButtonMenu, widget);
                            ret.adjust(0, 0, 0, -popupr.height());
                        }
                        else
                            ret.adjust(0, 0, -mbi, 0);
                        break;
                    }
                case SC_ToolButtonMenu:
                    if ((tb->features & (QStyleOptionToolButton::MenuButtonPopup | QStyleOptionToolButton::PopupDelay)) == QStyleOptionToolButton::MenuButtonPopup)
                    {
                        if (!qobject_cast<const RibbonDefaultGroupButton*>(widget)) 
                        {
                            if (tb->toolButtonStyle == Qt::ToolButtonTextUnderIcon) 
                            {
                                QString str(tb->text);
                                QString strFirstRow, strSecondRow;
                                splitString(str, strFirstRow, strSecondRow);

                                QSize textSize;
                                if (!strFirstRow.isEmpty())
                                {
                                    textSize = opt->fontMetrics.size(Qt::TextShowMnemonic, strFirstRow);
                                    textSize.setWidth(textSize.width() + opt->fontMetrics.width(QLatin1Char(' '))*2);
                                }
                                if (!strSecondRow.isEmpty())
                                {
                                    QSize textSize1 = opt->fontMetrics.size(Qt::TextShowMnemonic, strSecondRow);
                                    textSize1.setWidth(textSize1.width() + opt->fontMetrics.width(QLatin1Char(' '))*2);
                                    textSize.setWidth(qMax(textSize.width(), textSize1.width()));
                                }
                                ret.adjust(0, tb->iconSize.height()+8, 0, 0);
                                ret.setWidth(qMax(textSize.width(), opt->rect.width()));
                            } else
                                ret.adjust(ret.width() - mbi, 0, 0, 0);
                        }
                        break;
                    }
                default:
                    break;
                }
                ret = visualRect(tb->direction, tb->rect, ret);
            }
            break;
        default:
                ret = OfficeStyle::subControlRect(cc, opt, sc, widget);
            break;
    }
    return ret;
}

/*! \reimp */
QRect RibbonStyle::subElementRect(SubElement sr, const QStyleOption* opt, const QWidget* widget) const
{
    QRect r;
    switch (sr)
    {
        case SE_TabBarTabText:
            if (const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(opt))
            {
                if (qobject_cast<QMdiArea*>(widget->parentWidget()))
                {
                    QTN_D(const RibbonStyle);
                    QStyleOptionTabV3 tabV3(*tab);
                    QRect dummyIconRect;
                    d.tabLayout(&tabV3, widget, &r, &dummyIconRect);

                    bool verticalTabs = tab->shape == QTabBar::RoundedEast
                        || tab->shape == QTabBar::RoundedWest
                        || tab->shape == QTabBar::TriangularEast
                        || tab->shape == QTabBar::TriangularWest;

                    bool botton = tab->shape == QTabBar::RoundedSouth;
                    if (!verticalTabs && botton && tab->icon.isNull() && (tab->position == QStyleOptionTab::OnlyOneTab || tab->position == QStyleOptionTab::Beginning))
                        r.translate(6, 0);
                }
                else
                    r = OfficeStyle::subElementRect(sr, opt, widget);
            }
            break;
        case SE_TabBarTabLeftButton:
        case SE_TabBarTabRightButton:
            {
                r = OfficeStyle::subElementRect(sr, opt, widget);

                if (const QStyleOptionTabV3* tab = qstyleoption_cast<const QStyleOptionTabV3 *>(opt)) 
                {
                    QRect rc = r;
                    if (sr != SE_TabBarTabLeftButton)
                    {
                        if (tab->shape == QTabBar::RoundedSouth && (tab->position == QStyleOptionTab::OnlyOneTab || tab->position == QStyleOptionTab::Beginning))
                            rc.setLeft(rc.left() + rc.width()/2);
                        else if (tab->shape == QTabBar::RoundedWest && (tab->position == QStyleOptionTab::OnlyOneTab || tab->position == QStyleOptionTab::Beginning))
                            rc.setTop(rc.top() + rc.width()/2);
                        else if (tab->shape == QTabBar::RoundedEast)
                            rc.setTop(rc.top()/* - rc.width()/2*/);
                    }
                    r = rc;
                }
            }
            break;
        default:
                r = OfficeStyle::subElementRect(sr, opt, widget);
            break;
    }
    return r;
}

// for QForm
/*! \internal */
bool RibbonStyle::drawFrame(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const RibbonStyle);
    return d.ribbonPaintManager()->drawFrame(opt, p, w);
}

// for QForm
/*! \internal */
bool RibbonStyle::drawShapedFrame(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const RibbonStyle)
    bool ret = false;
    if (qobject_cast<const QMdiArea*>(w))
        ret = d.ribbonPaintManager()->drawShapedFrame(opt, p, w);
    else if (qobject_cast<const RibbonBackstageSeparator*>(w))
        ret = d.ribbonPaintManager()->drawRibbonBackstageSeparator(opt, p, w);
    return ret;
}

// for stausBar
/*! \internal */
bool RibbonStyle::drawPanelStatusBar(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const RibbonStyle);
    return d.ribbonPaintManager()->drawPanelStatusBar(opt, p, w);
}

/*! \internal */
void RibbonStyle::drawRibbonBar(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const RibbonStyle);
    d.ribbonPaintManager()->drawRibbonBar(opt, p, w);
}

/*! \internal */
void RibbonStyle::drawRibbonGroups(const QStyleOption* option, QPainter* p, const QWidget* widget) const
{
    QTN_D(const RibbonStyle);
    d.ribbonPaintManager()->drawRibbonGroups(option, p, widget);

    if (const StyleOptionRibbon* gr = qstyleoption_cast<const StyleOptionRibbon*>(option))
    {
        if (!gr->rectLogotype.isEmpty() && !gr->logotype.isNull())
            p->drawPixmap(gr->rectLogotype, gr->logotype);
    }
}

/*! \internal */
void RibbonStyle::drawGroup(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    if (paintAnimation(tp_ControlElement, CE_Group, opt, p, widget, 300))
        return;

    QTN_D(const RibbonStyle);
    d.ribbonPaintManager()->drawGroup(opt, p, widget);
}

/*! \internal */
void RibbonStyle::drawReducedGroup(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    //    if (paintAnimation(tp_ControlElement, CE_ReducedGroup, opt, p, widget, 300))
    //        return;
    QTN_D(const RibbonStyle);
    d.ribbonPaintManager()->drawReducedGroup(opt, p, widget);
}

// for SizeGrip
/*! \internal */
bool RibbonStyle::drawSizeGrip(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const RibbonStyle);
    return d.ribbonPaintManager()->drawSizeGrip(opt, p, w);
}

/*! \internal */
void RibbonStyle::drawContextHeaders(const QStyleOption* opt, QPainter* p) const
{
    QTN_D(const RibbonStyle);
    return d.ribbonPaintManager()->drawContextHeaders(opt, p);
}

/*! \internal */
bool RibbonStyle::drawIndicatorToolBarSeparator(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    QTN_D(const RibbonStyle);
    if (d.ribbonPaintManager()->drawIndicatorToolBarSeparator(opt, p, widget))
        return true;
    return OfficeStyle::drawIndicatorToolBarSeparator(opt, p, widget);
}

// for TitleBar
/*! \internal */
bool RibbonStyle::drawTitleBar(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const 
{
    QTN_D(const RibbonStyle)
    if (!qobject_cast<const RibbonBar*>(w))
        return OfficeStyle::drawTitleBar(opt, p, w);

    if (const StyleOptionTitleBar* optTitleBar = qstyleoption_cast<const StyleOptionTitleBar*>(opt))
    {
        if (optTitleBar->frameHelper)
        {
            QRect rcCaption = optTitleBar->rect;
            bool dwmEnabled = optTitleBar->frameHelper->isDwmEnabled();
            bool active = optTitleBar->state & State_Active;

            QRect rcText = optTitleBar->rcTitleText;
//            if (optTitleBar->titleBarState & Qt::WindowMaximized)
//                rcText.setTop(rcText.top() + optTitleBar->frameHelper->frameBorder() / 2);

            if (!dwmEnabled)
            {
                QRect rcSrc;
                QRect rcTopLeft;
                QPixmap soImage = cached("FrameTopLeft.png");
                if (!soImage.isNull())
                {
                    rcSrc = sourceRectImage(soImage.rect(), active ? 0 : 1, 2);
                    rcTopLeft = QRect(QPoint(rcCaption.left(), rcCaption.top()), QPoint(rcCaption.left() + rcSrc.width(), rcCaption.bottom()));
                    drawImage(soImage, *p, rcTopLeft, rcSrc, QRect(QPoint(0, 5), QPoint(0, 3)));
                }

                soImage = cached("FrameTopRight.png");
                QRect rcTopRight;
                if (!soImage.isNull())
                {
                    rcSrc = sourceRectImage(soImage.rect(), active ? 0 : 1, 2);
                    rcTopRight = QRect(QPoint(rcCaption.right() - rcSrc.width(), rcCaption.top()), QPoint(rcCaption.right(), rcCaption.bottom()));
                    drawImage(soImage, *p, rcTopRight, rcSrc, QRect(QPoint(0, 5), QPoint(0, 3)));
                }

                soImage = cached("FrameTopCenter.png");
                if (!soImage.isNull())
                {
                    rcSrc = sourceRectImage(soImage.rect(), active ? 0 : 1, 2);
                    QRect rcTopCenter(QPoint(rcTopLeft.right(), rcCaption.top()), QPoint(rcTopRight.left(), rcCaption.bottom()));
                    drawImage(soImage, *p, rcTopCenter, rcSrc, QRect(QPoint(0, 5), QPoint(0, 3)));
                }
            }
            else
            {
                optTitleBar->frameHelper->fillSolidRect(p, rcCaption, optTitleBar->airRegion, QColor(0, 0, 0));

                StyleOptionTitleBar optTitle = *optTitleBar;
                optTitle.rcTitleText = rcText;
                optTitleBar->frameHelper->drawTitleBar(p, optTitle);

                QTN_D(const RibbonStyle);
                if (d.m_flatFrame)
                {
                    QPixmap soImage = cached("RibbonTabBackgroundDwm.png");
                    if (!soImage.isNull())
                    {
                        QRect rcSrc = sourceRectImage(soImage.rect(), 0, 1);
                        QRect rcRibbonClient = optTitleBar->rcRibbonClient;
                        QRect rcRibbonTabs = optTitleBar->rectTabBar;
                        rcSrc.setBottom(rcSrc.bottom() - rcRibbonTabs.height());

                        if (rcSrc.height() > 0)
                        {
                            QRect rcArea(QPoint(rcRibbonClient.left(), rcRibbonTabs.top() - rcSrc.height()), 
                                QPoint(rcRibbonClient.right(), rcRibbonTabs.top()));
                            drawImage(soImage, *p, rcArea, rcSrc, QRect(QPoint(10, 0), QPoint(10, 0)), isStyle2010());
                        }
                    }
                }
            }

            if (optTitleBar->quickAccessOnTop && optTitleBar->quickAccessVisible && optTitleBar->quickAccessVisibleCount > 0)
            {
                if (optTitleBar->existSysButton && !optTitleBar->normalSysButton)
                {
                    QRect rcQuickAccess = optTitleBar->rcQuickAccess;
//                    QRect rcQuickAccessArea(QPoint(rcQuickAccess.left() - 15, rcQuickAccess.top() + 2), QPoint(rcQuickAccess.right(), rcQuickAccess.top() + 2 + rcQuickAccess.height()));
                    QRect rcQuickAccessArea = rcQuickAccess.adjusted(-16, -1, 4, 0);//(QPoint(rcQuickAccess.left() - 15, rcQuickAccess.top()), QPoint(rcQuickAccess.right(), rcQuickAccess.top() + rcQuickAccess.height()));

                    QPixmap soQuickAccessImage = cached(optTitleBar->frameHelper && optTitleBar->frameHelper->isDwmEnabled()?  "RibbonQuickAccessAreaDwm.png" : "RibbonQuickAccessArea.png" );
                    if (!soQuickAccessImage.isNull())
                        drawImage(soQuickAccessImage, *p, rcQuickAccessArea, soQuickAccessImage.rect(), QRect(QPoint(16, 3), QPoint(16, 3)));
                }
            }

            if ((!optTitleBar->existSysButton || optTitleBar->normalSysButton) && optTitleBar->frameHelper)
            {
                if (!optTitleBar->icon.isNull())
                {
                    int frameBorder = optTitleBar->frameHelper->frameBorder();
                    int topBorder = optTitleBar->frameHelper->isDwmEnabled() ? optTitleBar->frameHelper->frameBorder() : 0;

//                    QSize szIcon = proxy()->subControlRect(CC_TitleBar, optTitleBar, SC_TitleBarSysMenu, w).size();
//                    QSize szIcon = optTitleBar->icon.actualSize(QSize(64, 64));
                    QSize szIcon = optTitleBar->frameHelper->getSizeSystemIcon();

                    int top = topBorder / 2 - 1 + (rcCaption.bottom()  - szIcon.height())/2;
                    int left = rcCaption.left() + frameBorder + 1;

                    QPixmap soSeparatorImage = cached("SystemIconSeparator.png");
                    if (!soSeparatorImage.isNull())
                    {
                        QRect rcSrc = soSeparatorImage.rect();
                        QRect rcDest(QPoint(left + szIcon.height() + 6, top + szIcon.height() / 2 - rcSrc.height() / 2), rcSrc.size());
                        drawImage(soSeparatorImage, *p, rcDest, soSeparatorImage.rect());
                    }
                }
            }

            if (optTitleBar->listContextHeaders && optTitleBar->listContextHeaders->count() > 0)
                proxy()->drawPrimitive((QStyle::PrimitiveElement)RibbonStyle::PE_RibbonContextHeaders, opt, p, w);

            if (!dwmEnabled)
            {
                QPen savePen = p->pen();
                p->setPen(!active ? d.m_clrFrameCaptionTextInActive : d.m_clrFrameCaptionTextActive);
                QString text = optTitleBar->frameHelper ? optTitleBar->frameHelper->getWindowText() : QString();
                text = p->fontMetrics().elidedText(text.isEmpty() ? optTitleBar->text : text, Qt::ElideRight, rcText.width());
                p->drawText(rcText, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, text);
                p->setPen(savePen);
            }

            if (optTitleBar->drawIcon && optTitleBar->frameHelper)
            {
                if (!dwmEnabled && optTitleBar->subControls & SC_TitleBarSysMenu && optTitleBar->titleBarFlags & Qt::WindowSystemMenuHint) 
                {
                    QStyleOption tool(0);
                    tool.palette = optTitleBar->palette;

                    QRect ir = proxy()->subControlRect(CC_TitleBar, optTitleBar, SC_TitleBarSysMenu, w);

                    if (!optTitleBar->icon.isNull())
                    {
                    #ifdef Q_OS_WIN
                        if (HICON hIcon = optTitleBar->frameHelper->getWindowIcon(optTitleBar->icon))
                        {
                        #if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
                            QPixmap px = qt_pixmapFromWinHICON(hIcon);
                            if (!px.isNull())
                            {
                                QSize iconSize = px.size();
                                int controlHeight = optTitleBar->rect.height();
                                int hPad = (controlHeight - iconSize.height())/2;
                                int vPad = (controlHeight - iconSize.width())/2;
                                int br = optTitleBar->frameHelper->frameBorder()/2;
                                QRect r = QRect(hPad - br, vPad - br, iconSize.width(), iconSize.height());
                                r = visualRect(optTitleBar->direction, optTitleBar->rect, r);
                                p->drawPixmap(r.left(), r.top(), px);
                            }
                        #else
                            ir.translate(optTitleBar->frameHelper->frameBorder(), optTitleBar->frameHelper->frameBorder());
                            ::DrawIconEx(p->paintEngine()->getDC(), ir.left(), ir.top(), hIcon, 0, 0, 0, Q_NULL, DI_NORMAL | DI_COMPAT);
                        #endif
                        }
                    #else
                        ir.translate(4, 0);
                        optTitleBar->icon.paint(p, ir);
                    #endif // Q_OS_WIN
                    }
                    else
                    {
                        int iconSize = proxy()->pixelMetric(PM_SmallIconSize, optTitleBar, w);
                        QPixmap pm = standardIcon(SP_TitleBarMenuButton, &tool, w).pixmap(iconSize, iconSize);
                        tool.rect = ir;
                        p->save();
                        proxy()->drawItemPixmap(p, ir, Qt::AlignCenter, pm);
                        p->restore();
                    }
                }
            }
            return true;
        }
        else
        {
            QRect rcTitle = optTitleBar->rect;
            rcTitle.setTop(rcTitle.top()-1);

            QPixmap soImage = cached("FrameTopCenter.png");
            if (!soImage.isNull())
            {
                QRect rcSrc = sourceRectImage(soImage.rect(), 0, 2);
                drawImage(soImage, *p, rcTitle, rcSrc, QRect(QPoint(0, 5), QPoint(0, 3)));
            }

            if (optTitleBar->quickAccessOnTop && optTitleBar->quickAccessVisible && optTitleBar->quickAccessVisibleCount > 0)
            {
                if (optTitleBar->existSysButton && !optTitleBar->normalSysButton)
                {
                    QRect rcQuickAccess = optTitleBar->rcQuickAccess;
                    QRect rcQuickAccessArea = rcQuickAccess.adjusted(-16, -1, 4, 0);//(QPoint(rcQuickAccess.left() - 15, 0), QPoint(rcQuickAccess.right(), rcQuickAccess.height()));

                    QPixmap soQuickAccessImage = cached(optTitleBar->frameHelper && optTitleBar->frameHelper->isDwmEnabled()?  "RibbonQuickAccessAreaDwm.png" : "RibbonQuickAccessArea.png" );
                    if (!soQuickAccessImage.isNull())
                        drawImage(soQuickAccessImage, *p, rcQuickAccessArea, soQuickAccessImage.rect(), QRect(QPoint(16, 3), QPoint(16, 3)));
                }
            }

            if (optTitleBar->listContextHeaders && optTitleBar->listContextHeaders->count() > 0)
                proxy()->drawPrimitive((QStyle::PrimitiveElement)RibbonStyle::PE_RibbonContextHeaders, opt, p, w);
            return true;
        }
    }
    return false;
}

/*! \internal */
bool RibbonStyle::drawFrameMenu(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const RibbonStyle);
    return d.ribbonPaintManager()->drawFrameMenu(opt, p, w);
}

/*! \internal */
bool RibbonStyle::drawSlider(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w)  const
{
    QTN_D(const RibbonStyle);
    return d.ribbonPaintManager()->drawSlider(opt, p, w);
}

/*! \internal */
void RibbonStyle::drawSystemButton(const QStyleOption* option, QPainter* p, const QWidget* widget) const
{
    if (paintAnimation(tp_PrimitiveElement, PE_RibbonFileButton, option, p, widget))
        return;

    QTN_D(const RibbonStyle);
    d.ribbonPaintManager()->drawSystemButton(option, p, widget);
}

/*! \internal */
void RibbonStyle::drawOptionButton(const QStyleOption* option, QPainter* p, const QWidget* widget) const
{
    QTN_D(const RibbonStyle);
    d.ribbonPaintManager()->drawOptionButton(option, p, widget);
}

/*! \internal */
void RibbonStyle::drawGroupScrollButton(const QStyleOption* option, QPainter* p, const QWidget* widget) const
{
    Q_UNUSED(widget);
    if (const QStyleOptionToolButton* toolbutton = qstyleoption_cast<const QStyleOptionToolButton*>(option)) 
    {
        QPixmap soRibbonScrollButton = cached( toolbutton->arrowType == Qt::LeftArrow ? 
            "RibbonGroupScrollButtonLeft.png" : "RibbonGroupScrollButtonRight.png");

        if(soRibbonScrollButton.isNull())
            return;

        QRect rc = toolbutton->rect;

        bool selected = option->state & State_MouseOver;
        bool pressed = option->state & State_Sunken;

        int state = pressed ? 2 : selected ? 1 : 0;

        QRect rcSrc = sourceRectImage(soRibbonScrollButton.rect(), state, 3);
        drawImage(soRibbonScrollButton, *p, rc, rcSrc, QRect(QPoint(4, 6), QPoint(3, 6)), QColor(0xFF, 0, 0xFF));

        QPixmap soRibbonScrollGlyph = cached( toolbutton->arrowType == Qt::LeftArrow ? 
            "RibbonGroupScrollLeftGlyph.png" : "RibbonGroupScrollRightGlyph.png");

        rcSrc = sourceRectImage(soRibbonScrollGlyph.rect(), state, 3);
        QSize szGlyph = rcSrc.size();

        QRect rcGlyph(QPoint((rc.right() + rc.left() - szGlyph.width()) * 0.5, (rc.top() + rc.bottom() - szGlyph.height()) *0.5), szGlyph);

        drawImage(soRibbonScrollGlyph, *p, rcGlyph, rcSrc, QRect(QPoint(0, 0), QPoint(0, 0)), QColor(0xFF, 0, 0xFF));
    }
}

/*! \internal */
void RibbonStyle::drawFileButtonLabel(const QStyleOption* option, QPainter* p, const QWidget* widget) const
{
    QTN_D(const RibbonStyle);
    if (const QStyleOptionToolButton* toolbutton = qstyleoption_cast<const QStyleOptionToolButton*>(option)) 
    {
        int alignment = Qt::TextShowMnemonic;
        if (!proxy()->styleHint(SH_UnderlineShortcut, toolbutton, widget))
            alignment |= Qt::TextHideMnemonic;

        alignment |= Qt::AlignCenter;
        alignment |= Qt::TextSingleLine;

        QString text = toolbutton->text;
        if (d.ribbonPaintManager()->isTopLevelMenuItemUpperCase(widget))
            text = text.toUpper();

        proxy()->drawItemText(p, toolbutton->rect, alignment, toolbutton->palette,
            toolbutton->state & State_Enabled, text, QPalette::ButtonText);
    }
}

/*! \internal */
void RibbonStyle::drawPopupSystemButton(const QStyleOption* option, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    if (const QStyleOptionToolButton *toolbutton = qstyleoption_cast<const QStyleOptionToolButton *>(option)) 
    {
        QPixmap soRibbonSystemMenu = cached("RibbonSystemMenuButton.png");
        QRect rcSrc = sourceRectImage(soRibbonSystemMenu.rect(), toolbutton->state & State_MouseOver ? 1 : 0, 2);
        drawImage(soRibbonSystemMenu, *p, toolbutton->rect, rcSrc, QRect(QPoint(2, 2), QPoint(2, 2)), QColor(0xFF, 0, 0xFF));
    }
}

/*! \internal */
void RibbonStyle::drawQuickAccessButton(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
	QTN_D(const RibbonStyle);
	d.ribbonPaintManager()->drawQuickAccessButton(opt, p, w);
}

/*! \internal */
void RibbonStyle::drawPopupResizeGripper(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const RibbonStyle);
    d.ribbonPaintManager()->drawPopupResizeGripper(opt, p, w);
}

/*! \internal */
bool RibbonStyle::drawMenuItem(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    QTN_D(const RibbonStyle)
    if (qobject_cast<const RibbonPageSystemPopup*>(widget))
    {
        if (const QStyleOptionMenuItem* menuitem = qstyleoption_cast<const QStyleOptionMenuItem*>(opt)) 
        {
            if (menuitem->text.count(QString(qtn_PopupLable)) > 0)
            {
                p->fillRect(menuitem->rect.adjusted(-1, 0, 2, 0), d.m_clrControlGalleryLabel);
                p->fillRect(menuitem->rect.left(), menuitem->rect.bottom()-1, menuitem->rect.width(), 1, QColor(197, 197, 197));

                QFont fontSave = p->font();

                QFont font = fontSave;
                font.setBold(true);
                p->setFont(font);

                QRect rcText(menuitem->rect);
                rcText.adjust(7, 0, 0, 0);

                QPalette pal = menuitem->palette;
                pal.setColor(QPalette::WindowText, d.m_clrGroupClientText);

                int flags = 0;
                flags |= Qt::TextHideMnemonic | Qt::AlignVCenter | Qt::TextSingleLine;

                QString text = menuitem->text;
                text = p->fontMetrics().elidedText(text.remove(QString(qtn_PopupLable)), Qt::ElideRight, rcText.adjusted(2, 0, -2, 0).width());

                // draw text
                proxy()->drawItemText(p, rcText, flags, pal, opt->state & State_Enabled, text, QPalette::WindowText);
                p->setFont(fontSave);

                return true;
            }
            else
            {
                int x, y, w, h;
                menuitem->rect.getRect(&x, &y, &w, &h);
                int tab = menuitem->tabWidth;
                bool dis = !(menuitem->state & State_Enabled);
                bool checked = menuitem->checkType != QStyleOptionMenuItem::NotCheckable ? menuitem->checked : false;
                bool act = menuitem->state & State_Selected;

                // windows always has a check column, regardless whether we have an icon or not
                const int nIconSize = pixelMetric(PM_ToolBarIconSize, opt, widget);
                int checkcol = nIconSize;

                if (act)
                    drawRectangle(p, menuitem->rect, true/*selected*/, false/*pressed*/, !dis/*enabled*/, 
                        false/*checked*/, false/*popuped*/, TypePopup, BarPopup);

                if (menuitem->menuItemType == QStyleOptionMenuItem::Separator)
                {
                    int yoff = (y-1 + h / 2);
                    int xIcon = nIconSize;    
                    if(qobject_cast<const OfficePopupMenu*>(widget))
                        xIcon = 0;
                    p->setPen(menuitem->palette.dark().color());
                    p->drawLine(x + 2+/*menuitem->maxIconWidth*/xIcon, yoff, x + w - 4, yoff);
                    p->setPen(menuitem->palette.light().color());
                    p->drawLine(x + 2+/*menuitem->maxIconWidth*/xIcon, yoff + 1, x + w - 4, yoff + 1);
                    return true;
                }

                QRect vCheckRect = visualRect(opt->direction, menuitem->rect, QRect(menuitem->rect.x(), menuitem->rect.y(), checkcol, menuitem->rect.height()));

                // On Windows Style, if we have a checkable item and an icon we
                // draw the icon recessed to indicate an item is checked. If we
                // have no icon, we draw a checkmark instead.
                if (!menuitem->icon.isNull()) 
                {
                    QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;

                    int iconSize = pixelMetric(PM_LargeIconSize, opt, widget);
                    if (iconSize > qMin(opt->rect.height(), opt->rect.width()))
                        iconSize = pixelMetric(PM_SmallIconSize, opt, widget);

                    if ( act && !dis )
                        mode = QIcon::Active;
                    QPixmap pixmap;
                    if ( checked )
                        //                pixmap = menuitem->icon.pixmap(pixelMetric(PM_LargeIconSize, opt, widget), mode, QIcon::On);
                        pixmap = menuitem->icon.pixmap(iconSize, mode, QIcon::On);
                    else
                        //                pixmap = menuitem->icon.pixmap(pixelMetric(PM_LargeIconSize, opt, widget), mode);
                        pixmap = menuitem->icon.pixmap(iconSize, mode);

                    int pixw = pixmap.width();
                    int pixh = pixmap.height();

                    QRect pmr(0, 0, pixw, pixh);
                    pmr.moveCenter(vCheckRect.center());
                    p->setPen(menuitem->palette.text().color());

                    if (checked)
                    {
                        QRect vIconRect = visualRect(opt->direction, opt->rect, pmr);
                        QRect rcChecked = menuitem->rect;
                        rcChecked.setLeft(vIconRect.left());
                        rcChecked.setWidth(vIconRect.width());
                        drawRectangle(p, rcChecked.adjusted(-2, 1, 2, -1), false/*selected*/, true/*pressed*/, !dis/*enabled*/, 
                            true/*checked*/, false/*popuped*/, TypePopup, BarPopup);
                    }

                    p->drawPixmap(pmr.topLeft(), pixmap);
                } 

                int xm = windowsItemFrame + checkcol + windowsItemHMargin;
                int xpos = menuitem->rect.x() + xm;

                // draw text    
                if (!menuitem->text.isEmpty()) 
                { 
                    int height = menuitem->fontMetrics.height();
                    QRect textRect(xpos, y + windowsItemVMargin + 2, w - xm - windowsRightBorder - tab + 1, height);
                    QRect vTextRect = visualRect(opt->direction, menuitem->rect, textRect);

                    p->save();
                    QStringList split = menuitem->text.split(QString("\n"));
                    QString s = split.count() > 0 ? split.at(0) : menuitem->text;

                    QFont font = menuitem->font;
                    font.setBold(true);
                    p->setFont(font);

                    int t = s.indexOf(QLatin1Char('\t'));
                    int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;

                    if (!styleHint(SH_UnderlineShortcut, menuitem, widget))
                        text_flags |= Qt::TextHideMnemonic;
                    text_flags |= Qt::AlignLeft;

                    // draw hotkeys
                    if (t >= 0) 
                    {
                        QRect vShortcutRect = visualRect( opt->direction, menuitem->rect, QRect(textRect.topRight(), 
                            QPoint(menuitem->rect.right(), textRect.bottom())));

                        p->setPen(opt->state & State_Enabled ? d.m_clrMenuPopupText : d.m_clrMenuBarGrayText);
                        p->drawText(vShortcutRect, text_flags, s.mid(t + 1));
                        s = s.left(t);
                    }

                    p->setPen(opt->state & State_Enabled ? d.m_clrMenuPopupText : d.m_clrMenuBarGrayText);
                    
//                    p->drawRect(vTextRect);
                    p->drawText(vTextRect, text_flags, s.left(t));
                    p->restore();
                    if (split.count() > 1)
                    {
                        text_flags |= Qt::TextWordWrap;
                        QRect textTipRect(xpos, y + vTextRect.height(), w - xm - 1, opt->rect.height());
                        QRect rc = menuitem->fontMetrics.boundingRect(textTipRect, text_flags, split.at(1));
                        QRect vTextTipRect = visualRect(opt->direction, menuitem->rect, rc);
                        vTextTipRect.setTop(y + vTextRect.height());
                        vTextTipRect.setWidth(textTipRect.width());
//                        p->drawRect(vTextTipRect);
                        p->setPen(opt->state & State_Enabled ? d.m_clrMenuPopupText : d.m_clrMenuBarGrayText);
                        p->drawText(vTextTipRect, text_flags, split.at(1));
                    }
                }

                return true;
            }
        }
    }

    if (qobject_cast<const RibbonBackstageView*>(widget))
    {
        d.ribbonPaintManager()->drawRibbonBackstageMenu(opt, p, widget);
        return true;
    }
    else
        return OfficeStyle::drawMenuItem(opt, p, widget);
}

/*! \internal */
void RibbonStyle::drawTabShape(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    if (paintAnimation(tp_PrimitiveElement, PE_RibbonTab, opt, p, widget))
        return;

    QTN_D(const RibbonStyle);
    d.ribbonPaintManager()->drawTabShape(opt, p, widget);
}

/*! \internal */
void RibbonStyle::drawTabShapeLabel(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const RibbonStyle);
    d.ribbonPaintManager()->drawTabShapeLabel(opt, p, w);
}

/*! \internal */
bool RibbonStyle::drawTabBarTabShape(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    QTN_D(const RibbonStyle);
    return d.ribbonPaintManager()->drawTabBarTabShape(opt, p, widget);
}

/*! \internal */
bool RibbonStyle::drawTabBarTabLabel(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    QTN_D(const RibbonStyle);

    if (!qobject_cast<QMdiArea*>(widget->parentWidget()))
        return OfficeStyle::drawTabBarTabLabel(opt, p, widget);

    if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab*>(opt))
    {
        QStyleOptionTabV3 tabV2(*tab);
        QRect tr = tabV2.rect;
        bool verticalTabs = tabV2.shape == QTabBar::RoundedEast
            || tabV2.shape == QTabBar::RoundedWest
            || tabV2.shape == QTabBar::TriangularEast
            || tabV2.shape == QTabBar::TriangularWest;

        int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
        if (!proxy()->styleHint(SH_UnderlineShortcut, opt, widget))
            alignment |= Qt::TextHideMnemonic;

        if (verticalTabs) 
        {
            p->save();
            int newX, newY, newRot;
            if (tabV2.shape == QTabBar::RoundedEast || tabV2.shape == QTabBar::TriangularEast)
            {
                newX = tr.width() + tr.x();
                newY = tr.y();
                newRot = 90;
            } 
            else 
            {
                newX = tr.x();
                newY = tr.y() + tr.height();
                newRot = -90;
            }
            QTransform m = QTransform::fromTranslate(newX, newY);
            m.rotate(newRot);
            p->setTransform(m, true);
        }
        QRect iconRect;
        d.tabLayout(&tabV2, widget, &tr, &iconRect);
        //we compute tr twice because the style may override subElementRect
        tr = proxy()->subElementRect(SE_TabBarTabText, opt, widget); 

        if (!tabV2.icon.isNull()) 
        {
            QPixmap tabIcon = tabV2.icon.pixmap(tabV2.iconSize,
                (tabV2.state & State_Enabled) ? QIcon::Normal
                : QIcon::Disabled,
                (tabV2.state & State_Selected) ? QIcon::On
                : QIcon::Off);
            p->drawPixmap(iconRect.x(), iconRect.y(), tabIcon);
        }
/*
        QFont fn = p->font();
        if (tabV2.state & State_Selected)
        {
            fn.setBold(true);
            p->setFont(fn);
        }
*/
        proxy()->drawItemText(p, tr, alignment, tab->palette, tab->state & State_Enabled, tab->text, QPalette::WindowText);

//        p->setFont(fn);

        if (verticalTabs)
            p->restore();

        if (tabV2.state & State_HasFocus) 
        {
            const int OFFSET = 1 + pixelMetric(PM_DefaultFrameWidth);

            int x1, x2;
            x1 = tabV2.rect.left();
            x2 = tabV2.rect.right() - 1;

            QStyleOptionFocusRect fropt;
            fropt.QStyleOption::operator=(*tab);
            fropt.rect.setRect(x1 + 1 + OFFSET, tabV2.rect.y() + OFFSET,
                x2 - x1 - 2*OFFSET, tabV2.rect.height() - 2*OFFSET);
            drawPrimitive(PE_FrameFocusRect, &fropt, p, widget);
        }
        return true;
    }
    return false;
}

/*! \internal */
bool RibbonStyle::drawPanelButtonTool(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const RibbonStyle);
    return d.ribbonPaintManager()->drawPanelButtonTool(opt, p, w);
}

/*! \internal */
void RibbonStyle::calcRects(const QRect& rc, const QSize& pmSize, const QFontMetrics& fMetric, const QString& text, 
    bool hasMenu, QRect* pmRect, QRect* strFirsRect, QRect* strSecondRect) const
{
    QRect rect(QPoint(0,0), pmSize);
    *pmRect = rect;

    QString strFirstRow, strSecondRow;
    splitString(text, strFirstRow, strSecondRow);

    if (!strFirstRow.isEmpty())
    {
        QSize size = fMetric.size(Qt::TextSingleLine, strFirstRow);
        if (!size.isEmpty())
        {
            if (hasMenu && strSecondRow.isEmpty())
                size.setWidth(size.width() + 4 + 5);

            *strFirsRect = QRect(QPoint(0, rect.bottom() + 2), size);
            rect = rect.united(*strFirsRect);
        }
    }
    if (!strSecondRow.isEmpty())
    {
        QSize size = fMetric.size(Qt::TextSingleLine, strSecondRow);
        if (!size.isEmpty())
        {
            if (hasMenu)
                size.setWidth(size.width() + 4 + 5);
            *strSecondRect = QRect(QPoint(0, rect.bottom() + 2), size);
            rect = rect.united(*strSecondRect);
        }
    }

    int dx = (rc.width()-rect.width())/2;
    int dy = (rc.height()-rect.height())/2;

    if (!pmRect->isEmpty())
    {
        pmRect->translate(dx, dy);
        pmRect->translate((rect.width()-pmRect->width())/2, 0);
    }

    if (!strFirsRect->isEmpty())
    {
        strFirsRect->translate(dx, dy);
        strFirsRect->translate((rect.width()-strFirsRect->width())/2, 0);
    }

    if (!strSecondRect->isEmpty())
    {
        strSecondRect->translate(dx, dy);
        strSecondRect->translate((rect.width()-strSecondRect->width())/2, 0);
    }
}

/*! \internal */
bool RibbonStyle::drawToolButtonLabel(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    if (qobject_cast<const RibbonBackstageButton*>(w))
    {
        if (const StyleOptionBackstageButton* toolbutton = qstyleoption_cast<const StyleOptionBackstageButton*>(opt))
        {
            QRect rect = toolbutton->rect;
            QPalette pal = toolbutton->state & State_Enabled ? QPalette() : toolbutton->palette;

            // Arrow type always overrules and is always shown
            bool hasArrow = toolbutton->features & QStyleOptionToolButton::Arrow;
            if (((!hasArrow && toolbutton->icon.isNull()) && !toolbutton->text.isEmpty())|| toolbutton->toolButtonStyle == Qt::ToolButtonTextOnly) 
            {
                int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
                if (!styleHint(SH_UnderlineShortcut, opt, w))
                    alignment |= Qt::TextHideMnemonic;

                proxy()->drawItemText(p, rect, alignment, /*toolbutton->palette*/pal, /*opt->state & State_Enabled*/true, 
                    toolbutton->text, QPalette::ButtonText);
            }
            else
            {
                QPixmap pm;
                QSize pmSize = toolbutton->iconSize;
                if (pmSize.width() > qMin(rect.width(), rect.height()))
                {
                    const int iconExtent = proxy()->pixelMetric(PM_SmallIconSize);
                    pmSize = QSize(iconExtent, iconExtent);
                }

                if (!toolbutton->icon.isNull())
                {
                    QIcon::State state = toolbutton->state & State_On ? QIcon::On : QIcon::Off;
                    QIcon::Mode mode;
                    if (!(toolbutton->state & State_Enabled))
                        mode = QIcon::Disabled;
                    else if ((opt->state & State_MouseOver) && (opt->state & State_AutoRaise))
                        mode = QIcon::Active;
                    else
                        mode = QIcon::Normal;
                    pm = toolbutton->icon.pixmap(toolbutton->rect.size().boundedTo(pmSize), mode, state);
                    pmSize = pm.size();
                }

                if (toolbutton->toolButtonStyle != Qt::ToolButtonIconOnly)
                {
                    int alignment = Qt::TextShowMnemonic;
                    if (!proxy()->styleHint(SH_UnderlineShortcut, opt, w))
                        alignment |= Qt::TextHideMnemonic;

                    p->setFont(toolbutton->font);

                    if (toolbutton->toolButtonStyle == Qt::ToolButtonTextUnderIcon)
                    {
                        QRect pmRect, strFirsRect, strSecondRect;
                        calcRects(rect, pmSize, toolbutton->fontMetrics, toolbutton->text, 
                            toolbutton->features & QStyleOptionToolButton::HasMenu, &pmRect, &strFirsRect, &strSecondRect);
                    
                        if (!hasArrow)
                            drawItemPixmap(p, QStyle::visualRect(opt->direction, rect, pmRect), Qt::AlignHCenter, pm);
    //                    else 
    //                        drawArrow(this, toolbutton, pr, p, w);

                        alignment |= Qt::AlignCenter;

                        QString strFirstRow, strSecondRow;
                        splitString(toolbutton->text, strFirstRow, strSecondRow);

                        if (!strFirstRow.isEmpty())
                        {
                            if (toolbutton->features & QStyleOptionToolButton::HasMenu && strSecondRow.isEmpty())
                            {
                                alignment &= ~Qt::AlignCenter;
                                alignment |= Qt::AlignVCenter | Qt::AlignLeft;
                            }
                            proxy()->drawItemText(p, strFirsRect, alignment, /*toolbutton->palette*/pal,
                                /*toolbutton->state & State_Enabled*/true, strFirstRow, QPalette::ButtonText);
                        }

                        if (!strSecondRow.isEmpty()) 
                        {
                            if (toolbutton->features & QStyleOptionToolButton::HasMenu)
                            {
                                alignment &= ~Qt::AlignCenter;
                                alignment |= Qt::AlignVCenter | Qt::AlignLeft;
                            }
                            proxy()->drawItemText(p, strSecondRect, alignment, /*toolbutton->palette*/pal,
                                /*toolbutton->state & State_Enabled*/true, strSecondRow, QPalette::ButtonText);
                        }
                    }
                    else
                    {
                        QRect pmRect(QPoint(4, (rect.height() - pmSize.height())/2), pmSize);
                        if (!hasArrow) 
                            drawItemPixmap(p, QStyle::visualRect(opt->direction, rect, pmRect), Qt::AlignCenter, pm);
//                        else 
//                            drawArrow(this, toolbutton, pr, p, w);
                        alignment |= Qt::AlignLeft | Qt::AlignVCenter;

                        QSize sizeText = toolbutton->fontMetrics.size(Qt::TextSingleLine, toolbutton->text);
                        QRect tcText(QPoint(pmRect.right() + 4, (rect.height() - sizeText.height())/2), sizeText);
                        proxy()->drawItemText(p, tcText, alignment, /*toolbutton->palette*/pal,
                            /*toolbutton->state & State_Enabled*/true, toolbutton->text, QPalette::ButtonText);
                    }
                }
                else
                {
//                    if (hasArrow) 
//                        drawArrow(this, toolbutton, rect, p, w);
//                    else 
                    {
                        QRect pr = rect;
                        if ((toolbutton->subControls & SC_ToolButtonMenu) || (toolbutton->features & QStyleOptionToolButton::HasMenu))
                        {
                            int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, opt, w);
                            pr.setWidth(pr.width()-mbi);
                        }
                        drawItemPixmap(p, pr, Qt::AlignCenter, pm);
                    }
                }
            }
        }
        return true;
    }
    else
    {
        return OfficeStyle::drawToolButtonLabel(opt, p, w);
    }
}

/*! \internal */
bool RibbonStyle::drawToolButton(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    if (qobject_cast<const RibbonBackstageButton*>(w))
    {
        if (const QStyleOptionToolButton* toolbutton = qstyleoption_cast<const QStyleOptionToolButton*>(opt))
        {
            QRect button, menuarea;
            button = proxy()->subControlRect(CC_ToolButton, toolbutton, SC_ToolButton, w);
            menuarea = proxy()->subControlRect(CC_ToolButton, toolbutton, SC_ToolButtonMenu, w);

            State flags = toolbutton->state & ~State_Sunken;

            if (flags & State_AutoRaise) 
            {
                if (!(flags & State_MouseOver) || !(flags & State_Enabled)) 
                    flags &= ~State_Raised;
            }
            State mflags = flags;
            if (toolbutton->state & State_Sunken) 
            {
                if (toolbutton->activeSubControls & SC_ToolButton)
                    flags |= State_Sunken;
                mflags |= State_Sunken;
            }

            RibbonStyle* rs = (RibbonStyle*)(this);
            bool animation = isAnimationEnabled();
            rs->setAnimationEnabled(false);
            proxy()->drawPrimitive(PE_PanelButtonTool, toolbutton, p, w);
            rs->setAnimationEnabled(animation);

            QStyleOptionToolButton label = *toolbutton;
            label.state = flags;
            proxy()->drawControl(CE_ToolButtonLabel, &label, p, w);

            QPixmap pm;
            QSize pmSize = toolbutton->iconSize;
            if (pmSize.width() > qMin(toolbutton->rect.width(), toolbutton->rect.height()))
            {
                const int iconExtent = proxy()->pixelMetric(PM_SmallIconSize);
                pmSize = QSize(iconExtent, iconExtent);
            }

            if (!toolbutton->icon.isNull())
            {
                QIcon::State state = toolbutton->state & State_On ? QIcon::On : QIcon::Off;
                QIcon::Mode mode;
                if (!(toolbutton->state & State_Enabled))
                    mode = QIcon::Disabled;
                else if ((opt->state & State_MouseOver) && (opt->state & State_AutoRaise))
                    mode = QIcon::Active;
                else
                    mode = QIcon::Normal;
                pm = toolbutton->icon.pixmap(toolbutton->rect.size().boundedTo(pmSize), mode, state);
                pmSize = pm.size();
            }

            if (toolbutton->subControls & SC_ToolButtonMenu) 
            {
                QStyleOption tool(0);
                tool.palette = toolbutton->palette;

                QRect ir = menuarea, rcArrow;

                if (toolbutton->toolButtonStyle == Qt::ToolButtonTextUnderIcon)
                {
                    QString strFirstRow, strSecondRow;
                    splitString(toolbutton->text, strFirstRow, strSecondRow);
                    rcArrow = QRect(QPoint(strSecondRow.isEmpty() ? opt->rect.width()/2 - 2 : opt->rect.right()-7, opt->rect.bottom()-8), QSize(5, 4));
                }
                else
                    rcArrow = QRect(QPoint((ir.left() + ir.right()-6) / 2, (ir.top() + ir.bottom()-5) / 2), QSize(5, 4));

                tool.rect = rcArrow;
                tool.state = mflags;

                proxy()->drawPrimitive(PE_IndicatorArrowDown, &tool, p, w);
            } 
            else if (toolbutton->features & QStyleOptionToolButton::HasMenu) 
            {
                QStyleOptionToolButton newBtn = *toolbutton;
                if (toolbutton->toolButtonStyle == Qt::ToolButtonTextUnderIcon)
                {
                    QRect pmRect, strFirsRect, strSecondRect;
                    calcRects(toolbutton->rect, pmSize, toolbutton->fontMetrics, toolbutton->text, 
                        true, &pmRect, &strFirsRect, &strSecondRect);

                    QRect r = strSecondRect.isEmpty() ? strFirsRect : strSecondRect;
                    newBtn.rect = QRect(QPoint(r.right() - 5, r.top() + (r.height())/2), QSize(5, 5));
                }
                else
                {
                    QRect ir = menuarea;
                    int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, toolbutton, w);
                    newBtn.rect = QRect(QPoint(ir.right() + 4 - mbi, ir.y() + ir.height() - mbi), QSize(mbi - 5, mbi - 5));
                }
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &newBtn, p, w);
            }
        }
        return true;
    }
    else
    {
        return OfficeStyle::drawToolButton(opt, p, w);
    }
}

/*! \internal */
bool RibbonStyle::drawToolBar(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const RibbonStyle);
    return d.ribbonPaintManager()->drawToolBar(opt, p, w);
}

/*! \internal */
bool RibbonStyle::drawGroupControlEntry(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    if (const RibbonStatusBarSwitchGroup* switchGroup = qobject_cast<const RibbonStatusBarSwitchGroup*>(widget ? widget->parentWidget() : Q_NULL))
    {
        int index = widget->property(qtn_Prop_Index_Group).toInt();
        int count = switchGroup->property(qtn_Prop_Switch_Count).toInt();

        bool enabled  = opt->state & State_Enabled;
        bool checked  = opt->state & State_On;
        bool selected = opt->state & State_MouseOver;
        bool pressed  = opt->state & State_Sunken;

        QPixmap soImage = cached("StatusBarSwitchGroup.png");
        if (!soImage.isNull())
        {
            int state = !enabled ? 5 : pressed ? 2 : checked ? (selected ? 4 : 3) : selected ? 1 : 0;
            QRect rcSrc = sourceRectImage(soImage.rect(), state, 6);
            int srcWidth = rcSrc.width() / 4;
            int image = count == 1 ? 3 : index == 0 ? 0 : index == count - 1 ? 2 : 1;

            rcSrc.setLeft(srcWidth * image);
            rcSrc.setRight(rcSrc.left() + srcWidth - 1);
            drawImage(soImage, *p, opt->rect, rcSrc, QRect(QPoint(2, 2), QPoint(2, 2)), QColor(0xFF, 0, 0xFF));
            return true;
        }
    }

    if (widget && /*rg->isControlsGrouping()*/widget->property(qtn_Prop_Group).toBool())
    {
        if (const QStyleOptionToolButton*toolbutton = qstyleoption_cast<const QStyleOptionToolButton*>(opt))
        {
            if (toolbutton->subControls & SC_ToolButtonMenu)
                return false;
        }

        const RibbonGroup * rg = qobject_cast<const RibbonGroup*>(widget ? widget->parentWidget() : Q_NULL);
        if (!rg)
            return false;

        bool beginGroup = widget->property(qtn_Prop_Begin_Group).toBool();
        bool wrap       = widget->property(qtn_Prop_Wrap).toBool();
        bool left       = beginGroup || wrap;

        int indexNext = rg->qtn_d().getNextIndex(widget);
        QWidget* nextWidget = rg->qtn_d().getWidget(indexNext);
        bool beginGroupNext = nextWidget->property(qtn_Prop_Begin_Group).toBool();
        bool wrapNext       = nextWidget->property(qtn_Prop_Wrap).toBool();

        bool right = indexNext <= rg->qtn_d().getIndexWidget(widget)|| beginGroupNext || wrapNext;

        QPixmap soImage = left && right  ? cached("ToolbarButtonsSpecialSingle.png") :
                                   left  ? cached("ToolbarButtonsSpecialLeft.png")   :
                                   right ? cached("ToolbarButtonsSpecialRight.png")  :
                                           cached("ToolbarButtonsSpecialCenter.png");
        
        if(soImage.isNull())
            return false;

        bool enabled  = opt->state & State_Enabled;
        bool checked  = opt->state & State_On;
        bool selected = opt->state & State_MouseOver;
        bool pressed  = opt->state & State_Sunken;
        bool popuped  = false;
        if (const QStyleOptionToolButton *toolbutton= qstyleoption_cast<const QStyleOptionToolButton*>(opt)) 
            popuped  = (toolbutton->activeSubControls & QStyle::SC_ToolButtonMenu) && (opt->state & State_Sunken);

        int state = 0;
        if (!enabled)
        {
//          if (isKeyboardSelected(selected)) state = 1;
        }

        else if (popuped) state = 2 + 1;
        else if (checked && !selected && !pressed) state = 3;
        else if (checked && selected && !pressed) state = 4;
        else if (/*isKeyboardSelected(pressed) ||*/ (selected && pressed)) state = 2;
        else if (selected || pressed) state = 1;

        drawImage(soImage, *p, opt->rect, sourceRectImage(soImage.rect(), state, 5), 
            QRect(QPoint(2, 2), QPoint(2, 2)), QColor(0xFF, 0, 0xFF));
        return true;
    }
    return false;
}

/*! \internal */
bool RibbonStyle::drawIndicatorArrow(PrimitiveElement pe, const QStyleOption* opt, QPainter* p, const QWidget* w) const 
{ 
    QTN_D(const RibbonStyle);
    return d.ribbonPaintManager()->drawIndicatorArrow(pe, opt, p, w);
}

/*! \internal */
void RibbonStyle::drawRectangle(QPainter* p, const QRect& rect, bool selected, bool pressed, bool enabled, bool checked, bool popuped,
                                BarType barType, BarPosition barPos) const
{
    QTN_D(const RibbonStyle);
    d.ribbonPaintManager()->drawRectangle(p, rect, selected, pressed, enabled, checked, popuped, barType, barPos);
}

/*! \internal */
void RibbonStyle::drawSplitButtonPopup(QPainter* p, const QRect& rect, bool selected, bool enabled, bool popuped) const
{
    if (rect.height() > 33 && enabled)
    {
        QPixmap pixmap = cached("MenuSplitItem.png");
        if (!pixmap.isNull())
        {
            int state = !selected ? 0 : popuped ? 2 : 1;
            QRect rcButton(rect.left(), rect.top(), rect.width() - splitActionPopupWidth, rect.height());
            drawImage(pixmap, *p, rcButton, sourceRectImage(pixmap.rect(), state, 3), QRect(QPoint(2, 2), QPoint(2, 2)), QColor(0xFF, 0, 0xFF));
        }

        pixmap = cached("MenuSplitDropDown.png");

        int state = !selected || !enabled ? 0 : 2;

        QRect rcButton(rect.right() - splitActionPopupWidth, rect.top(), splitActionPopupWidth, rect.height());
        drawImage(pixmap, *p, rcButton, sourceRectImage(pixmap.rect(), state, 3), QRect(QPoint(2, 2), QPoint(2, 2)), QColor(0xFF, 0, 0xFF));
    }
}

/*! \internal */
void RibbonStyle::drawItemText(QPainter* painter, const QRect& rect, int alignment, const QPalette& pal, bool enabled,
                               const QString& text, QPalette::ColorRole textRole) const
{
    if (text.isEmpty())
        return;

    QPen savedPen;
    if (textRole != QPalette::NoRole) 
    {
        savedPen = painter->pen();
        painter->setPen(pal.color(textRole));
    }
    if (!enabled) 
    {
        QPen pen = painter->pen();
        painter->setPen(pal.light().color());
        //painter->drawText(rect.adjusted(1, 1, 1, 1), alignment, text);
        painter->setPen(pen);
    }

    painter->drawText(rect, alignment, text);

    if (textRole != QPalette::NoRole)
        painter->setPen(savedPen);
}

/*! \internal */
void RibbonStyle::drawFillRect(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    QTN_D(const RibbonStyle)
    if (qobject_cast<const QMenu*>(widget ? widget->parentWidget() : Q_NULL))
    {
        p->fillRect(opt->rect, d.m_clrControlGalleryMenuBk);
        return;
    }

    bool enabled  = opt->state & State_Enabled;
    bool selected = opt->state & State_MouseOver;
    p->fillRect(opt->rect, selected && enabled ? d.m_clrControlGallerySelected : d.m_clrControlGalleryNormal);
}

/*! \internal */
void RibbonStyle::drawRect(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    Q_UNUSED(widget)
    QTN_D(const RibbonStyle)

    QPen savePen = p->pen();
    p->setPen(d.m_clrControlGalleryBorder);
    p->drawRect(opt->rect);
    p->setPen(savePen);
}

/*! \internal */
void RibbonStyle::drawKeyTip(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    QTN_D(const RibbonStyle);
    d.ribbonPaintManager()->drawKeyTip(opt, p, widget);
}

/*! \internal */
void RibbonStyle::drawBackstage(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    QTN_D(const RibbonStyle);
    d.ribbonPaintManager()->drawBackstage(opt, p, widget);
}

/*! \internal */
void RibbonStyle::drawRibbonBackstageCloseButton(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const RibbonStyle);
    d.ribbonPaintManager()->drawRibbonBackstageCloseButton(opt, p, w);
}

/*! \internal */
void RibbonStyle::drawRibbonSliderButton(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const RibbonStyle);
    d.ribbonPaintManager()->drawRibbonSliderButton(opt, p, w);
}

template<class T>
static QWidget* getPrevParentWidget(QWidget* pWidget)
{
    if (qobject_cast<T*>(pWidget))
        return Q_NULL;

    QWidget* pPrevWidget = pWidget;
    while (pWidget)
    {
        pWidget = pWidget->parentWidget();
        if (qobject_cast<T*>(pWidget))
            return pPrevWidget;
        else
            pPrevWidget = pWidget;
    }
    return Q_NULL;
}

/*! \internal */
bool RibbonStyle::showToolTip(const QPoint& pos, QWidget* w)
{
    Q_UNUSED(pos);

    if ((qobject_cast<QToolButton*>(w) && getParentWidget<RibbonSystemPopupBar>(w)) ||
        qobject_cast<RibbonBackstageButton*>(w))
        return true;

    if (getParentWidget<RibbonBar>(w))
    {
        QPoint p = pos;
        p += QPoint(2,
        #ifdef Q_WS_WIN
            21
        #else
            16
        #endif
            );

        int posX = p.x();
        int posY = p.y();

        if (const RibbonGroup* group = getParentWidget<RibbonGroup>(w))
        {
            Q_UNUSED(group);
            posX = w->mapToGlobal(w->rect().topLeft()).x();

            if (QWidget* prevWidget = getPrevParentWidget<RibbonGroup>(w))
            {
                if (RibbonGallery* gallery = qobject_cast<RibbonGallery*>(w))
                {
                    QRect rect = gallery->getDrawItemRect(gallery->getSelectedItem());
                    posX = prevWidget->mapToGlobal(rect.topLeft()).x();
                }
                else
                    posX = prevWidget->mapToGlobal(prevWidget->rect().topLeft()).x();
            }

			// BEGIN ROBLOX CHANGES
			if (group->isReduced())
			{
				posY = group->mapToGlobal(group->rect().bottomRight()).y() + 2;
			}
			else
			{
				if (const RibbonPage* page = getParentWidget<RibbonPage>(w))
					posY = page->mapToGlobal(page->rect().bottomRight()).y() + 2;
			}

			//if (const RibbonPage* page = getParentWidget<RibbonPage>(w))
			//		posY = page->mapToGlobal(page->rect().bottomRight()).y() + 2;

			// END ROBLOX CHANGES
        }

        QIcon icon;
        QString strTitleText;
        QString strTipText = w->toolTip();

        if (RibbonGallery* gallery = qobject_cast<RibbonGallery*>(w))
        {
            if (RibbonGalleryItem* item = gallery->getItem(gallery->getSelectedItem()))
            {
                QString str = item->toolTip();
                if (strTipText != str)
                    strTipText = str;
            }
        }
        else if (strTipText.isEmpty())
        {
            QWidget* widget = w->parentWidget();

            if (!qobject_cast<RibbonGroup*>(widget))
            {
                if (RibbonPageSystemRecentFileList* recentFile = qobject_cast<RibbonPageSystemRecentFileList*>(w))
                {
                    if (QAction* currentAction = recentFile->getCurrentAction())
                        strTipText = currentAction->data().toString();
                }
                else
                {
                    while (widget)
                    {
                        strTipText = widget->toolTip();
                        if (!strTipText.isEmpty())
                            break;
                        widget = widget->parentWidget();
                        if (qobject_cast<RibbonGroup*>(widget))
                            break;
                    }
                }
            }
        }
        else if (QToolButton* button = qobject_cast<QToolButton*>(w))
        {
            if (!strTipText.isEmpty())
                strTipText.remove(QChar('&'));

            icon = button->icon();
            QString str = button->text();
            str.remove(QChar('&'));
            if (strTipText != str)
                strTitleText = str;

            if (QAction * defAction = button->defaultAction())
            {
                QString strShortcuts;
                QKeySequence keySequence;
                QList<QKeySequence> lstShortcuts = defAction->shortcuts();

                foreach(keySequence, lstShortcuts)
                {
                    QString strShortcutString = keySequence.toString(QKeySequence::NativeText);
                    if ( strShortcuts != "" )
                        strShortcuts += ", ";
                    strShortcuts += strShortcutString;
                }
                if ( strShortcuts != "" )
                {
                    if ( strTitleText != "" )
                        strTitleText += " (" + strShortcuts + ")";
                    else
                        strTipText += " (" + strShortcuts + ")";
                }
            }

        }

        if (icon.isNull())
        {
            if (const ExWidgetWrapper* wrapper = getParentWidget<ExWidgetWrapper>(w))
                icon = wrapper->icon();
        }

        QPoint globalPos(posX, posY);
        RibbonToolTip::showToolTip(globalPos, strTitleText, strTipText, icon, w);
        return true;
    }
    return false;
}

static RibbonBar* findMainWindow()
{
    foreach (QWidget *widget, qApp->topLevelWidgets()) 
    {
        if (qobject_cast<QMainWindow *>(widget))
        {
            QList<RibbonBar*> l = widget->findChildren<RibbonBar*>();
            if (l.size() > 0)
                return l.at(0);
        }
    }
    return Q_NULL;
}

/*! \reimp */
bool RibbonStyle::eventFilter(QObject* watched, QEvent* event)
{
    QTN_D(RibbonStyle);
    switch (event->type())
    {
        case QEvent::HoverMove :
        case QEvent::HoverEnter :
        case QEvent::HoverLeave :
            if (QTabBar* tabBar = qobject_cast<QTabBar*>(watched))
            {
                if (qobject_cast<QMdiArea*>(tabBar->parentWidget()))
                {
                    QTN_D(RibbonStyle);
                    d.tabHoverUpdate(tabBar, event);
                }
            }
            break;
        case QEvent::KeyPress:
            if (QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event))
            {
                d.m_blockKeyTips = false;
                d.m_blockKeyTips = (!keyEvent->modifiers() || 
                    (keyEvent->modifiers()&(Qt::MetaModifier|Qt::AltModifier))) && keyEvent->text().length() == 1;

#ifdef Q_OS_WIN
                if (!d.m_blockKeyTips)
                {
                    quint32 virtualKey = keyEvent->nativeVirtualKey();
                    if (keyEvent->key() != Qt::Key_Alt && ::MapVirtualKey(virtualKey, 2) == 0)
                        d.m_blockKeyTips = true;
                }
#endif //Q_OS_WIN

                if (keyEvent->key() == Qt::Key_Alt) 
                {
                    d.m_completeKey = static_cast<QKeyEvent*>(event)->text().isEmpty();
                    if (QMainWindow* widget = qobject_cast<QMainWindow*>(watched))
                    {
                        QList<RibbonBar*> l = widget->findChildren<RibbonBar*>();
                        for (int i = 0, count = l.count(); i < count; ++i)
                        {
                            const RibbonBar* rb = l.at(i);
                            if (rb->keyTipsEnabled())
                            {
                                bool hasKeyTips = rb->qtn_d().m_keyTips.count() > 0;
                                HideKeyTipEvent ktEvent;
                                QApplication::sendEvent(l.at(i), &ktEvent);
                                d.m_destroyKeyTips = hasKeyTips;
                            }
                        }
                    }
                }
            }
            break;
        case QEvent::KeyRelease:
            if (QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event))
            {
                if (keyEvent->key() == Qt::Key_Alt) 
                {
                    if (QMainWindow* widget = qobject_cast<QMainWindow*>(watched))
                    {
                        if (!d.m_destroyKeyTips)
                        {
                            QList<RibbonBar*> l = widget->findChildren<RibbonBar*>();
                            for (int i = 0; i < l.size(); ++i)
                            {
                                const RibbonBar* rb = l.at(i);
                                if (!rb->isBackstageVisible() && rb->keyTipsEnabled() && !d.m_blockKeyTips)
                                {
                                    ShowKeyTipEvent ktEvent(l.at(i));
                                    QApplication::sendEvent(l.at(i), &ktEvent);
//                                    l.at(i)->setFocus();
                                }
                            }
                        }
                        else
                            d.m_destroyKeyTips = false;
                    }
                    d.m_completeKey = false;
                }
            }
            break;
        case QEvent::Show :
            if (QMenu* menu = qobject_cast<QMenu*>(watched))
            {
                if (RibbonBar* ribbonBar = ::findMainWindow())
                {
                    if (ribbonBar->qtn_d().m_levels.size() > 0 || 
                        ((qobject_cast<RibbonPageSystemPopup*>(watched) || qobject_cast<RibbonSystemPopupBar*>(watched)) && d.m_completeKey))
                        QApplication::postEvent(ribbonBar, new ShowKeyTipEvent(menu));
                }
                d.m_completeKey = false;
            }
            break;
        case QEvent::FontChange :
            if (qobject_cast<RibbonBar*>(watched)) //|| 
//                qobject_cast<Qtitan::RibbonPage*>(watched) || 
//                qobject_cast<Qtitan::RibbonGroup*>(watched) ||
//                qobject_cast<Qtitan::RibbonTabBar*>(watched))
            {
                d.refreshMetrics((QWidget*)watched);
            }
            break;
        default:
            break;
    }
    return OfficeStyle::eventFilter(watched, event);
}

/*!
    \class Qtitan::RibbonStylePlugin
    \internal
*/
QStringList RibbonStylePlugin::keys() const
{
    return QStringList() << "RibbonStyle";
}

QStyle* RibbonStylePlugin::create( const QString& key )
{
    if ( key.toLower() == QLatin1String("ribbonstyle"))
        return new RibbonStyle();
    return Q_NULL;
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#else
QObject* qt_plugin_instance_ribbonstyle()
{
    static QObject* instance = Q_NULL;
    if ( !instance )
        instance = new RibbonStylePlugin();
    return instance;
}

Q_IMPORT_PLUGIN(ribbonstyle)
#endif
