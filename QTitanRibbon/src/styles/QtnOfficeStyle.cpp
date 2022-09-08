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
#include <QApplication>
#include <QStyleOption>
#include <QPainter>
#include <QScrollBar>
#include <QMdiArea>
#include <QMenuBar>
#include <QComboBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QPushButton>
#include <QDockWidget>
#include <QGroupBox>
#include <QTreeView>
#include <QLineEdit>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QMenu>
#include <QProgressBar>
#include <QToolButton>
#include <QMainWindow>
#include <QBitmap>
#include <QTableView>
#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>
#include <QHeaderView>
#include <QMdiSubWindow>
#include <QTableWidget>
#include <QTreeWidget>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <qdrawutil.h>
#endif /* Q_OS_WIN*/

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif /* Q_OS_WIN*/

#include "../src/ribbon/QtnOfficePopupMenu.h"
#include "QtnPopupHelpers.h"
#include "QtnCommonStylePrivate.h"
#include "QtnStyleHelpers.h"
#include "QtnOfficeStyle.h"
#include "QtnOfficeStylePrivate.h"
#include "QtnRibbonGallery.h"


using namespace Qtitan;

static const int windowsItemFrame        =  2; // menu item frame width
static const int windowsItemHMargin      =  3; // menu item hor text margin
static const int windowsItemVMargin      =  2; // menu item ver text margin
static const int windowsRightBorder      = 15; // right border on windows
static const int windowsArrowHMargin     =  0; // arrow horizontal margin


/* OfficeStylePrivate */
OfficeStylePrivate::OfficeStylePrivate()
    : CommonStylePrivate()
{
    m_curAccentColor = OfficeStyle::AccentColorBlue;
    m_clrAccent = QColor(43, 87, 154);
    m_dpiAware = true;
    m_animEnabled = true;
    m_ignoreDialogs = false;
    m_popupProxy = Q_NULL;
    m_refCountProxy = 0;

    m_typePopupStyle = OfficeStyle::PopupSystemDecoration;

#ifdef Q_OS_LINUX
    m_themeType = OfficeStyle::Office2013Gray;
#endif /* Q_OS_LINUX */

#ifdef Q_OS_MAC
    m_themeType = OfficeStyle::Office2013Gray;
#endif /* Q_OS_MAC*/

#ifdef Q_OS_WIN
    m_themeType = OfficeStyle::Office2013White;
#endif /* Q_OS_WIN*/
}

OfficeStylePrivate::~OfficeStylePrivate()
{
}

void OfficeStylePrivate::initialization()
{
    updateColors();
}

void OfficeStylePrivate::setBrush(QWidget* widget)
{
    if (QMdiArea* mdiArea = qobject_cast<QMdiArea*>(widget))
    {
        m_customBrushWidgets.insert(widget, mdiArea->background());
    }
}

void OfficeStylePrivate::unsetBrush(QWidget* widget)
{
    if (m_customBrushWidgets.contains(widget)) 
    {
        if (QMdiArea* mdiArea = qobject_cast<QMdiArea*>(widget))
        {
            QBrush b = m_customBrushWidgets.value(widget);
            mdiArea->setBackground(b);
        }
        m_customBrushWidgets.remove(widget);
    }
}

void OfficeStylePrivate::updateColors()
{
    QTN_P(OfficeStyle);
    m_clrMenuBarGrayText = QColor(167, 167, 167);
    m_clrToolBarGrayText = QColor(141, 141, 141);
    m_clrHighlightDisabledBorder = QColor(141, 141, 141);
    m_clrMenubarFace = QColor(246, 246, 246);

    StyleHelper& helper = p.helper();
    helper.openIniFile(theme("OfficeTheme.ini"));

    if (m_curAccentColor != (OfficeStyle::AccentColor)-1)
        m_clrAccent = accentIndexToColor(m_curAccentColor);

    // [Window]
    m_clrFrame                    = helper.getColor("DockingPane", "WordSplitter");

    m_clr3DFace                   = helper.getColor("Window", "ButtonFace");  
    m_clrFrameBorderActive0       = helper.getColor("Window", "BorderActive0");
    m_clrFrameBorderActive1       = helper.getColor("Window", "BorderActive1");
    m_clrFrameBorderActive2       = helper.getColor("Window", "BorderActive2");
    m_clrFrameBorderActive3       = helper.getColor("Window", "BorderActive3");

    m_clrFrameBorderInactive0     = helper.getColor("Window", "BorderInactive0");
    m_clrFrameBorderInactive1     = helper.getColor("Window", "BorderInactive1");
    m_clrFrameBorderInactive2     = helper.getColor("Window", "BorderInactive2");
    m_clrFrameBorderInactive3     = helper.getColor("Window", "BorderInactive3");

    m_clrHighlightBorder          = helper.getColor("Window", "HighlightSelectedBorder");
    m_clrHighlightPushedBorder    = helper.getColor("Window", "HighlightPressedBorder");
    m_clrHighlightCheckedBorder   = helper.getColor("Window", "HighlightCheckedBorder");
    m_clrHighlightPushed          = helper.getColor("Window", "HighlightPressed");
    m_clrHighlight                = helper.getColor("Window", "HighlightSelected");
    m_clrHighlightChecked         = helper.getColor("Window", "HighlightChecked");

    m_palLunaSelected.setColor(QPalette::Dark, helper.getColor("Window", "HighlightSelectedDark"));
    m_palLunaSelected.setColor(QPalette::Light, helper.getColor("Window", "HighlightSelectedLight"));

    m_palLunaPushed.setColor(QPalette::Dark, helper.getColor("Window", "HighlightPressedDark"));
    m_palLunaPushed.setColor(QPalette::Light, helper.getColor("Window", "HighlightPressedLight"));

    m_palLunaChecked.setColor(QPalette::Dark, helper.getColor("Window", "HighlightCheckedDark"));
    m_palLunaChecked.setColor(QPalette::Light, helper.getColor("Window", "HighlightCheckedLight"));

    m_clrFrameCaptionTextInActive = helper.getColor("Window", "CaptionTextInActive");
    m_clrFrameCaptionTextActive   = helper.getColor("Window", "CaptionTextActive");

    m_clrTooltipLight             = helper.getColor("Window", "TooltipLight");
    m_clrTooltipDark              = helper.getColor("Window", "TooltipDark");
    m_clrTooltipBorder            = helper.getColor("Window", "TooltipBorder");

    // [Workspace]
    m_clrAppWorkspace             = helper.getColor("Workspace", "AppWorkspace");
    m_clrWorkspaceClientTop       = helper.getColor("Workspace", "WorkspaceClientTop");
    m_clrWorkspaceClientMiddle    = helper.getColor("Workspace", "WorkspaceClientMiddle");
    m_clrWorkspaceClientBottom    =  helper.getColor("Workspace", "WorkspaceClientBottom");

    // [Ribbon]
    m_clrControlEditNormal        = helper.getColor("Ribbon", "ControlEditNormal");
    m_clrMenuPopupText            = helper.getColor("Ribbon", "MenuPopupText");
    m_clrTabNormalText            = helper.getColor("Ribbon", "TabNormalText");
    m_clrHighlightText            = helper.getColor("Ribbon", "RibbonText");
    m_clrSelectedText             = helper.getColor("Ribbon", "TabSelectedText");
    m_clrToolBarText              = helper.getColor("Ribbon", /*"RibbonText"*/"MenuPopupText");
    m_clrMenuPopupGripperShadow   = helper.getColor("Ribbon", "MenuPopupGripperShadow", QColor(197, 197, 197));
    m_clrMenuGripper              = helper.getColor("Ribbon", "MenuPopupGripper");
    m_clrMenuPopupSeparator       = helper.getColor("Ribbon", "MenuPopupSeparator");

    // [Toolbar]
    m_clrEditCtrlBorder           = helper.getColor("Toolbar", "ControlEditBorder");
    m_clrMenuBarText              = helper.getColor("Toolbar", "MenuBarText");
    m_ToolbarGripper              = helper.getColor("Toolbar", "ToolbarGripper");
    m_clrControlGallerySelected   = helper.getColor("Toolbar", "ControlGallerySelected");
    m_clrControlGalleryNormal     = helper.getColor("Toolbar", "ControlGalleryNormal");
    m_clrControlGalleryBorder     = helper.getColor("Toolbar", "ControlGalleryBorder");
    m_clrControlGalleryMenuBk     = helper.getColor("Toolbar", "ControlGalleryBk", QColor(246, 246, 246)) ;
    m_clrControlGalleryLabel      = helper.getColor("Toolbar", "ControlLabel");
    m_clrDockBar                  = helper.getColor("Toolbar", "DockBarFace");
    m_clrMenubarBorder            = helper.getColor("Toolbar", "MenuPopupBorder");  
    m_clrToolbarFace              = helper.getColor("Toolbar", "ToolbarFace");
    m_clrToolbarSeparator         = helper.getColor("Toolbar", "ToolbarSeparator");

    // [PopupControl]
    m_clrBackgroundLight = helper.getColor("PopupControl", "BackgroundLight");
    m_clrBackgroundDark  = helper.getColor("PopupControl", "BackgroundDark");
    m_clrCaptionLight = helper.getColor("PopupControl", "CaptionLight");
    m_clrCaptionDark  = helper.getColor("PopupControl", "CaptionDark");


    // [ShortcutBar]
    m_clrShortcutItemShadow = helper.getColor("ShortcutBar", "FrameBorder");

    m_clrCommandBar.setColor(QPalette::Dark, helper.getColor("Toolbar", "ToolbarFaceDark"));
    m_clrCommandBar.setColor(QPalette::Light, helper.getColor("Toolbar", "ToolbarFaceLight"));
    m_clrCommandBar.setColor(QPalette::Shadow, helper.getColor("Toolbar", "ToolbarFaceShadow"));

    m_clrPopupControl.setColor(QPalette::Dark, helper.getColor("Toolbar", "ControlHighlightPopupedDark"));
    m_clrPopupControl.setColor(QPalette::Light, helper.getColor("Toolbar", "ControlHighlightPopupedLight"));

    // [DockingPane]
    m_palNormalCaption.setColor(QPalette::Dark, helper.getColor("DockingPane", "NormalCaptionDark"));
    m_palNormalCaption.setColor(QPalette::Light, helper.getColor("DockingPane", "NormalCaptionLight"));
    m_clrNormalCaptionText = helper.getColor("DockingPane", "NormalCaptionText");

    m_palActiveCaption.setColor(QPalette::Dark, helper.getColor("DockingPane", "ActiveCaptionDark"));
    m_palActiveCaption.setColor(QPalette::Light, helper.getColor("DockingPane", "ActiveCaptionLight"));
    m_clrActiveCaptionText = helper.getColor("DockingPane", "ActiveCaptionText");

    m_palSplitter.setColor(QPalette::Dark, helper.getColor("DockingPane", "SplitterDark"));
    m_palSplitter.setColor(QPalette::Light, helper.getColor("DockingPane", "SplitterLight"));

    // [StatusBar]
    m_clrStatusBarText = helper.getColor("StatusBar", "StatusBarText");
    m_clrStatusBarShadow = helper.getColor("StatusBar", "StatusBarShadow");
    m_palStatusBarTop.setColor(QPalette::Dark, helper.getColor("StatusBar", "StatusBarFaceTopDark"));
    m_palStatusBarTop.setColor(QPalette::Light, helper.getColor("StatusBar", "StatusBarFaceTopLight"));
    m_palStatusBarBottom.setColor(QPalette::Dark, helper.getColor("StatusBar", "StatusBarFaceBottomDark"));
    m_palStatusBarBottom.setColor(QPalette::Light, helper.getColor("StatusBar", "StatusBarFaceBottomLight"));

    // [Controls]
    m_crBorderShadow = helper.getColor("Controls", "GroupBoxFrame");
    m_tickSlider = helper.getColor("Controls", "TickSlider");  

    // [ReportControl]
    m_palGradientColumn.setColor(QPalette::Dark, helper.getColor("ReportControl", "NormalColumnDark"));
    m_palGradientColumn.setColor(QPalette::Light, helper.getColor("ReportControl", "NormalColumnLight"));
    m_palGradientColumn.setColor(QPalette::Shadow, helper.getColor("ReportControl", "ColumnShadow"));

    m_palGradientColumnPushed.setColor(QPalette::Dark, helper.getColor("ReportControl", "PressedColumnDark"));
    m_palGradientColumnPushed.setColor(QPalette::Light, helper.getColor("ReportControl", "PressedColumnLight"));

    m_clrColumnSeparator = helper.getColor("ReportControl", "ColumnSeparator");
    m_clrGridLineColor = helper.getColor("PropertyGrid", "GridLine");
    m_clrSelectionBackground = helper.getColor("ReportControl", "SelectionBackground");

    m_paintManeger->modifyColors();
}

void OfficeStylePrivate::refreshMetrics()
{
    QTN_P(OfficeStyle);
    QWidgetList all = p.allWidgets();

    // clean up the old style
    for (QWidgetList::ConstIterator it = all.constBegin(); it != all.constEnd(); ++it) 
    {
        register QWidget* w = *it;
        if (!(w->windowType() == Qt::Desktop) && w->testAttribute(Qt::WA_WState_Polished)) 
            p.unpolish(w);
    }
    p.unpolish(qApp);

    // initialize the application with the new style
    p.polish(qApp);

    all = p.allWidgets();

    // re-polish existing widgets if necessary
    for (QWidgetList::ConstIterator it1 = all.constBegin(); it1 != all.constEnd(); ++it1) 
    {
        register QWidget* w = *it1;
        if (w->windowType() != Qt::Desktop && w->testAttribute(Qt::WA_WState_Polished)) 
            p.polish(w);
    }

    for (QWidgetList::ConstIterator it2 = all.constBegin(); it2 != all.constEnd(); ++it2) 
    {
        register QWidget* w = *it2;
        if (w->windowType() != Qt::Desktop && !w->testAttribute(Qt::WA_SetStyle))
        {
            QEvent e(QEvent::StyleChange);
            QApplication::sendEvent(w, &e);
            w->update();
        }
    }
}

QString OfficeStylePrivate::theme(const QString& str) const
{
    QString strStyle;

    switch (m_themeType) 
    {
        case OfficeStyle::Office2007Blue  :
            strStyle += ":/res/Office2007Blue/";
            break;
        case OfficeStyle::Office2007Black :
            strStyle += ":/res/Office2007Black/";
            break;
        case OfficeStyle::Office2007Silver :
            strStyle += ":/res/Office2007Silver/";
            break;
        case OfficeStyle::Office2007Aqua :
            strStyle += ":/res/Office2007Aqua/";
            break;
        case OfficeStyle::Windows7Scenic :
            strStyle += ":/res/Windows7Scenic/";
            break;
        case OfficeStyle::Office2010Silver :
            strStyle += ":/res/Office2010Silver/";
            break;
        case OfficeStyle::Office2010Blue :
            strStyle += ":/res/Office2010Blue/";
            break;
        case OfficeStyle::Office2010Black :
            strStyle += ":/res/Office2010Black/";
            break;
        case OfficeStyle::Office2013White :
            strStyle += ":/res/Office2013White/";
            break;
        case OfficeStyle::Office2013Gray:
            strStyle += ":/res/Office2013Gray/";
            break;
        default:
            Q_ASSERT(false);
            break;
    }

    strStyle += str;
    return strStyle;
}

/*! \internal */
IOfficePaintManager* OfficeStylePrivate::officePaintManager() const
{
    QTN_P(const OfficeStyle);
    IOfficePaintManager* officePaintManager = qobject_cast<IOfficePaintManager*>(&p.paintManager());
    return officePaintManager;
}

/*! \internal */
void OfficeStylePrivate::makePaintManager()
{
    QTN_P(OfficeStyle)
    if (p.getTheme() == OfficeStyle::Office2013White || p.getTheme() == OfficeStyle::Office2013Gray)
        setPaintManager(*new OfficePaintManager2013(&p));
    else
        setPaintManager(*new OfficePaintManager(&p));
}

/*! \internal */
QColor OfficeStylePrivate::accentIndexToColor(OfficeStyle::AccentColor accentcolor) const
{
    QTN_P(const OfficeStyle)
    switch (accentcolor)
    {
        case OfficeStyle::AccentColorBlue:
        default:
            return p.getThemeColor("Window", "AccentColorBlue", QColor(43, 87, 154));

        case OfficeStyle::AccentColorBrown:
            return p.getThemeColor("Window", "AccentColorBrown", QColor(161, 53, 55));

        case OfficeStyle::AccentColorGreen:
            return p.getThemeColor("Window", "AccentColorGreen", QColor(33, 115, 70));

        case OfficeStyle::AccentColorLime:
            return p.getThemeColor("Window", "Accent2013Lime", QColor(137, 164, 48));

        case OfficeStyle::AccentColorMagenta:
            return p.getThemeColor("Window", "Accent2013Magenta", QColor(216, 0, 115));

        case OfficeStyle::AccentColorOrange:
            return p.getThemeColor("Window", "Accent2013Orange", QColor(208, 69, 37));

        case OfficeStyle::AccentColorPink:
            return p.getThemeColor("Window", "Accent2013Pink", QColor(230, 113, 184));

        case OfficeStyle::AccentColorPurple:
            return p.getThemeColor("Window", "Accent2013Purple", QColor(126, 56, 120));

        case OfficeStyle::AccentColorRed:
            return p.getThemeColor("Window", "Accent2013Red", QColor(229, 20, 0));

        case OfficeStyle::AccentColorTeal:
            return p.getThemeColor("Window", "Accent2013Red", QColor(7, 114, 101));
    }
}

/*!
Constuctor of the OfficeStyle.
*/
OfficeStyle::OfficeStyle()
    : CommonStyle(*new OfficeStylePrivate)
{
//    QTN_INIT_PRIVATE(OfficeStyle);
    QTN_D(OfficeStyle);
    d.initialization();
}

OfficeStyle::OfficeStyle(OfficeStylePrivate& d)
    : CommonStyle(d)
{
    d.initialization();
}

OfficeStyle::OfficeStyle(QMainWindow* mainWindow, OfficeStylePrivate& d)
    : CommonStyle(d)
{
    Q_UNUSED(mainWindow);
    //    QTN_INIT_PRIVATE(OfficeStyle);
    d.initialization();
    qApp->setStyle(this);
}

/*!
Constructor of OfficeStyle class. The application is usually required to use this constructor only once. 
After this, the call QApplcation::setStyle(...) is not required. Parameter \a mainWindow is not used.
*/
OfficeStyle::OfficeStyle(QMainWindow* mainWindow)
    : CommonStyle(*new OfficeStylePrivate)
{
    Q_UNUSED(mainWindow);
//    QTN_INIT_PRIVATE(OfficeStyle);
    QTN_D(OfficeStyle);
    d.initialization();
    qApp->setStyle(this);
}

/*!
Destructor of OfficeStyle class.
*/
OfficeStyle::~OfficeStyle()
{
    unsetPopupProxy();
//    QTN_FINI_PRIVATE();
}

/*!
Sets an accent color for Office stlye by color index.
*/
void OfficeStyle::setAccentColor(AccentColor index)
{
    QTN_D(OfficeStyle);
    d.m_curAccentColor = index;
    d.m_clrAccent = d.accentIndexToColor(index);
    d.updateColors();
}

/*!
Sets an accent color for Office stlye.
*/
void OfficeStyle::setAccentColor(const QColor& accentcolor)
{
    QTN_D(OfficeStyle);
    d.m_curAccentColor = (AccentColor)-1;
    d.m_clrAccent = accentcolor;
    d.updateColors();
}


/*!
Returns an accent color for Office stlye.
*/
QColor OfficeStyle::accentColor() const
{
    QTN_D(const OfficeStyle);
    return d.m_curAccentColor;
}

/*!
Sets the \a theme of the office style. You can choose from Office 2007 or Office 2010 theme's families. 
\sa Theme
*/
void OfficeStyle::setTheme(Theme theme)
{
    QTN_D(OfficeStyle);
    if (d.m_themeType != theme) 
    {
        d.m_themeType = theme;
        clearCache();
        d.makePaintManager();
        d.refreshMetrics();
    }
}

/*!
Returns the theme of the style.
\sa Theme
*/
OfficeStyle::Theme OfficeStyle::getTheme() const
{
    QTN_D(const OfficeStyle);
    return d.m_themeType; 
}

/*!
\brief Sets the animation for Microsoft Office's controls when mouse over the elements to \a enable state. By default the animation is set.
 */
void OfficeStyle::setAnimationEnabled(bool enable/*= true*/)
{
    QTN_D(OfficeStyle);
    d.m_animEnabled = enable;
}

/*!
\brief Returns animation state for Microsoft Office's controls when mouse over the elements.
 */
bool OfficeStyle::isAnimationEnabled() const
{
    QTN_D(const OfficeStyle);
    return d.m_animEnabled;
}

/*!
\brief Sets \a ignore dialog flag. If this flag is true that can ignore the current Ribbon or Office Style for dialogs and standard Qt style will be used for dialog. By default this flag is false.
*/ 
void OfficeStyle::setDialogsIgnored(bool ignore)
{
    QTN_D(OfficeStyle);
    d.m_ignoreDialogs = ignore;
}

/*!
    \brief Returns the ignore dialog flag.
*/ 
bool OfficeStyle::isDialogsIgnored() const
{
    QTN_D(const OfficeStyle);
    return d.m_ignoreDialogs;
}

/*!
Activates in the style the mechanism that makes it possible to bind variety font metrics to the system DPI settings.
If parameter \a aware is true then the application is sensitive to DPI settings.
*/
void OfficeStyle::setDPIAware(bool aware)
{
    QTN_D(OfficeStyle);
    if (d.m_dpiAware == aware)
        return;

    d.m_dpiAware = aware;
    d.refreshMetrics();
}

/*!
Returns the feature of style reaction to the system DPI settings.
*/
bool OfficeStyle::isDPIAware() const
{
#ifdef Q_OS_WIN
    QTN_D(const OfficeStyle);
    return d.m_dpiAware;
#endif /* Q_OS_WIN*/
    return true;
}

/*!
Returns the value of the DPI setting in native format. If isDPIAware() is false then this function always returns 96. 
*/
int OfficeStyle::getDPI() const
{
#ifdef Q_OS_WIN
    return isDPIAware() ? DrawHelpers::getDPI() : 96;
#endif /* Q_OS_WIN*/
    return -1;
}

/*!
Returns the value of the DPI setting in percentage. If isDPIAware() is false then this function always returns 100%.
*/
int OfficeStyle::getDPIToPercent() const
{
#ifdef Q_OS_WIN
    return isDPIAware() ? DrawHelpers::getDPIToPercent() : 100;
#endif /* Q_OS_WIN*/
    return -1;
}

/*!
Returns the value of the popupDecoration flag.
*/
OfficeStyle::OfficePopupDecoration OfficeStyle::popupDecoration() const
{
    QTN_D(const OfficeStyle);
    return d.m_typePopupStyle;
}

/*!
Sets the style of the \a decoration for OfficePopupWindow. 
*/
void OfficeStyle::setPopupDecoration(OfficeStyle::OfficePopupDecoration decoration)
{
    QTN_D(OfficeStyle);
    d.m_typePopupStyle = decoration;
}

/*! \reimp */
void OfficeStyle::polish(QApplication* pApp)
{
    CommonStyle::polish(pApp);
    QTN_D(OfficeStyle);
    d.updateColors();
}

// native Dialog
static bool isParentDialog(const QWidget* widget)
{
    return widget && qobject_cast<QDialog*>(widget->topLevelWidget());
}

/*! \reimp */
void OfficeStyle::polish(QWidget* widget)
{
    CommonStyle::polish(widget);

    QTN_D(OfficeStyle);
    d.unsetBrush(widget);
    d.setBrush(widget);


    if (qobject_cast<QCheckBox*>(widget)   ||
        qobject_cast<QRadioButton*>(widget)||
        qobject_cast<QToolButton*>(widget) ||
        qobject_cast<QComboBox*>(widget)   ||
        qobject_cast<QScrollBar*>(widget)  ||
        qobject_cast<QGroupBox*>(widget)   ||
        qobject_cast<QTabBar*>(widget)     ||
        qobject_cast<QAbstractSpinBox*>(widget) ||
        qobject_cast<QSlider*>(widget)     ||
        qobject_cast<QTabWidget*>(widget)  ||
        qobject_cast<QAbstractButton*>(widget))
    {
        widget->setAttribute(Qt::WA_Hover, true);
    }

    if (widget->metaObject()->className() == QString("QToolButton"))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::HighlightedText, d.m_clrMenuPopupText);
        widget->setPalette(palette);
    }
    else if (widget->metaObject()->className() == QString("QComboBoxListView"))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::All, QPalette::HighlightedText, QColor(Qt::white));
        widget->setPalette(palette);
    }
    else if (widget->inherits("Qtitan::QuickCustomizationPopup"))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::All, QPalette::Background, helper().getColor("Ribbon", "RibbonFace"));
        widget->setPalette(palette);
    }
    else if (widget->inherits("QTableView") || widget->inherits("Qtitan::Grid"))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::All, QPalette::Background, helper().getColor("Ribbon", "RibbonFace"));
        widget->setPalette(palette);
    }
    else if (qobject_cast<QTableView*>(widget) || widget->inherits("Qtitan::Grid"))
    {
        QPalette palette = widget->palette();
        QColor clrNormalText = helper().getColor(tr("ListBox"), tr("NormalText"));
        palette.setColor(QPalette::Active, QPalette::Text, d.m_clrHighlight);
        palette.setColor(QPalette::All, QPalette::HighlightedText, clrNormalText);
        palette.setColor(QPalette::All, QPalette::Text, clrNormalText);

        palette.setColor(QPalette::Normal, QPalette::Highlight, d.m_clrSelectionBackground);
        palette.setColor(QPalette::Disabled, QPalette::Highlight, d.m_clrSelectionBackground);
        palette.setColor(QPalette::Inactive, QPalette::Highlight, d.m_clrSelectionBackground);

        widget->setPalette(palette);

        widget->setAttribute(Qt::WA_Hover, true);
    }
    else if (QTreeView* tree = qobject_cast<QTreeView*>(widget)) 
    {
        QPalette palette = widget->palette();
        QColor clrNormalText = helper().getColor(tr("ListBox"), tr("NormalText"));
        palette.setColor(QPalette::Active, QPalette::Text, d.m_clrHighlight);
        palette.setColor(QPalette::All, QPalette::HighlightedText, clrNormalText);
        palette.setColor(QPalette::All, QPalette::Text, clrNormalText);
        widget->setPalette(palette);

        tree->viewport()->setAttribute(Qt::WA_Hover);
    }
    else if (QListView* list = qobject_cast<QListView*>(widget)) 
    {
        QPalette palette = widget->palette();
        QColor clrNormalText = helper().getColor(tr("ListBox"), tr("NormalText"));
        palette.setColor(QPalette::Active, QPalette::Text, d.m_clrHighlight);
        palette.setColor(QPalette::All, QPalette::HighlightedText, clrNormalText);
        palette.setColor(QPalette::All, QPalette::Text, clrNormalText);
        widget->setPalette(palette);

        list->viewport()->setAttribute(Qt::WA_Hover);
    }
    else if (qobject_cast<QMainWindow*>(widget))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Background, d.m_clrDockBar);
        widget->setPalette(palette);
    }
    else if (!isDialogsIgnored() && qobject_cast<QDialog*>(widget))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Background, helper().getColor(tr("Dialog"), tr("Background")));
        widget->setPalette(palette);
    }
    else if (qobject_cast<QDockWidget*>(widget))
    {
        widget->setAutoFillBackground(true);
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Background, d.m_clrDockBar);
        widget->setPalette(palette);
    }
    else if(qobject_cast<QStatusBar*>(widget))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Foreground, d.m_clrStatusBarText);
        widget->setPalette(palette);
    }
    else if(qobject_cast<QMdiArea*>(widget))
    {
        d.officePaintManager()->setupPalette(widget);
    }
    else if (qobject_cast<QProgressBar*>(widget))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Active, QPalette::Highlight, d.m_clrAppWorkspace);
        widget->setPalette(palette);
    }
    else if (qobject_cast<QGroupBox*>(widget))
    {
        QPalette palette = widget->palette();
        palette.setBrush(QPalette::Dark, d.m_crBorderShadow);
        widget->setPalette(palette);
    }
    else if (qobject_cast<QMenuBar*>(widget))
    {
        QPalette palette = widget->palette();
        palette.setColor(QPalette::ButtonText, getTextColor(false, false, true, false, false, TypeMenuBar, BarNone));
        palette.setColor(QPalette::HighlightedText, getTextColor(true, false, true, false, false, TypeMenuBar, BarNone));
        widget->setPalette(palette);
    }
    else if (qobject_cast<QSlider*>(widget))
    {
        QPalette palette = widget->palette();
        palette.setBrush(QPalette::Shadow, d.m_tickSlider);
        palette.setBrush(QPalette::Foreground, d.m_tickSlider);
        widget->setPalette(palette);
    }
    else if (qobject_cast<QTabBar*>(widget) && qobject_cast<QMdiArea*>(widget->parentWidget()))
    {
        widget->setAutoFillBackground(true);
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Background, helper().getColor(tr("TabManager"), tr("HeaderFaceLight")));
        widget->setPalette(palette);
        ((QTabBar*)widget)->setExpanding(false);
    }
    else if (qobject_cast<QHeaderView*>(widget))
    {
        QPalette palette = widget->palette();
        QColor clrNormalText = helper().getColor(tr("ListBox"), tr("NormalText"));
        palette.setColor(QPalette::All, QPalette::ButtonText, clrNormalText);
        widget->setPalette(palette);
    }

    if(defaultStyle())
    {   
        if (qobject_cast<QMdiSubWindow*>(widget))
            widget->setStyle(defaultStyle());
        else if(isDialogsIgnored() && isParentDialog(widget) && qApp->style() == this && 
                widget->style()->metaObject()->className() != QString("QStyleSheetStyle"))
            widget->setStyle(defaultStyle());
    }
}

/*! \reimp */
void OfficeStyle::polish(QPalette& palette)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QCommonStyle::polish(palette);
#else
    QWindowsStyle::polish(palette);
#endif
}

/*! \reimp */
void OfficeStyle::unpolish(QApplication* app)
{
    CommonStyle::unpolish(app);
}

/*! \reimp */
void OfficeStyle::unpolish(QWidget* widget)
{
    CommonStyle::unpolish(widget);

    QTN_D(OfficeStyle);
    d.unsetBrush(widget);

    if (qobject_cast<QCheckBox*>(widget)   ||
        qobject_cast<QRadioButton*>(widget)||
        qobject_cast<QPushButton*>(widget) ||
        qobject_cast<QToolButton*>(widget) ||
        qobject_cast<QAbstractSpinBox*>(widget) ||
        qobject_cast<QComboBox*>(widget)   ||
        qobject_cast<QScrollBar*>(widget)  ||
        qobject_cast<QGroupBox*>(widget)   ||
        qobject_cast<QSlider*>(widget))
        widget->setAttribute(Qt::WA_Hover, false);
}

/*! Returns true if style is using Office 2010 themes family. Otherwise it return false. */
bool OfficeStyle::isStyle2010() const
{
    QTN_D(const OfficeStyle);
    return d.m_themeType == Office2010Black || d.m_themeType == Office2010Blue || d.m_themeType == Office2010Silver;
}

/*! \internal */
void OfficeStyle::createPopupProxy()
{
    QTN_D(OfficeStyle);

    if (d.m_popupProxy)
    {
        ++d.m_refCountProxy;
        return;
    }

    PopupDrawHelper* popupStyle = Q_NULL;
    switch(d.m_typePopupStyle)
    {
        case PopupCompatibleDecoration :
                popupStyle = new PopupOffice2000DrawHelper();
            break;
        case PopupOfficeFlatDecoration :
        case PopupSystemDecoration :
            {
                OfficeStyle::Theme theme = getTheme();
                PopupOffice2003DrawHelper* popup2003Style = Q_NULL;
                if (d.m_typePopupStyle == PopupSystemDecoration)
                {
                    if (theme == OfficeStyle::Office2007Blue   || 
                        theme == OfficeStyle::Office2007Black  || 
                        theme == OfficeStyle::Office2007Silver || 
                        theme == OfficeStyle::Office2007Aqua)
                        popup2003Style = new PopupOffice2007DrawHelper();
                    else
                        popup2003Style = new PopupOffice2010DrawHelper();
                }
                else
                {
                    popup2003Style = new PopupOffice2003DrawHelper();
                }
                switch (getTheme())
                {
                    case OfficeStyle::Office2007Blue :
                    case OfficeStyle::Office2010Blue :
                            popup2003Style->setDecoration(PopupOffice2003DrawHelper::OS_SYSTEMBLUE);
                        break;
                    case OfficeStyle::Office2007Black :
                    case OfficeStyle::Office2010Black :
                            popup2003Style->setDecoration(PopupOffice2003DrawHelper::OS_SYSTEMROYALE);
                        break;
                    case OfficeStyle::Office2007Silver :
                    case OfficeStyle::Office2010Silver :
                            popup2003Style->setDecoration(PopupOffice2003DrawHelper::OS_SYSTEMSILVER);
                        break;
                    case OfficeStyle::Office2007Aqua :
                    case OfficeStyle::Windows7Scenic :
                            popup2003Style->setDecoration(PopupOffice2003DrawHelper::OS_SYSTEMOLIVE);
                        break;
                    default:
                            Q_ASSERT(false);
                        break;
                }
                popupStyle = popup2003Style;
            }
            break;
        case PopupMSNDecoration :
                popupStyle = new PopupMSNDrawHelper();
            break;
        default:
            break;
    }
    if (popupStyle)
        popupStyle->refreshPalette();

    d.m_popupProxy = popupStyle;
    ++d.m_refCountProxy;
}

/*! \internal */
bool OfficeStyle::isExistPopupProxy() const
{
    QTN_D(const OfficeStyle);
    return d.m_popupProxy;
}

/*! \internal */
void OfficeStyle::unsetPopupProxy()
{
    QTN_D(OfficeStyle);
    if (!--d.m_refCountProxy) 
    {
        delete d.m_popupProxy;
        d.m_popupProxy = Q_NULL;
    }
}

/*! \internal */
bool OfficeStyle::transitionsEnabled() const
{
    QTN_D(const OfficeStyle);
    return d.m_animEnabled;
}

/*! \reimp */
QSize OfficeStyle::sizeFromContents(ContentsType ct, const QStyleOption* opt, const QSize& contentsSize, const QWidget* w) const
{
    switch (ct) 
    {
        case CT_MenuItem :
            {
                QSize s = CommonStyle::sizeFromContents(ct, opt, contentsSize, w);
                s.setWidth(s.width() + s.height());
                if (const QStyleOptionMenuItem* menuitem = qstyleoption_cast<const QStyleOptionMenuItem *>(opt))
                {
                    if (menuitem->menuItemType != QStyleOptionMenuItem::Separator)
                        s.setHeight(qMax(s.height(), 22));
                    else if (menuitem->menuItemType == QStyleOptionMenuItem::Separator && !menuitem->text.isEmpty())
                    {
                        int w = 0;
                        QFontMetrics fm(menuitem->font);
                        QFont fontBold = menuitem->font;
                        fontBold.setBold(true);
                        QFontMetrics fmBold(fontBold);
                        w += fmBold.width(menuitem->text) + 6 + 6;
                        int h =  menuitem->fontMetrics.height() + 6;
                        s.setHeight(qMax(h, 22));
                        s.setWidth(qMax(w, s.width()));
                    }
                }
                return s;
            }
        default:
            break;
    }
    return CommonStyle::sizeFromContents(ct, opt, contentsSize, w);
}

/*! \reimp */
QRect OfficeStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex* opt, SubControl sc, const QWidget* widget) const
{
    QTN_D(const OfficeStyle);
    switch (cc)
    {
        case CC_TitleBar:
            if (widget && (widget->metaObject()->className() == QString("Qtitan::OfficePopupWindow")) && d.m_popupProxy)
                return d.m_popupProxy->subControlRect(cc, opt, sc, widget);
            break;
        case CC_ComboBox:
            if (const QStyleOptionComboBox* cmb = qstyleoption_cast<const QStyleOptionComboBox*>(opt)) 
            {
                int x = cmb->rect.x(), y = cmb->rect.y(), wi = cmb->rect.width(), he = cmb->rect.height();
                int xpos = x;
                xpos += wi - 16;

                switch (sc) 
                {
                    case SC_ComboBoxArrow:
                        {
                            QRect rect = QRect(xpos, y, 16, he);
                            return visualRect(opt->direction, opt->rect, rect);
                        }
                    default:
                        break;
                }
            }
            break;

        case QStyle::CC_ToolButton :
            if (const QStyleOptionToolButton *tb = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) 
            {
                int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, tb, widget);
                QRect ret = tb->rect;
                switch (sc) 
                {
                    case SC_ToolButton:
                        if ((tb->features
                            & (QStyleOptionToolButton::MenuButtonPopup | QStyleOptionToolButton::PopupDelay))
                            == QStyleOptionToolButton::MenuButtonPopup)
                            ret.adjust(0, 0, -mbi, 0);
                        break;
                    case SC_ToolButtonMenu:
                        if ((tb->features
                            & (QStyleOptionToolButton::MenuButtonPopup | QStyleOptionToolButton::PopupDelay))
                            == QStyleOptionToolButton::MenuButtonPopup)
                            ret.adjust(ret.width() - mbi - 2, 0, 0, 0);
                        break;
                    default:
                        break;
                    }
                    return visualRect(tb->direction, tb->rect, ret);
            }
            break;
        default:
            break;
    };

    return CommonStyle::subControlRect(cc, opt, sc, widget);
}

/*! \reimp */
int OfficeStyle::pixelMetric(PixelMetric pm, const QStyleOption* opt, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    int ret = 0;
    if (d.officePaintManager()->pixelMetric(pm, opt, w, ret))
        return ret;

    switch(pm)
    {
        case PM_TitleBarHeight:
            if (w && (w->metaObject()->className() == QString("Qtitan::OfficePopupWindow")) && d.m_popupProxy)
                ret = d.m_popupProxy->pixelMetric(pm, opt, w);
            else
                ret = CommonStyle::pixelMetric(pm, opt, w);
            break;
        case PM_DefaultFrameWidth:
            if (w && (w->metaObject()->className() == QString("Qtitan::OfficePopupWindow")) && d.m_popupProxy)
                d.m_popupProxy->pixelMetric(pm, opt, w);
            else
                ret = CommonStyle::pixelMetric(pm, opt, w);
            break;
        case PM_MenuPanelWidth :
                ret = 2;
            break;
        case PM_MenuHMargin:
                ret = 0;
            break;
        case PM_MenuBarItemSpacing :
                ret = 1;
            break;
        case PM_MenuBarVMargin:
                ret = 2;
            break;

        case PM_MenuBarHMargin:
                ret = 4;
            break;

        case PM_DockWidgetSeparatorExtent :
                ret = 4;
            break;
        case PM_DockWidgetTitleMargin :
                ret = CommonStyle::pixelMetric(pm, opt, w);
            break;
        default:
                ret = CommonStyle::pixelMetric(pm, opt, w);
            break;
    }
    return ret;
}

/*! \reimp */
int OfficeStyle::styleHint(StyleHint hint, const QStyleOption* opt, const QWidget* widget, QStyleHintReturn* returnData) const
{
    QTN_D(const OfficeStyle)
    int ret = 0;
    switch (hint)
    {
        case SH_Menu_MouseTracking:
        case SH_ComboBox_ListMouseTracking :
                ret = 1;
            break;
        case SH_ToolTip_Mask :
        case SH_Menu_Mask :
            if (QStyleHintReturnMask* mask = qstyleoption_cast<QStyleHintReturnMask*>(returnData))
            {
                mask->region = opt->rect;
                //left rounded corner
                mask->region -= QRect(opt->rect.left(), opt->rect.top(), 1, 1);
                //right rounded corner
                mask->region -= QRect(opt->rect.right(), opt->rect.top(), 1, 1);
                //right-bottom rounded corner
                mask->region -= QRect(opt->rect.right(), opt->rect.bottom(), 1, 1);
                //left-bottom rounded corner
                mask->region -= QRect(opt->rect.left(), opt->rect.bottom(), 1, 1);
                ret = 1;
            }
            break;
        case SH_Table_GridLineColor:
                ret = opt ? d.m_clrGridLineColor.rgb() : CommonStyle::styleHint(hint, opt, widget, returnData);;
            break;
        default:
                ret = CommonStyle::styleHint(hint, opt, widget, returnData);
            break;
    }
    return ret;
}

/*! \reimp */
QStyle::SubControl OfficeStyle::hitTestComplexControl(ComplexControl control, const QStyleOptionComplex* opt, const QPoint& pt, const QWidget* widget) const
{
    if (control == CC_TitleBar)
    {
        QTN_D(const OfficeStyle);
        if (widget && (widget->metaObject()->className() == QString("Qtitan::OfficePopupWindow")) && d.m_popupProxy)
            return d.m_popupProxy->hitTestComplexControl(control, opt, pt, widget);
    }
    return CommonStyle::hitTestComplexControl(control, opt, pt, widget);
}

/*! \reimp */
void OfficeStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex* opt, QPainter* p, const QWidget* widget) const
{
    if (cc == CC_TitleBar)
    {
        QTN_D(const OfficeStyle);
        if (widget && (widget->metaObject()->className() == QString("Qtitan::OfficePopupWindow")) && d.m_popupProxy)
        {
            d.m_popupProxy->drawComplexControl(cc, opt, p, widget);
            return;
        }
    }
    CommonStyle::drawComplexControl(cc, opt, p, widget);
}

/*! \reimp */
void OfficeStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    if (element == PE_FrameWindow)
    {
        QTN_D(const OfficeStyle);
        if (widget && (widget->metaObject()->className() == QString("Qtitan::OfficePopupWindow")) && d.m_popupProxy)
        {
            d.m_popupProxy->drawPrimitive(element, option, painter, widget);
            return;
        }
    }
    CommonStyle::drawPrimitive(element, option, painter, widget);
}

/*! \internal */
QPixmap OfficeStyle::cached(const QString& img) const
{
    QTN_D(const OfficeStyle)
    return CommonStyle::cached(d.theme(img));
}

/*! \internal */
bool OfficeStyle::drawWorkspace(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawWorkspace(opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawWidget(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle)
    if (qobject_cast<const QMainWindow*>(w))
    {
        QRect rc(opt->rect);
        p->fillRect(rc, d.m_clrFrame);
        return true;
    }
    return false;
}

// for QForm
/*! \internal */
bool OfficeStyle::drawFrameTabWidget(const QStyleOption* opt, QPainter* p, const QWidget*) const
{
    QPixmap soTabPaneEdge = cached("TabPaneEdge.png");
    if (soTabPaneEdge.isNull())
        return false;
    drawImage(soTabPaneEdge, *p, opt->rect, soTabPaneEdge.rect(), QRect(QPoint(5, 18), QPoint(5, 5)));
    return true;
}

/*! \internal */
bool OfficeStyle::drawShapedFrame(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawShapedFrame(opt, p, w);
}

// for statusBar
/*! \internal */
bool OfficeStyle::drawPanelStatusBar(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawPanelStatusBar(opt, p, w);
}

// for menuBar
/*! \internal */
bool OfficeStyle::drawMenuBarEmptyArea(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(opt);
    Q_UNUSED(p);
    Q_UNUSED(w);
//    QPixmap soImage = cached("ToolbarFaceHorizontal.png");
//    drawImage(soImage, *p, opt->rect, soImage.rect(), QRect(QPoint(5, 5), QPoint(5, 5)));
    return true;
}

/*! \internal */
bool OfficeStyle::drawPanelMenu(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawPanelMenu(opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawMenuBarItem(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    if (const QStyleOptionMenuItem* mbi = qstyleoption_cast<const QStyleOptionMenuItem*>(opt))
    {
        QRect rect(opt->rect);

        if (mbi->menuItemType == QStyleOptionMenuItem::DefaultItem)
            return true;

        QPixmap pix = mbi->icon.pixmap(proxy()->pixelMetric(PM_SmallIconSize, opt, w), QIcon::Normal);

        uint alignment = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
        if (!proxy()->styleHint(SH_UnderlineShortcut, mbi, w))
            alignment |= Qt::TextHideMnemonic;

        bool enabled  = opt->state & State_Enabled;
        bool checked  = opt->state & State_On;
        bool selected = opt->state & State_Selected;
        bool pressed  = opt->state & State_Sunken;
//        bool popuped  = false;//(mbi->activeSubControls & QStyle::SC_ToolButtonMenu) && (opt->state & State_Sunken);
        bool popuped  = (opt->state & QStyle::State_Selected) && (opt->state & State_Sunken);

        drawRectangle(p, opt->rect, selected, pressed, enabled, checked, popuped, TypeMenuBar, BarTop);

        QPalette::ColorRole textRole = !enabled ? QPalette::Text:
            selected ? QPalette::HighlightedText : QPalette::ButtonText;

        if (!pix.isNull())
            drawItemPixmap(p, mbi->rect, alignment, pix);
        else
            drawItemText(p, mbi->rect, alignment, mbi->palette, mbi->state & State_Enabled, mbi->text, textRole);
        return true;
    }
    return false;
}

/*! \internal */
bool OfficeStyle::drawIndicatorMenuCheckMark(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle)
    return d.officePaintManager()->drawIndicatorMenuCheckMark(opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawMenuItem(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    QTN_D(const OfficeStyle)
    if (const QStyleOptionMenuItem* menuitem = qstyleoption_cast<const QStyleOptionMenuItem*>(opt)) 
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

        if (menuitem->text.count(QString(qtn_SplitActionPopup)) != 0)
            drawSplitButtonPopup(p, menuitem->rect, act, !dis, menuitem->checkType == QStyleOptionMenuItem::Exclusive);

        if (menuitem->menuItemType == QStyleOptionMenuItem::Separator)
        {
            d.officePaintManager()->drawMenuItemSeparator(opt, p, widget);
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
                pixmap = menuitem->icon.pixmap(iconSize, mode, QIcon::On);
            else
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
        else if (checked) 
        {
            QStyleOptionMenuItem newMi = *menuitem;
            newMi.state = State_None;
            if (!dis)
                newMi.state |= State_Enabled;
            if (act)
                newMi.state |= State_On;
            newMi.rect = visualRect(opt->direction, menuitem->rect, QRect(menuitem->rect.x() + windowsItemFrame, menuitem->rect.y() + windowsItemFrame,
                                    checkcol - 2 * windowsItemFrame, menuitem->rect.height() - 2*windowsItemFrame));
            drawPrimitive(PE_IndicatorMenuCheckMark, &newMi, p, widget);
        }
        //    p->setPen(act ? menuitem->palette.highlightedText().color() : menuitem->palette.buttonText().color());

        QColor discol;
        if (dis) 
        {
            discol = menuitem->palette.text().color();
            p->setPen(discol);
        }

        int xm = windowsItemFrame + checkcol + windowsItemHMargin;
        int xpos = menuitem->rect.x() + xm;
        QRect textRect(xpos, y + windowsItemVMargin, w - xm - windowsRightBorder - tab + 1, h - 2 * windowsItemVMargin);
        QRect vTextRect = visualRect(opt->direction, menuitem->rect, textRect);
        QString s = menuitem->text;
        s = s.remove(QString(qtn_SplitActionPopup));
        // draw text    
        if (!s.isEmpty()) 
        { 
            p->save();
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
                /*
                if (dis && !act && styleHint(SH_EtchDisabledText, opt, widget)) 
                {
                    p->setPen(menuitem->palette.light().color());
                    p->drawText(vShortcutRect.adjusted(1,1,1,1), text_flags, s.mid(t + 1));
                    p->setPen(discol);
                }
                */

                p->setPen(opt->state & State_Enabled ? d.m_clrMenuPopupText : d.m_clrMenuBarGrayText);
                p->drawText(vShortcutRect, text_flags, s.mid(t + 1));
                p->setPen(discol);
                s = s.left(t);
            }

            QFont font = menuitem->font;
            if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                font.setBold(true);
            p->setFont(font);
            /*
            if (dis && !act && styleHint(SH_EtchDisabledText, opt, widget)) 
            {
                p->setPen(menuitem->palette.light().color());
                p->drawText(vTextRect.adjusted(1,1,1,1), text_flags, s.left(t));
                p->setPen(discol);
            }
            */
            p->setPen(opt->state & State_Enabled ? d.m_clrMenuPopupText : d.m_clrMenuBarGrayText);
            p->drawText(vTextRect, text_flags, s.left(t));
            p->setPen(discol);
            p->restore();
        }
        // draw sub menu arrow
        if (menuitem->menuItemType == QStyleOptionMenuItem::SubMenu) 
        {
            int dim = (h - 2 * windowsItemFrame) / 2;
            PrimitiveElement arrow;
            arrow = (opt->direction == Qt::RightToLeft) ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight;
            xpos = x + w - windowsArrowHMargin - windowsItemFrame - dim;
            QRect  vSubMenuRect = visualRect(opt->direction, menuitem->rect, QRect(xpos, y + h / 2 - dim / 2, dim, dim));
            QStyleOptionMenuItem newMI = *menuitem;
            newMI.rect = vSubMenuRect;
            newMI.state = dis ? State_None : State_Enabled;
            proxy()->drawPrimitive(arrow, &newMI, p, widget);
        }
        return true;
    }
    return false;
}

/*! \internal */
bool OfficeStyle::drawFrameMenu(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle)
    if (d.officePaintManager()->drawPanelMenu(opt, p, w))
        return true;
    return CommonStyle::drawFrameMenu(opt, p, w);
}

// for toolBar
/*! \internal */
bool OfficeStyle::drawToolBar(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawToolBar(opt, p, w);
}

/*! \internal */
static void drawArrow(const QStyle* style, const QStyleOptionToolButton* toolbutton,
                      const QRect& rect, QPainter* painter, const QWidget* widget = 0)
{
    QStyle::PrimitiveElement pe;

    switch (toolbutton->arrowType) 
    {
        case Qt::LeftArrow:
                pe = QStyle::PE_IndicatorArrowLeft;
            break;
        case Qt::RightArrow:
                pe = QStyle::PE_IndicatorArrowRight;
            break;
        case Qt::UpArrow:
                pe = QStyle::PE_IndicatorArrowUp;
            break;
        case Qt::DownArrow:
                pe = QStyle::PE_IndicatorArrowDown;
            break;
        default:
            return;
    }

    QStyleOptionToolButton arrowOpt;
    arrowOpt.rect = rect;
    arrowOpt.palette = toolbutton->palette;
    arrowOpt.state = toolbutton->state;
    style->drawPrimitive(pe, &arrowOpt, painter, widget);
}

/*! \internal */
bool OfficeStyle::drawToolButtonLabel(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    if (const QStyleOptionToolButton* toolbutton = qstyleoption_cast<const QStyleOptionToolButton*>(opt))
    {
        QRect rect = toolbutton->rect;
        // Arrow type always overrules and is always shown
        bool hasArrow = toolbutton->features & QStyleOptionToolButton::Arrow;
        if (((!hasArrow && toolbutton->icon.isNull()) && !toolbutton->text.isEmpty())|| toolbutton->toolButtonStyle == Qt::ToolButtonTextOnly) 
        {
            int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
            if (!styleHint(SH_UnderlineShortcut, opt, w))
                alignment |= Qt::TextHideMnemonic;

            QPalette::ColorRole textRole = QPalette::ButtonText;
			// BEGIN ROBLOX CHANGES
            //if (opt->state & State_MouseOver || opt->state & State_Sunken)
            //    textRole = QPalette::HighlightedText;
			// END ROBLOX CHANGES

            QString text = toolbutton->text;
            if (d.officePaintManager()->isTopLevelMenuItemUpperCase(w))
                text = text.toUpper();

            QRect rectText = rect;
            if ((toolbutton->subControls & SC_ToolButtonMenu) || (toolbutton->features & QStyleOptionToolButton::HasMenu))
            {
                int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, opt, w);
                rectText.adjust(0, 0, -mbi, 0);
            }

            proxy()->drawItemText(p, rectText, alignment, toolbutton->palette, opt->state & State_Enabled, text, textRole);
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
                p->setFont(toolbutton->font);
                QRect pr = rect, tr = rect;

                int alignment = Qt::TextShowMnemonic;
                if (!proxy()->styleHint(SH_UnderlineShortcut, opt, w))
                    alignment |= Qt::TextHideMnemonic;

                if (toolbutton->toolButtonStyle == Qt::ToolButtonTextUnderIcon)
                {
                    pr.setHeight(pmSize.height() + 6);
                    tr.adjust(0, pr.height() - 1, 0, -3);
                    if ( !hasArrow )
                        drawItemPixmap(p, pr, Qt::AlignCenter, pm);
                    else 
                        drawArrow(this, toolbutton, pr, p, w);

                    alignment |= Qt::AlignCenter;
                }
                else
                {
                    pr.setWidth(pmSize.width() + 8);
                    tr.adjust(pr.width(), 0, 0, 0);
                    if (!hasArrow) 
                        drawItemPixmap(p, QStyle::visualRect(opt->direction, rect, pr), Qt::AlignCenter, pm);
                    else 
                        drawArrow(this, toolbutton, pr, p, w);
                    alignment |= Qt::AlignLeft | Qt::AlignVCenter;
                }

                if (toolbutton->toolButtonStyle == Qt::ToolButtonTextUnderIcon)
                {
                    QString strFirstRow, strSecondRow;
                    splitString(toolbutton->text, strFirstRow, strSecondRow);

                    QRect rcText(QStyle::visualRect(opt->direction, rect, tr));
                    int height = toolbutton->fontMetrics.height();
                    QRect rcFirstRowText(QPoint(rcText.left(), rcText.bottom() - height*2 + 1), QPoint(rcText.right(), rcText.bottom() - height + 1));

                    if (!strFirstRow.isEmpty())
                    {
                        proxy()->drawItemText(p, rcFirstRowText, alignment, toolbutton->palette,
                                    toolbutton->state & State_Enabled, strFirstRow, QPalette::ButtonText);
                    }

                    if (!strSecondRow.isEmpty()) 
                    {
                        int left = rcText.left();
                        if (toolbutton->subControls & SC_ToolButtonMenu || toolbutton->features & QStyleOptionToolButton::HasMenu)
                            left = opt->rect.left()-5;
                        
                        QRect rcSecondRowText(QPoint(left, rcText.bottom() - height), QPoint(rcText.right(), rcText.bottom()+2));
                        proxy()->drawItemText(p, rcSecondRowText, alignment, toolbutton->palette,
                                    toolbutton->state & State_Enabled, strSecondRow, QPalette::ButtonText);
                    }
                }
                else
                {
                    proxy()->drawItemText(p, QStyle::visualRect(opt->direction, rect, tr), alignment, toolbutton->palette,
                                toolbutton->state & State_Enabled, toolbutton->text, QPalette::ButtonText);
                }
            }
            else
            {
                if (hasArrow) 
                {
                    drawArrow(this, toolbutton, rect, p, w);
                }
                else 
                {
                    QRect pr = rect;
                    if ((toolbutton->subControls & SC_ToolButtonMenu) || (toolbutton->features & QStyleOptionToolButton::HasMenu))
                    {
                        int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, opt, w);
                        pr.setWidth(pr.width() - mbi);
                    }
                    drawItemPixmap(p, pr, Qt::AlignCenter, pm);
                }
            }
        }
        return true;
    }
    return false;
}

/*! \internal */
bool OfficeStyle::drawPanelButtonTool(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    if (const QStyleOptionToolButton* toolbutton = qstyleoption_cast<const QStyleOptionToolButton*>(opt)) 
    {
        bool smallSize = opt->rect.height() < 33;

        bool enabled  = opt->state & State_Enabled;
        bool checked  = opt->state & State_On;
        bool selected = opt->state & State_MouseOver;
        bool mouseInSplit = opt->state & State_MouseOver && toolbutton->activeSubControls & SC_ToolButton;
        bool mouseInSplitDropDown = opt->state & State_MouseOver && toolbutton->activeSubControls & SC_ToolButtonMenu;
        bool pressed  = opt->state & State_Sunken;
        bool popuped  = (toolbutton->activeSubControls & QStyle::SC_ToolButtonMenu) && (opt->state & State_Sunken);

        if (!(toolbutton->features & QStyleOptionToolButton::MenuButtonPopup))
        {
            drawRectangle(p, opt->rect, selected, pressed, enabled, checked, popuped, TypeNormal, BarTop);
            return true;
        }

        int specialOffset = 0;
        QPixmap soImageSplit = smallSize ? cached("ToolbarButtonsSplit22.png") 
                : cached("ToolbarButtonsSplit50.png");
        QPixmap soImageSplitDropDown = smallSize ? cached("ToolbarButtonsSplitDropDown22.png") 
                : cached("ToolbarButtonsSplitDropDown50.png");

        if(soImageSplit.isNull() || soImageSplitDropDown.isNull())
            return false;

        QRect rcButton = opt->rect;
        QRect popupr = subControlRect(QStyle::CC_ToolButton, toolbutton, QStyle::SC_ToolButtonMenu, w);

        QRect rcSplit = smallSize ? QRect(QPoint(rcButton.left(), rcButton.top()), QPoint(rcButton.right() - popupr.width()-2, rcButton.bottom())) 
            : QRect(QPoint(rcButton.left(), rcButton.top()), QPoint(rcButton.right(), rcButton.bottom() - popupr.height()-2));

        int stateSplit = -1;

        if (/*enabledSplit && (selected || pressed || popuped)*/false)
        {
            stateSplit = 4;
        }
        else if (!enabled)
        {
            //            if (isKeyboardSelected(selected)) nStateSplit = 5;
        }
        else if (checked)
        {
            if (popuped) stateSplit = 5;
            else if (!selected && !pressed) stateSplit = 2;
            else if (selected && !pressed) stateSplit = 3;
            else if (/*isKeyboardSelected(bPressed) ||*/ (selected && pressed)) stateSplit = 1;
            else if (pressed) stateSplit = 2;
            else if (selected || pressed) stateSplit = !mouseInSplit ? 5 : 0;
        }
        else
        {
            if (popuped) stateSplit = 5;
            else if (/*isKeyboardSelected(bPressed) ||*/ (selected && pressed)) stateSplit = 1;
            else if (selected || pressed) stateSplit = !mouseInSplit ? 5 : 0;
        }
        stateSplit += specialOffset;

        if (stateSplit != -1)
            drawImage(soImageSplit, *p, rcSplit, sourceRectImage(soImageSplit.rect(), stateSplit, 6+specialOffset),
                      QRect(QPoint(2, 2), QPoint(2, 2)),  QColor(0xFF, 0, 0xFF));

        QRect rcSplitDropDown = smallSize ? QRect(QPoint(rcButton.right() - popupr.width()-1, rcButton.top()), QPoint(rcButton.right(), rcButton.bottom())) :
                                             QRect(QPoint(rcButton.left(), rcButton.bottom() - popupr.height()-1), QPoint(rcButton.right(), rcButton.bottom()));

        int stateSplitDropDown = -1;

        if (/*enabledDropDown && (selected || pressed || popuped)*/false)
        {
            stateSplitDropDown = 3;
        }
        else if (!enabled)
        {
            //            if (isKeyboardSelected(selected)) stateSplitDropDown = 4;
        }
        else if (checked)
        {
            if (popuped) stateSplitDropDown = 2;
            else if (!selected && !pressed) stateSplitDropDown = 2;
            else if (/*isKeyboardSelected(pressed) ||*/ (selected && pressed)) stateSplitDropDown = 0;
            else if (selected || pressed) stateSplitDropDown = !mouseInSplitDropDown ? 4 : 0;
            else stateSplitDropDown = 4;
        }
        else
        {
            if (popuped) stateSplitDropDown = 2;
            else if (/*isKeyboardSelected(pressed) ||*/ (selected && pressed)) stateSplitDropDown = 0;
            else if (selected || pressed) stateSplitDropDown = !mouseInSplitDropDown ? 4 : 0;
        }

        stateSplitDropDown += specialOffset;
        if (stateSplitDropDown != -1)
            drawImage(soImageSplitDropDown, *p, rcSplitDropDown, sourceRectImage(soImageSplitDropDown.rect(), stateSplitDropDown, 5+specialOffset), 
                QRect(QPoint(2, 2), QPoint(2, 2)), QColor(0xFF, 0, 0xFF));
        return true;
    }
    return false;
}

/*! \internal */
bool OfficeStyle::drawToolButton(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    QTN_D(const OfficeStyle);
    if (const QStyleOptionToolButton* toolbutton = qstyleoption_cast<const QStyleOptionToolButton*>(opt))
    {
        if (d.officePaintManager()->drawGalleryToolButton(opt, p, widget))
            return true;

        QRect /*button,*/ menuarea;
//        button = proxy()->subControlRect(CC_ToolButton, toolbutton, SC_ToolButton, widget);
        menuarea = proxy()->subControlRect(CC_ToolButton, toolbutton, SC_ToolButtonMenu, widget);

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

        proxy()->drawPrimitive(PE_PanelButtonTool, toolbutton, p, widget);

        QStyleOptionToolButton label = *toolbutton;
        label.state = flags;
        proxy()->drawControl(CE_ToolButtonLabel, &label, p, widget);

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
            {
                int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, toolbutton, widget);
                rcArrow = QRect(ir.right() - mbi - 1, opt->rect.top()  + (ir.height() - mbi) / 2 + ((ir.height() - mbi) % 2), mbi, mbi);
            }

            tool.rect = rcArrow;
            tool.state = mflags;

            proxy()->drawPrimitive(PE_IndicatorArrowDown, &tool, p, widget);
        } 
        else if (toolbutton->features & QStyleOptionToolButton::HasMenu) 
        {
            QStyleOptionToolButton newBtn = *toolbutton;
            if (toolbutton->toolButtonStyle == Qt::ToolButtonTextUnderIcon)
            {
                QString strFirstRow, strSecondRow;
                splitString(toolbutton->text, strFirstRow, strSecondRow);
                newBtn.rect = QRect(QPoint(strSecondRow.isEmpty() ? opt->rect.width()/2 - 2 : opt->rect.right()-8, opt->rect.bottom()-8), QSize(5, 5));
            }
            else
            {
                int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, toolbutton, widget);
                QRect ir = menuarea;
//                QStyleOptionButton newBtn = *toolbutton;
                newBtn.rect = QRect(ir.right() - mbi - 1, /*(ir.height() - mbi)/2*/opt->rect.top()  + (ir.height() - mbi) / 2 + ((ir.height() - mbi) % 2), mbi, mbi);
//                newBtn.rect = QRect(ir.right() - mbi - 1, (ir.height() - mbi)/2, mbi, mbi);
/*
                QRect ir = menuarea;
                int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, toolbutton, widget);

                if (!isDPIAware())
                    mbi = (double)mbi/((double)(DrawHelpers::getDPIToPercent()/100.0));

                newBtn.rect = QRect(QPoint(ir.right() - mbi, ir.y() + (ir.height()-mbi)/2), QSize(mbi, ir.height()));
*/
            }
            proxy()->drawPrimitive(PE_IndicatorArrowDown, &newBtn, p, widget);
        }
        return true;
    }
    return false;
}

/*! \internal */
bool OfficeStyle::drawGroupControlEntry(const QStyleOption *, QPainter *, const QWidget *) const
{ 
    return false;
}

/*! \internal */
bool OfficeStyle::drawPanelButtonCommand(const QStyleOption* opt, QPainter* p, const QWidget* w) const 
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawPanelButtonCommand(opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawIndicatorToolBarSeparator(const QStyleOption* opt, QPainter* p, const QWidget* w) const 
{
    QTN_D(const OfficeStyle);
    d.officePaintManager()->drawIndicatorToolBarSeparator(opt, p, w);
    return false;
}

/*! \internal */
bool OfficeStyle::drawIndicatorCheckRadioButton(PrimitiveElement element, const QStyleOption* option, 
                                                QPainter* painter, const QWidget* widget) const
{
    if (const QStyleOptionButton* but = qstyleoption_cast<const QStyleOptionButton*>(option))
    {
        QStyleOptionButton but1(*but);
        but1.rect.setWidth(13); but1.rect.setHeight(13);
        if (paintAnimation(tp_PrimitiveElement, (int)element, &but1, painter, widget))
            return true;
    }

    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawIndicatorCheckRadioButton(element, option, painter, widget);
}

/*! \internal */
bool OfficeStyle::drawFrameDefaultButton(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    Q_UNUSED(opt);
    Q_UNUSED(p);
    Q_UNUSED(widget);
    return true;
}

/*! \internal */
bool OfficeStyle::drawIndicatorToolBarHandle(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D(const OfficeStyle)

    p->translate(opt->rect.x(), opt->rect.y());
    QRect rect(opt->rect);
    if (opt->state & State_Horizontal) 
    {
        for (int y = 4; y < rect.height() - 4; y += 4)
        {
            p->fillRect(QRect(QPoint(3, y+1), QPoint(4, y + 2)), QColor(234, 251, 251));
            p->fillRect(QRect(QPoint(2, y), QPoint(3, y + 1)), d.m_ToolbarGripper);
        }
    } 
    else 
    {
        for (int x = 4; x < opt->rect.width() - 4; x += 4)
        {
            p->fillRect(QRect(QPoint(x + 1, 3), QPoint(x + 2, 4)), QColor(234, 251, 251));
            p->fillRect(QRect(QPoint(x, 2), QPoint(x + 1, 3)), d.m_ToolbarGripper);
        }
    }
    return true;
}

// for SizeGrip
/*! \internal */
bool OfficeStyle::drawSizeGrip(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawSizeGrip(opt, p, w);
}

/*! \internal */
static QRect getSourceH(QRect rcSrc, int nState, int nCount)
{
    QRect rcImage(0, 0, rcSrc.width() / nCount, rcSrc.height());
    rcImage.translate(nState * rcImage.width(), 0);
    return rcImage;
}

// for Slider
/*! \internal */
bool OfficeStyle::drawSlider(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const
{
    if (const QStyleOptionSlider* slider = qstyleoption_cast<const QStyleOptionSlider*>(opt))
    {
        QRect groove = proxy()->subControlRect(CC_Slider, opt, SC_SliderGroove, w);
        QRect handle = proxy()->subControlRect(CC_Slider, opt, SC_SliderHandle, w);

        int thickness  = proxy()->pixelMetric(PM_SliderControlThickness, slider, w);
        int len        = proxy()->pixelMetric(PM_SliderLength, slider, w);
        int ticks = slider->tickPosition;

        if ((opt->subControls & SC_SliderHandle)) 
        {
            int stateId = 0;
            if (!(slider->state & State_Enabled))
                stateId = 4;
            else if (slider->activeSubControls & SC_SliderHandle && (slider->state & State_Sunken))
                stateId = 2;
            else if (slider->activeSubControls & SC_SliderHandle && (slider->state & State_MouseOver))
                stateId = 1;
            else if (slider->state & State_HasFocus)
                stateId = 3;
            else
                stateId = 0;

            QPixmap hndl;
            QRect rcSrc;
            QRect rcSizingMargins = QRect(QPoint(0, 0), QPoint(0, 0));
            if (slider->orientation == Qt::Horizontal)
            {
                if (slider->tickPosition == QSlider::TicksAbove)
                {
                    hndl = cached("TrackBarUp13.png");
                    rcSizingMargins = QRect(QPoint(3, 6), QPoint(6, 10));
                    rcSrc = sourceRectImage(hndl.rect(), stateId, 5);
                }
                else if (slider->tickPosition == QSlider::TicksBelow)
                {
                    rcSizingMargins = QRect(QPoint(3, 6), QPoint(6, 10));
                    hndl = cached("TrackBarDown13.png");
                    rcSrc = sourceRectImage(hndl.rect(), stateId, 5);
                }
                else
                {
                    rcSizingMargins = QRect(QPoint(5, 5), QPoint(5, 5));
                    hndl = cached("TrackbarVertical.png");
                    rcSrc = getTheme() == Office2010Silver || getTheme() == Office2007Aqua ? sourceRectImage(hndl.rect(), stateId, 5) : getSourceH(hndl.rect(), stateId, 5);
                }
            }
            else if (slider->orientation == Qt::Vertical)
            {
                if (slider->tickPosition == QSlider::TicksLeft)
                {
                    rcSizingMargins = QRect(QPoint(10, 3), QPoint(6, 6));
                    hndl = cached("TrackBarLeft13.png");
                    rcSrc = getTheme() == Office2010Silver || getTheme() == Office2007Aqua ? sourceRectImage(hndl.rect(), stateId, 5) : getSourceH(hndl.rect(), stateId, 5);
                }
                else if (slider->tickPosition == QSlider::TicksRight)
                {
                    rcSizingMargins = QRect(QPoint(6, 3), QPoint(10, 6));
                    hndl = cached("TrackbarRight13.png");
                    rcSrc = getTheme() == Office2010Silver || getTheme() == Office2007Aqua ? sourceRectImage(hndl.rect(), stateId, 5) : getSourceH(hndl.rect(), stateId, 5);
                }
                else
                {
                    rcSizingMargins = QRect(QPoint(5, 5), QPoint(5, 5));
                    hndl = cached("TrackbarHorizontal.png");
                    rcSrc = sourceRectImage(hndl.rect(), stateId, 5);
                }
            }

            if (hndl.isNull())
                return false;

            if ((slider->subControls & SC_SliderGroove) && groove.isValid()) 
            {
                int mid = thickness / 2;

                if (ticks & QSlider::TicksAbove)
                    mid += len / 8;
                if (ticks & QSlider::TicksBelow)
                    mid -= len / 8;

                p->setPen(slider->palette.shadow().color());
                if (slider->orientation == Qt::Horizontal) 
                {
                    qDrawWinPanel(p, groove.x(), groove.y() + mid - 2, groove.width(), 4, slider->palette, true);
                    p->drawLine(groove.x() + 1, groove.y() + mid - 1, groove.x() + groove.width() - 3, groove.y() + mid - 1);
                } 
                else 
                {
                    qDrawWinPanel(p, groove.x() + mid - 2, groove.y(), 4, groove.height(), slider->palette, true);
                    p->drawLine(groove.x() + mid - 1, groove.y() + 1, groove.x() + mid - 1, groove.y() + groove.height() - 3);
                }
            }

            if (slider->subControls & SC_SliderTickmarks) 
            {
                QStyleOptionSlider tmpSlider = *slider;
                tmpSlider.subControls = SC_SliderTickmarks;
                QCommonStyle::drawComplexControl(CC_Slider, &tmpSlider, p, w);
            }

            drawPixmap(hndl, *p, handle, rcSrc, false, rcSizingMargins);
            return true;
        }
    }
    return false;
}

// for ScrollBar
/*! \internal */
bool OfficeStyle::drawScrollBar(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawScrollBar(opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawScrollBarLine(ControlElement element, const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawScrollBarLine(element, opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawScrollBarSlider(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawScrollBarSlider(opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawScrollBarPage(ControlElement element, const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawScrollBarPage(element, opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawControlEdit(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawControlEdit(opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawFrameLineEdit(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawFrameLineEdit(opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawComboBox(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawComboBox(opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawDockWidgetTitle(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle)
    if (const QStyleOptionDockWidget* dwOpt = qstyleoption_cast<const QStyleOptionDockWidget*>(opt)) 
    {
        bool verticalTitleBar = true;//v2 == 0 ? false : v2->verticalTitleBar;
        bool active = false; //opt->state & State_HasFocus;

        // [1]
        DrawHelpers::drawGradientFill(*p, opt->rect, active ? d.m_palActiveCaption.color(QPalette::Light) : d.m_palNormalCaption.color(QPalette::Light), 
            active ? d.m_palActiveCaption.color(QPalette::Dark) : d.m_palNormalCaption.color(QPalette::Dark), verticalTitleBar);

        bool floating = false;
        if (dwOpt->movable) 
        {
            if (w && w->isWindow()) 
                floating = true;
        }

        if (!dwOpt->title.isEmpty()) 
        {
            QFont oldFont = p->font();

            if (floating) 
            {
                QFont font = oldFont;
                font.setBold(true);
                p->setFont(font);
            }

            QPalette palette = dwOpt->palette;
            palette.setColor(QPalette::Window, d.m_clrNormalCaptionText);
            palette.setColor(QPalette::WindowText, d.m_clrActiveCaptionText);
            palette.setColor(QPalette::BrightText, d.m_clrActiveCaptionText);

            QRect titleRect = subElementRect(SE_DockWidgetTitleBarText, opt, w);
            titleRect.adjust(4, 0, 0, 0);

            proxy()->drawItemText(p, titleRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, palette,
                    dwOpt->state & State_Enabled, dwOpt->title,
                    floating ? (active ? QPalette::BrightText : QPalette::Window) : active ? QPalette::WindowText : QPalette::Window);
            p->setFont(oldFont);
        }
        return true;
    }
    return false;
}

// docksplitter
/*! \internal */
bool OfficeStyle::drawIndicatorDockWidgetResizeHandle(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawIndicatorDockWidgetResizeHandle(opt, p, w);
}

/*! \internal */
static QPixmap invertPixmap(const QPixmap& pm, const QColor& invertColor )
{
    if (qRgb(0,0,0) == invertColor.rgb())
        return pm;

    QImage image = pm.toImage();
    int w = pm.width();
    int h = pm.height();

    for (int y = 0 ; y < h ; y++)
    {
        for (int x = 0; x < w ; x++)
        {
            QRgb px = image.pixel(x, y);
            if (px == qRgb(0,0,0))
                image.setPixel(x, y, invertColor.rgb());
        }
    }
    return QPixmap::fromImage(image);
}

// MDI
/*! \internal */
bool OfficeStyle::drawMdiControls(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const
{
    QStyleOptionToolButton btnOpt;
    btnOpt.QStyleOption::operator=(*opt);
    if (opt->subControls & QStyle::SC_MdiCloseButton)
    {
        if (opt->activeSubControls & QStyle::SC_MdiCloseButton && (opt->state & State_Sunken))
        {
            btnOpt.state |= State_Sunken;
            btnOpt.state &= ~State_Raised;
        } 
        else
        {
            btnOpt.state |= State_Raised;
            btnOpt.state &= ~State_Sunken;
        }
        btnOpt.rect = proxy()->subControlRect(CC_MdiControls, opt, SC_MdiCloseButton, w);
        if (opt->activeSubControls & QStyle::SC_MdiCloseButton)
            drawRectangle(p, btnOpt.rect, opt->state & State_MouseOver, opt->state & State_Sunken,
                opt->state & State_Enabled, false, false, TypeMenuBar, BarTop);

        QPixmap pm = standardIcon(SP_TitleBarCloseButton).pixmap(16, 16);
        pm = invertPixmap(pm, getTextColor((opt->state & State_MouseOver)&&(opt->activeSubControls & QStyle::SC_MdiCloseButton),
            /*opt->state & State_Sunken*/false,  opt->state & State_Enabled, false, false, TypeMenuBar, BarNone));
        proxy()->drawItemPixmap(p, btnOpt.rect, Qt::AlignCenter, pm);
    }

    if (opt->subControls & QStyle::SC_MdiNormalButton)
    {
        if (opt->activeSubControls & QStyle::SC_MdiNormalButton && (opt->state & State_Sunken))
        {
            btnOpt.state |= State_Sunken;
            btnOpt.state &= ~State_Raised;
        } 
        else 
        {
            btnOpt.state |= State_Raised;
            btnOpt.state &= ~State_Sunken;
        }
        btnOpt.rect = proxy()->subControlRect(CC_MdiControls, opt, SC_MdiNormalButton, w);
        if (opt->activeSubControls & QStyle::SC_MdiNormalButton)
            drawRectangle(p, btnOpt.rect, opt->state & State_MouseOver, opt->state & State_Sunken,
            opt->state & State_Enabled, false, false, TypeMenuBar, BarTop);

        QPixmap pm = standardIcon(SP_TitleBarNormalButton).pixmap(16, 16);
        pm = invertPixmap(pm, getTextColor((opt->state & State_MouseOver)&&(opt->activeSubControls & QStyle::SC_MdiNormalButton),
            /*opt->state & State_Sunken*/false, opt->state & State_Enabled, false, false, TypeMenuBar, BarNone));

        proxy()->drawItemPixmap(p, btnOpt.rect, Qt::AlignCenter, pm);
    }
    if (opt->subControls & QStyle::SC_MdiMinButton) 
    {
        if (opt->activeSubControls & QStyle::SC_MdiMinButton && (opt->state & State_Sunken))
        {
            btnOpt.state |= State_Sunken;
            btnOpt.state &= ~State_Raised;
        }
        else
        {
            btnOpt.state |= State_Raised;
            btnOpt.state &= ~State_Sunken;
        }
        btnOpt.rect = proxy()->subControlRect(CC_MdiControls, opt, SC_MdiMinButton, w);

        if (opt->activeSubControls & QStyle::SC_MdiMinButton)
            drawRectangle(p, btnOpt.rect, opt->state & State_MouseOver, opt->state & State_Sunken,
            opt->state & State_Enabled, false, false, TypeMenuBar, BarTop);

        QPixmap pm = standardIcon(SP_TitleBarMinButton).pixmap(16, 16);
        pm = invertPixmap(pm, getTextColor((opt->state & State_MouseOver)&&(opt->activeSubControls & QStyle::SC_MdiMinButton),
            /*opt->state & State_Sunken*/false, opt->state & State_Enabled, false, false, TypeMenuBar, BarNone));
        proxy()->drawItemPixmap(p, btnOpt.rect, Qt::AlignCenter, pm);
    }
    return true;
}

// for TabBar
/*! \internal */
bool OfficeStyle::drawTabBarTab(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    Q_UNUSED(widget);
    if (const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(opt))
    {
        proxy()->drawControl(CE_TabBarTabShape, tab, p, widget);
        proxy()->drawControl(CE_TabBarTabLabel, tab, p, widget);
        return true; 
    }
    return false; 
}

/*! \internal */
bool OfficeStyle::drawTabBarTabShape(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawTabBarTabShape(opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawTabBarTabLabel(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    if (const QStyleOptionTabV3* tab = qstyleoption_cast<const QStyleOptionTabV3*>(opt))
    {
        QPalette palette = opt->palette;  
        QColor color =  palette.color(QPalette::WindowText);

        QColor col = helper().getColor(tr("TabManager"), tr("TextColor"));
        if (col.isValid())
            color = col;

        if (!(tab->state & State_Selected) && (widget && qobject_cast<const QMainWindow*>(widget->parentWidget())))
        {
            QColor col = helper().getColor(tr("TabManager"), tr("NormalTextColor"));
            if (col.isValid())
                color = col;
        }

        if (color.isValid())
            palette.setColor(QPalette::WindowText, color);

        QStyleOptionTabV3 optTab = *tab;
        optTab.palette = palette;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        if (defaultStyle())
            defaultStyle()->drawControl(CE_TabBarTabLabel, &optTab, p, widget);
        else
            QCommonStyle::drawControl(CE_TabBarTabLabel, &optTab, p, widget);
#else
            QWindowsStyle::drawControl(CE_TabBarTabLabel, &optTab, p, widget);
#endif

        return true;
    }
    return false;
}

// for SpinBox
/*! \internal */
bool OfficeStyle::drawSpinBox(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawSpinBox(opt, p, w);
}

// for ProgressBar
/*! \internal */
bool OfficeStyle::drawProgressBarGroove(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D(const OfficeStyle)

    p->save();
    QRect r = opt->rect;
    p->fillRect(r, d.m_clrControlEditNormal);
    QPen savePen = p->pen();
    p->setPen(d.m_clrEditCtrlBorder);
    r.adjust(0, 0, -1, -1);
    p->drawRect(r);
    p->setPen(savePen);
    p->restore();
    return true;
}

/*! \internal */
bool OfficeStyle::drawToolBoxTab(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{ 
    if (const QStyleOptionToolBox* tb = qstyleoption_cast<const QStyleOptionToolBox*>(opt))
    {
        proxy()->drawControl(CE_ToolBoxTabShape, tb, p, w);
        proxy()->drawControl(CE_ToolBoxTabLabel, tb, p, w);
        return true;
    }
    return false;
}

/*! \internal */
bool OfficeStyle::drawToolBoxTabShape(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawToolBoxTabShape(opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawToolBoxTabLabel(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    if (const QStyleOptionToolBoxV2* tb = qstyleoption_cast<const QStyleOptionToolBoxV2*>(opt)) 
    {
        QStyleOptionToolBoxV2 optTB = *tb;

        optTB.palette.setColor(QPalette::ButtonText, helper().getColor(tr("ShortcutBar"), tr("NormalText")));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        if (defaultStyle())
            defaultStyle()->drawControl(CE_ToolBoxTabLabel, &optTB, p, widget);
        else
            QCommonStyle::drawControl(CE_ToolBoxTabLabel, &optTB, p, widget);
#else
            QWindowsStyle::drawControl(CE_ToolBoxTabLabel, &optTB, p, widget);
#endif
        return true;
    }
    return false;
}

// for ViewItem
/*! \internal */
bool OfficeStyle::drawItemViewItem(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    if (defaultStyle())
        defaultStyle()->drawControl(CE_ItemViewItem, opt, p, widget);
    else
        QCommonStyle::drawControl(CE_ItemViewItem, opt, p, widget);
#else
        QWindowsStyle::drawControl(CE_ItemViewItem, opt, p, widget);
#endif
    return true;
}

/*! \internal */
bool OfficeStyle::drawPanelItemViewItem(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    QTN_D(const OfficeStyle);

    if (qobject_cast<const QTableView*>(widget))
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        if (defaultStyle())
            defaultStyle()->drawPrimitive(PE_PanelItemViewItem, opt, p, widget);
        else
            QCommonStyle::drawPrimitive(PE_PanelItemViewItem, opt, p, widget);
#else
        QWindowsStyle::drawPrimitive(PE_PanelItemViewItem, opt, p, widget);
#endif
        return true;
    }

    return d.officePaintManager()->drawPanelItemViewItem(opt, p, widget);
}

/*! \internal */
bool OfficeStyle::drawHeader(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    if (const QStyleOptionHeader* header = qstyleoption_cast<const QStyleOptionHeader*>(opt))
    {
        QRegion clipRegion = p->clipRegion();
        p->setClipRect(opt->rect);

        proxy()->drawControl(CE_HeaderSection, header, p, widget);

        QStyleOptionHeader subopt = *header;
        subopt.rect = subElementRect(SE_HeaderLabel, header, widget);

        if (subopt.rect.isValid())
        {
//            QPalette palette = header->palette;
//            QColor clrNormalText = helper().getColor(tr("ListBox"), tr("NormalText"));
//            palette.setColor(QPalette::All, QPalette::ButtonText, clrNormalText);

            QStyleOptionHeader headerLabelOpt = subopt;
//            headerLabelOpt.palette = palette;

            proxy()->drawControl(CE_HeaderLabel, &headerLabelOpt, p, widget);
        }

        if (header->sortIndicator != QStyleOptionHeader::None) 
        {
            subopt.rect = subElementRect(SE_HeaderArrow, opt, widget);
            proxy()->drawPrimitive(PE_IndicatorHeaderArrow, &subopt, p, widget);
        }
        p->setClipRegion(clipRegion);
        return true;
    }
    return false;
}

/*! \internal */
bool OfficeStyle::drawHeaderSection(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawHeaderSection(opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawHeaderEmptyArea(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawHeaderEmptyArea(opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawIndicatorHeaderArrow(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawIndicatorHeaderArrow(opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawIndicatorArrow(PrimitiveElement pe, const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawIndicatorArrow(pe, opt, p, w);
}

/*! \internal */
bool OfficeStyle::drawPanelTipLabel(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawPanelTipLabel(opt, p, w);
}

/*! \internal */
void OfficeStyle::drawRectangle(QPainter* p, const QRect& rect, bool selected, bool pressed, bool enabled, bool checked, bool popuped,
    BarType barType, BarPosition barPos) const
{
    QTN_D(const OfficeStyle);
    return d.officePaintManager()->drawRectangle(p, rect, selected, pressed, enabled, checked, popuped, barType, barPos);
}

/*! \internal */
void OfficeStyle::drawSplitButtonPopup(QPainter* p, const QRect& rect, bool selected, bool enabled, bool popuped) const
{
    Q_UNUSED(p);
    Q_UNUSED(rect);
    Q_UNUSED(selected);
    Q_UNUSED(enabled);
    Q_UNUSED(popuped);
}

/*! \internal */
void OfficeStyle::drawLabelGallery(QPainter* p, RibbonGalleryItem* item, const QRect& rect)
{
    QTN_D(const OfficeStyle)
    QRect rectItem = rect;
    p->fillRect(rect, d.m_clrControlGalleryLabel);
    p->fillRect(rectItem.left(), rectItem.bottom() - 1, rectItem.width(), 1, QColor(197, 197, 197));

    int alignment = 0;
    alignment |= Qt::TextSingleLine | Qt::AlignVCenter;

    QPalette palette;
    palette.setColor(QPalette::WindowText, d.m_clrMenuPopupText);
    rectItem.adjust(10, 0, 0, 0);
    const QFont& saveFont = p->font();
    QFont font(saveFont);
    font.setBold(true);
    p->setFont(font);
    proxy()->drawItemText(p, rectItem, alignment, palette, true, item->caption(), QPalette::WindowText);
    p->setFont(saveFont);
}

/*! \internal */
QColor OfficeStyle::getTextColor(bool selected, bool pressed, bool enabled, bool checked, bool popuped, BarType barType, BarPosition barPosition) const
{
    Q_UNUSED(barPosition);
    QTN_D(const OfficeStyle)

    if (barType == TypeMenuBar && !selected && enabled && !pressed && !checked && !popuped)
        return d.m_clrMenuBarText;

    if (barType == TypePopup)
        return enabled ? d.m_clrMenuPopupText : d.m_clrMenuBarGrayText;
//        return enabled ? m_clrMenuBarText : m_clrMenuBarGrayText;

    return enabled ? d.m_clrToolBarText : d.m_clrToolBarGrayText;
}

/*! \reimp */
bool OfficeStyle::eventFilter(QObject* watched, QEvent* event)
{
    return CommonStyle::eventFilter(watched, event);
}

/*! \internal */
QWidgetList OfficeStyle::allWidgets() const
{
    return qApp->allWidgets();
}

/*! \internal */
bool OfficeStyle::isNativeDialog(const QWidget* wid) const
{
    return ::isParentDialog(wid) && isDialogsIgnored();
}

/*!
    \class Qtitan::OfficeStylePlugin
    \internal
*/
QStringList OfficeStylePlugin::keys() const
{
    return QStringList() << "OfficeStyle";
}

QStyle* OfficeStylePlugin::create( const QString& key )
{
    if ( key.toLower() == QLatin1String("officestyle"))
        return new OfficeStyle();
    return Q_NULL;
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#else
QObject* qt_plugin_instance_officestyle()
{
    static QObject* instance = Q_NULL;
    if ( !instance )
        instance = new OfficeStylePlugin();
    return instance;
}

Q_IMPORT_PLUGIN(officestyle)
#endif
