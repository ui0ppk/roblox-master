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
#include <QApplication>
#include <QDesktopWidget>
#include <QMainWindow>
#include <QPainter>
#include <QStyle>
#include <QBitmap>
#include <QComboBox>
#include <QDialog>
#include <QToolBar>
#include <QToolButton>
#include <QListWidget>
#include <QMdiArea>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <qdrawutil.h>
#endif /* Q_OS_WIN*/

#include "QtnOfficeStylePrivate.h"
#include "QtnStyleHelpers.h"
#include "QtnRibbonGallery.h"

#include "../src/ribbon/QtnOfficePopupMenu.h"


using namespace Qtitan;

static const int iImageWidth = 9;
static const int iImageHeight = 9;
static const int iTextMargin = 3;
static const QColor clrTransparent = QColor(255, 0, 255);

static inline bool isAncestor(const QObject* child)
{
    static QString className = QString("Qtitan::RibbonBar");
    while (child) 
    {
        if (child->metaObject()->className() == className)
            return true;
        child = child->parent();
    }
    return false;
}


/*!
\class Qtitan::OfficePaintManager
\internal
*/
OfficePaintManager::OfficePaintManager(CommonStyle* baseStyle)
    : CommonPaintManager(baseStyle)
{
}

OfficePaintManager::~OfficePaintManager()
{
}

QString OfficePaintManager::theme(const QString& str) const
{
    return ((OfficeStyle*)baseStyle())->qtn_d().theme(str);
}

OfficeStyle::Theme OfficePaintManager::getTheme() const
{
    return ((OfficeStyle*)baseStyle())->getTheme();
}

QColor OfficePaintManager::textColor(bool selected, bool pressed, bool enabled, bool checked, bool popuped, BarType barType, BarPosition barPosition) const
{
    return ((OfficeStyle*)baseStyle())->getTextColor(selected, pressed, enabled, checked, popuped, barType, barPosition);
}

/*! \internal */
bool OfficePaintManager::isStyle2010() const
{
    return ((OfficeStyle*)baseStyle())->isStyle2010();
}

/*! \internal */
bool OfficePaintManager::drawWorkspace(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle)

    if (opt == Q_NULL)
        return false;

    QPixmap soWorkspace = cached("WorkspaceTopLeft.png");
    if (!soWorkspace.isNull())
    {
        QRect rc = opt->rect;
        QRect rcSrc = soWorkspace.rect();
        QRect rcTopLeft = opt->rect;
        rcTopLeft.setBottom(rcTopLeft.top() + rcSrc.height());
        rcTopLeft.setRight(rcTopLeft.left() + qMax(rcTopLeft.width(), rcSrc.width()));
        drawImage(soWorkspace, *p, rcTopLeft, rcSrc, QRect(QPoint(rcSrc.width() - 1, 0), QPoint(0, 0)));

        QRect rcFill(QPoint(rc.left(), rc.top() + rcSrc.height()), QPoint(rc.right(), rc.bottom()));
        QRect rcFillTop(QPoint(rcFill.left(), rcFill.top()), QPoint(rcFill.right() + 1, rcFill.top() + rcFill.height() * 2 / 3));
        QRect rcFillBottom(QPoint(rcFill.left(), rcFillTop.bottom()), QPoint(rcFill.right() + 1, rcFill.bottom()));
        DrawHelpers::drawGradientFill(*p, rcFillTop, d.m_clrWorkspaceClientTop, d.m_clrWorkspaceClientMiddle, true);
        DrawHelpers::drawGradientFill(*p, rcFillBottom, d.m_clrWorkspaceClientMiddle, d.m_clrWorkspaceClientBottom, true);
        return true;
    }
    return false;
}

// for TitleBar
/*! \internal */
bool OfficePaintManager::drawToolBar(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle)
    if (const QStyleOptionToolBar* toolbar = qstyleoption_cast<const QStyleOptionToolBar*>(opt)) 
    {
        if (toolbar->state & QStyle::State_Horizontal)
        {
            if (w->parentWidget())
                p->fillRect(toolbar->rect, d.m_clrDockBar);

            QRect rcFill = toolbar->rect;
            rcFill.setTop(rcFill.top()+1);
            // [1]
            DrawHelpers::drawGradientFill(*p, rcFill, d.m_clrCommandBar.color(QPalette::Light), d.m_clrCommandBar.color(QPalette::Dark), true);

            p->fillRect(QRect(QPoint(rcFill.left(), rcFill.bottom()), QSize(rcFill.width(), rcFill.height() - rcFill.height() - 2)), d.m_clrCommandBar.color(QPalette::Dark));

            p->fillRect(QRect(QPoint(toolbar->rect.left() + 5, toolbar->rect.bottom() - 1), QSize(toolbar->rect.width()/* - 7*/, 1)), d.m_clrCommandBar.color(QPalette::Shadow));

            p->fillRect(QRect(QPoint(toolbar->rect.right(), toolbar->rect.top() - 1), QSize(1, toolbar->rect.height())), d.m_clrCommandBar.color(QPalette::Shadow));
        }
        else
        {
            if (w->parentWidget())
            {
                if (toolbar->toolBarArea & Qt::RightToolBarArea)
                    p->fillRect(toolbar->rect, d.m_clrCommandBar.color(QPalette::Dark));
                else
                    p->fillRect(toolbar->rect, d.m_clrDockBar);
            }
            QRect rcFill = toolbar->rect;
            rcFill.setLeft(rcFill.left() + 1);
            // [1]
            DrawHelpers::drawGradientFill(*p, rcFill, d.m_clrCommandBar.color(QPalette::Light), d.m_clrCommandBar.color(QPalette::Dark), false);

            p->fillRect(QRect(QPoint(toolbar->rect.right() - 1, toolbar->rect.top() + 1), QSize(1, toolbar->rect.height()/* - 7*/)), d.m_clrCommandBar.color(QPalette::Shadow));

            p->fillRect(QRect(QPoint(toolbar->rect.left(), toolbar->rect.bottom()), QSize(toolbar->rect.width(), 1)), d.m_clrCommandBar.color(QPalette::Shadow));
        }
        return true;
    }
    return false;
}

/*! \internal */
bool OfficePaintManager::drawIndicatorToolBarSeparator(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    Q_UNUSED(p);
    QTN_D_STYLE(OfficeStyle)

    QStyleOption* option = (QStyleOption*)opt;
    option->palette.setColor(QPalette::Dark, d.m_clrToolbarSeparator);
    option->palette.setColor(QPalette::Light, QColor(255, 255, 255));
    return true; 
}

// for DockWidget
/*! \internal */
bool OfficePaintManager::drawIndicatorDockWidgetResizeHandle(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(OfficeStyle)

    QRect rcFrame = w->rect();
    float fHeight = (float)rcFrame.height();

    QRgb clr1 = DrawHelpers::blendColors(d.m_palSplitter.color(QPalette::Light).rgb(), 
        d.m_palSplitter.color(QPalette::Dark).rgb(), float(opt->rect.bottom() - rcFrame.top()) / fHeight);

    QRgb clr2 = DrawHelpers::blendColors(d.m_palSplitter.color(QPalette::Light).rgb(), 
        d.m_palSplitter.color(QPalette::Dark).rgb(), float(opt->rect.top() - rcFrame.top()) / fHeight);

    // [1]
    DrawHelpers::drawGradientFill(*p, opt->rect, clr1, clr2, true);
    return true;
}

// for menuBar
/*! \internal */
bool OfficePaintManager::drawPanelMenu(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D_STYLE(const OfficeStyle)
    QPixmap soPopupBarFrame = cached("PopupBarFrame.png");
    if (!soPopupBarFrame.isNull())
        drawImage(soPopupBarFrame, *p, opt->rect, soPopupBarFrame.rect(), QRect(4, 4, 4, 4));

    bool showGripper = true;
    // draw a Gripper or not
    QVariant var = w->property("showGripper");
    if (!var.isNull())
        showGripper = var.toBool();

    if(showGripper)
    {
        const int nMenuPanelWidth = baseStyle()->pixelMetric(QStyle::PM_MenuPanelWidth, opt, w);
        const int nIconSize = baseStyle()->pixelMetric(QStyle::PM_ToolBarIconSize, opt, w)+2;
        QRect rcBorders(QPoint(nMenuPanelWidth, nMenuPanelWidth), QPoint(nMenuPanelWidth, nMenuPanelWidth));

        int x = rcBorders.left()-1;
        int y = rcBorders.top();
        int cx = nIconSize;
        int cy = opt->rect.bottom() - rcBorders.top() - rcBorders.bottom();
        // [1]
        DrawHelpers::drawGradientFill(*p, QRect(QPoint(x + 1, y), QSize(cx - 1, cy)), d.m_clrMenuGripper, d.m_clrMenuGripper, true);

        p->fillRect(x + cx - 1, y, 1, cy + 1, d.m_clrMenuPopupGripperShadow);
        p->fillRect(x + cx, y, 1, cy + 1, QColor(245, 245, 245));
    }
    return true;
}

/*! \internal */
bool OfficePaintManager::drawFrameMenu(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(opt);
    Q_UNUSED(p);
    if (qobject_cast<const QToolBar*>(w))
        return true;
    return false;
}

/*! \internal */
bool OfficePaintManager::drawIndicatorMenuCheckMark(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    bool dis = !(opt->state & QStyle::State_Enabled);
    QPixmap soMenuCheckedItem = cached("MenuCheckedItem.png");
    QRect rcSrc = sourceRectImage(soMenuCheckedItem.rect(), !dis ? 0 : 1, 2);

    const int iconSize = baseStyle()->proxy()->pixelMetric(QStyle::PM_SmallIconSize, opt, w) + 4;
    QRect rc(QPoint(opt->rect.left() + opt->rect.width()/2 - iconSize/2, 
    opt->rect.top() + opt->rect.height()/2 - iconSize/2), QSize(iconSize, iconSize));
    drawImage(soMenuCheckedItem, *p, rc, rcSrc, QRect(QPoint(2, 2), QPoint(2, 2)));

    QPixmap soMenuCheckedItemMark = cached("MenuCheckedItemMark.png");
    rcSrc = sourceRectImage(soMenuCheckedItemMark.rect(), !dis ? 0 : 1, 4);
    QRect rcDest(QPoint((rc.left() + rc.right()-rcSrc.width())/2+1, (rc.top() + rc.bottom() - rcSrc.height())/2+1), rcSrc.size() );
    drawImage(soMenuCheckedItemMark, *p, rcDest, rcSrc, QRect(QPoint(0, 0), QPoint(0, 0)), QColor(0xFF, 0, 0xFF));
    return true;
}

// for Buttons
/*! \internal */
bool OfficePaintManager::drawIndicatorCheckRadioButton(QStyle::PrimitiveElement element, const QStyleOption* option, 
                                                       QPainter* painter, const QWidget* widget) const
{
    Q_UNUSED(widget);
    bool isRadio = (element == QStyle::PE_IndicatorRadioButton);
    QPixmap soImage = cached(isRadio ? "ToolbarButtonRadioButton.png" : "ToolbarButtonCheckBox.png");
    int state = 0;
    bool enabled  = option->state & QStyle::State_Enabled;
    bool checked  = option->state & QStyle::State_On;
    bool noChange = option->state & QStyle::State_NoChange;
    bool selected = option->state & QStyle::State_MouseOver;
    bool pressed  = option->state & QStyle::State_Sunken;

    int stateChecked = checked ? 1 : 0;
    if (!isRadio && noChange)
        stateChecked = 2;

    if (!enabled)
        state = 3;
    else if (selected && pressed)
        state = 2;
    else if (selected)
        state = 1;

    if (stateChecked == 1)
        state += 4;

    if (stateChecked == 2)
        state += 8;

    QRect rc = option->rect;
    rc.setWidth(13); rc.setHeight(13);

    drawImage(soImage, *painter, rc, sourceRectImage(soImage.rect(), state, isRadio ? 8 : 12));
    return true;
}

/*! \internal */
bool OfficePaintManager::drawPanelButtonCommand(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    if (const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton*>(opt)) 
    {
        QPixmap soImage = cached("PushButton.png");

        if (soImage.isNull())
        {
            Q_ASSERT(false);
            return false;
        }

        bool enabled  = opt->state & QStyle::State_Enabled;
        bool checked  = opt->state & QStyle::State_On;
        bool selected = opt->state & QStyle::State_MouseOver;
        bool pressed  = opt->state & QStyle::State_Sunken;
        bool defaultBtn = btn->features & QStyleOptionButton::DefaultButton;

        int state = defaultBtn ? 4 : 0;

        if (!enabled)
            state = 3;
        else if (checked && !selected && !pressed) state = 2;
        else if (checked && selected && !pressed) state = 1;
        else if ((selected && pressed) /*|| qobject_cast<const QPushButton*>(w)*/) state = 2;
        else if (selected || pressed) state = 1;

        drawImage(soImage, *p, opt->rect, sourceRectImage(soImage.rect(), state, 5), 
            QRect(QPoint(4, 4), QPoint(4, 4)), QColor(0xFF, 0, 0xFF));
        return true;
    }
    return false;
}

// shared
/*! \internal */
bool OfficePaintManager::drawPanelTipLabel(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(OfficeStyle)

    QColor clrLight = d.m_clrTooltipLight;
    QColor clrDark = d.m_clrTooltipDark;
    if (!clrLight.isValid()) 
        clrLight = QColor(255, 255, 255);
    if (!clrDark.isValid()) 
        clrDark = QColor(201, 217, 239);
    // [1]
    DrawHelpers::drawGradientFill(*p, opt->rect, clrLight, clrDark, true);

    QPixmap soImage = cached("TooltipFrame.png");
    if (!soImage.isNull())
    {
        drawImage(soImage, *p, opt->rect, sourceRectImage(soImage.rect(), 0, 1), QRect(QPoint(3, 3), QPoint(3, 3)), QColor(0xFF, 0, 0xFF));
    }
    else
    {
        const QPen oldPen = p->pen();
        p->setPen(d.m_clrTooltipBorder);
        p->drawRect(opt->rect.adjusted(0, 0, -1, -1));
        p->setPen(oldPen);
    }
    return true; 
}

/*! \internal */
bool OfficePaintManager::drawControlEdit(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
	// BEGIN ROBLOX CHANGES
    //Q_UNUSED(w)
	// END ROBLOX CHANGES
    QTN_D_STYLE(const OfficeStyle)

    if (const QStyleOptionFrame* panel = qstyleoption_cast<const QStyleOptionFrame*>(opt)) 
    {
		// BEGIN ROBLOX CHANGES
        if(w && !qobject_cast<const QComboBox*>(w->parentWidget()))
		//if(!qobject_cast<const QComboBox*>(w->parentWidget()))
		// END ROBLOX CHANGES
        {
            bool enabled  = opt->state & QStyle::State_Enabled;
            bool selected = opt->state & QStyle::State_MouseOver;
            bool hasFocus = opt->state & QStyle::State_HasFocus;
            QRect rect(panel->rect);

            rect.adjust(0, 0, -1, -1);
            p->fillRect(rect, selected || hasFocus ? panel->palette.brush(QPalette::Current, QPalette::Base) : 
                enabled ? d.m_clrControlEditNormal : panel->palette.brush(QPalette::Disabled, QPalette::Base));
            if ( panel->lineWidth > 0 )
                baseStyle()->proxy()->drawPrimitive(QStyle::PE_FrameLineEdit, panel, p, w);
        }
        return true;
    }
    return false;
}

/*! \internal */
bool OfficePaintManager::drawFrameLineEdit(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle)

    if (const QStyleOptionFrame* panel = qstyleoption_cast<const QStyleOptionFrame*>(opt))
    {
        QRect rect(panel->rect);
        p->save();
        QPen savePen = p->pen();
        p->setPen(d.m_clrEditCtrlBorder);
        rect.adjust(0, 0, -1, -1);
        p->drawRect(rect);
        p->setPen(savePen);
        p->restore();
        return true;
    }
    return false;
}

/*! \internal */
bool OfficePaintManager::drawComboBox(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const
{
    QTN_D_STYLE(const OfficeStyle)
    if (const QStyleOptionComboBox* cmb = qstyleoption_cast<const QStyleOptionComboBox*>(opt))
    {
        QStyle::State flags = opt->state;
        QStyle::SubControls sub = opt->subControls;
        QRect r = opt->rect;

        //        if (w && w->testAttribute(Qt::WA_UnderMouse) && w->isActiveWindow())
        //            flags |= State_MouseOver;

        bool enabled  = flags & QStyle::State_Enabled;
        bool selected = flags & QStyle::State_MouseOver;
        bool dropped  = cmb->activeSubControls == QStyle::SC_ComboBoxArrow && ((cmb->state & QStyle::State_On) || (cmb->state & QStyle::State_Sunken));
        bool dropButtonHot = cmb->activeSubControls == QStyle::SC_ComboBoxArrow && (cmb->state & QStyle::State_MouseOver);
        bool hasFocus = opt->state & QStyle::State_HasFocus;

        if (cmb->frame) 
        {
            r.adjust(0, 0, -1, -1);
            p->fillRect(r, dropped || selected || hasFocus ? cmb->palette.brush(QPalette::Base) : 
                enabled ? d.m_clrControlEditNormal : cmb->palette.brush(QPalette::Disabled, QPalette::Base));

            p->save();
            QPen savePen = p->pen();
            p->setPen(d.m_clrEditCtrlBorder);
            p->drawRect(r);
            p->setPen(savePen);
            p->restore();
        }

        if (sub & QStyle::SC_ComboBoxArrow)
        {
            QPixmap soImage = cached("ToolbarButtonsComboDropDown.png"); 
            QRect rcBtn = baseStyle()->proxy()->subControlRect(QStyle::CC_ComboBox, opt, QStyle::SC_ComboBoxArrow, w);

            if (!enabled)
                drawImage(soImage, *p, rcBtn, sourceRectImage(soImage.rect(), 4, 5), QRect(QPoint(2, 2), QPoint(2, 2)));
            else if (dropped)
                drawImage(soImage, *p, rcBtn, sourceRectImage(soImage.rect(), 3, 5), QRect(QPoint(2, 2), QPoint(2, 2)));
            else if (selected)
            {
                int state = !cmb->editable || dropButtonHot || hasFocus ? 2 : selected ? 1 : 2;
                drawImage(soImage, *p, rcBtn, sourceRectImage(soImage.rect(), state, 5), QRect(QPoint(2, 2), QPoint(2, 2)));
            }
            else // NORMAL
                drawImage(soImage, *p, rcBtn, sourceRectImage(soImage.rect(), hasFocus ? 2 : 0, 5), QRect(QPoint(2, 2), QPoint(2, 2)));

            QPoint pt = rcBtn.center();     
            QRect rc(QPoint(pt.x() - 2, pt.y() - 2), QPoint(pt.x() + 3, pt.y() + 2));
            drawDropDownGlyph(p, pt, selected, dropped, enabled, false/*bVert*/);
        }
        return true;
    }
    return false;
}

// for SpinBox
/*! \internal */
bool OfficePaintManager::drawSpinBox(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle)

    if (const QStyleOptionSpinBox* sb = qstyleoption_cast<const QStyleOptionSpinBox *>(opt))
    {
        bool selected = opt->state & QStyle::State_MouseOver;
        bool enabled  = opt->state & QStyle::State_Enabled;
        bool hasFocus = opt->state & QStyle::State_HasFocus;

        if (sb->frame && (sb->subControls & QStyle::SC_SpinBoxFrame)) 
        {
            p->save();
            QRect r = baseStyle()->proxy()->subControlRect(QStyle::CC_SpinBox, sb, QStyle::SC_SpinBoxFrame, w);

            p->fillRect(r, selected || hasFocus ? sb->palette.brush(QPalette::Base) : 
                enabled ? d.m_clrControlEditNormal : sb->palette.brush(QPalette::Disabled, QPalette::Base));

            QPen savePen = p->pen();
            p->setPen(d.m_clrEditCtrlBorder);
            r.adjust(0, 0, -1, -1);
            p->drawRect(r);
            p->setPen(savePen);
            p->restore();
        }

        QStyleOptionSpinBox copy = *sb;
        copy.state = QStyle::State_None;
        copy.subControls |= QStyle::SC_SpinBoxUp;
        QRect rcTop = baseStyle()->proxy()->subControlRect(QStyle::CC_SpinBox, &copy, QStyle::SC_SpinBoxUp, w);

        QPixmap soBackground = cached("ToolbarButtonSpinArrowsVertical.png"); 

        QPixmap pImage = rcTop.width() > 12 && rcTop.height() > 12 ? cached("ControlGalleryScrollArrowGlyphs.png") :
            cached("ToolbarButtonSpinArrowGlyphs.png");

        if (soBackground.isNull() || pImage.isNull())
        {
            Q_ASSERT(false);
            return false;
        }

        copy = *sb;

        if (sb->subControls & QStyle::SC_SpinBoxUp)
        {
            bool pressedButton = false;
            bool hotButton = false;
            bool enabledIndicator = true;

            rcTop = baseStyle()->proxy()->subControlRect(QStyle::CC_SpinBox, sb, QStyle::SC_SpinBoxUp, w);

            if (!(sb->stepEnabled & QAbstractSpinBox::StepUpEnabled) || !(sb->state & QStyle::State_Enabled))
                enabledIndicator = false;
            else if (sb->activeSubControls == QStyle::SC_SpinBoxUp && (sb->state & QStyle::State_Sunken))
                pressedButton = true;
            else if (sb->activeSubControls == QStyle::SC_SpinBoxUp && (sb->state & QStyle::State_MouseOver))
                hotButton = true;

            int state = !enabled ? 4 : (hasFocus || selected) && pressedButton ? 3 : hotButton ? 2 : (hasFocus || selected) ? 1 : 0;
            if (sb->state & QStyle::State_MouseOver)
                drawImage(soBackground, *p, rcTop, sourceRectImage(soBackground.rect(), state, 10), QRect(QPoint(2, 2), QPoint(2, 2)), QColor(0xFF, 0, 0xFF));

            state = !enabledIndicator || !enabled ? 3 : selected && pressedButton ? 2 : selected ? 2 : selected ? 1 :  0;
            QRect rcSrc = sourceRectImage(pImage.rect(), state, 16);
            QSize sz(rcSrc.size());
            QRect rcDraw(QPoint(rcTop.left() + (rcTop.width()/2-sz.width()/2), rcTop.top()+(rcTop.height()/2-sz.height()/2)), rcSrc.size());
            drawImage(pImage, *p, rcDraw, rcSrc, QRect(QPoint(0, 0), QPoint(0, 0)), QColor(0xFF, 0, 0xFF));
        }

        if (sb->subControls & QStyle::SC_SpinBoxDown) 
        {
            bool pressedButton = false;
            bool hotButton = false;
            bool enabledIndicator = true;

            QRect rcBottom = baseStyle()->proxy()->subControlRect(QStyle::CC_SpinBox, sb, QStyle::SC_SpinBoxDown, w);

            if (!(sb->stepEnabled & QAbstractSpinBox::StepDownEnabled) || !(sb->state & QStyle::State_Enabled))
                enabledIndicator = false;
            else if (sb->activeSubControls == QStyle::SC_SpinBoxDown && (sb->state & QStyle::State_Sunken))
                pressedButton = true;
            else if (sb->activeSubControls == QStyle::SC_SpinBoxDown && (sb->state & QStyle::State_MouseOver))
                hotButton = true;

            int state = !enabled ? 9 : (hasFocus || selected) && pressedButton ? 8 : hotButton ? 7 : (hasFocus || selected) ? 6 : 5;
            if (sb->state & QStyle::State_MouseOver)
                drawImage(soBackground, *p, rcBottom, sourceRectImage(soBackground.rect(), state, 10), QRect(QPoint(2, 2), QPoint(2, 2)), QColor(0xFF, 0, 0xFF));

            state = 4 + (!enabledIndicator || !enabled ? 3 : selected && pressedButton ? 2 : selected ? 2 : selected ? 1 :  0);
            QRect rcSrc = sourceRectImage(pImage.rect(), state, 16);
            QSize sz(rcSrc.size());
            QRect rcDraw(QPoint(rcBottom.left() + (rcBottom.width()/2-sz.width()/2), rcBottom.top()+(rcBottom.height()/2-sz.height()/2)), rcSrc.size());
            drawImage(pImage, *p, rcDraw, rcSrc, QRect(QPoint(0, 0), QPoint(0, 0)), QColor(0xFF, 0, 0xFF));
        }
        return true;
    }
    return false;
}

// for ToolBox
/*! \internal */
bool OfficePaintManager::drawToolBoxTabShape(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle)
    QPixmap soImage = cached("ShortcutBarItems.png"); 
    if (soImage.isNull())
    {
        Q_ASSERT(false);
        return false;
    }
    int state = 0;
    if (opt->state & QStyle::State_Sunken)
        state = 3;
    else if (opt->state & QStyle::State_MouseOver)
        state = 2;
    drawImage(soImage, *p, opt->rect, sourceRectImage(soImage.rect(), state, 4), QRect(QPoint(2, 2), QPoint(2, 2)));

    p->fillRect(opt->rect.left(), opt->rect.bottom() - 1, opt->rect.width(), 1, d.m_clrShortcutItemShadow);

    return true; 
}

// for TabBar
/*! \internal */
bool OfficePaintManager::drawTabBarTabShape(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    if (const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(opt))
    {
        QPixmap soImage = tab->position == QStyleOptionTab::Beginning || tab->position == QStyleOptionTab::OnlyOneTab ? 
            cached("TabItemTopLeft.png") : cached("TabItemTop.png"); 

        if (soImage.isNull())
        {
            Q_ASSERT(false);
            return false;
        }

        bool selected = tab->state & QStyle::State_Selected;
        bool highlight = tab->state & QStyle::State_MouseOver;
        bool focused = tab->state & QStyle::State_HasFocus;
        bool pressed = tab->state & QStyle::State_Sunken;

        QRect rect = opt->rect;
        int state = 0;
        if (selected && focused)
            state = 4;
        if (selected && highlight)
            state = 4;
        else if (selected)
            state = 2;
        else if (pressed)
            state = 4;
        else if (highlight)
            state = 1;

        if (state == 0)
            return true;

        bool correct = w != 0 ? qobject_cast<QMdiArea*>(w->parentWidget()) != 0 : true;
        int borderThickness = baseStyle()->proxy()->pixelMetric(QStyle::PM_DefaultFrameWidth, opt, w)/2;

        QSize sz;
        QTransform matrix;
        switch (tab->shape)
        {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                {
                    if (!selected)
                        rect.adjust(0, 0, 0, correct ? 0 : -borderThickness);

                    sz = QSize(rect.width(), rect.height());
                    matrix.rotate(0.);
                    break;
                }
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                {
                    if (!selected)
                        rect.adjust(0 , correct ? -borderThickness : borderThickness, 0, 0);

                    sz = QSize(rect.width(), rect.height());
                    matrix.rotate(-180., Qt::XAxis);
                    break;
                }
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                {
                    if (!selected)
                        rect.adjust(0, 0, correct ? 0 : -borderThickness, 0);

                    sz = QSize(rect.height(), rect.width());
                    matrix.rotate(-90.);
                    matrix.rotate(180., Qt::YAxis);
                    break;
                }
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                {
                    if (!selected)
                        rect.adjust(correct ? 0 : borderThickness, 0, 0, 0);

                    sz = QSize(rect.height(), rect.width());
                    matrix.rotate(90.);
                    break;
                }
            default:
                break;
        }

        QPixmap soImageRotate(sz);
        soImageRotate.fill(Qt::transparent);

        QPainter painter(&soImageRotate);
        drawPixmap(soImage, painter, QRect(QPoint(0, 0), sz), sourceRectImage(soImage.rect(), state, 5), false, QRect(QPoint(6, 6), QPoint(6, 6)));

        QPixmap resultImage = soImageRotate.transformed(matrix);
        p->drawPixmap(rect, resultImage);

        return true;
    }
    return false;
}

// for QForm
/*! \internal */
bool OfficePaintManager::drawShapedFrame(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    bool ret = false;
    if (const QMdiArea* mdiArea = qobject_cast<const QMdiArea*>(w))
    {
        if (const QStyleOptionFrameV3* f = qstyleoption_cast<const QStyleOptionFrameV3*>(opt)) 
        {
            int frameShape  = f->frameShape;
            int frameShadow = QFrame::Plain;
            if (f->state & QStyle::State_Sunken)
                frameShadow = QFrame::Sunken;
            else if (f->state & QStyle::State_Raised)
                frameShadow = QFrame::Raised;

            switch (frameShape) 
            {
            case QFrame::Panel:
                {
                    if (frameShadow == QFrame::Sunken)
                    {
                        QRect rect = opt->rect;
                        if (QTabBar* tabBar = mdiArea->findChild<QTabBar*>())
                        {
                            int lw = f->lineWidth;
                            int mlw = f->midLineWidth;

                            p->fillRect(opt->rect, opt->palette.light().color());

                            switch (mdiArea->tabPosition()) 
                            {
                            case QTabWidget::North:
                                rect.adjust(0, tabBar->sizeHint().height(), 0, 0);
                                break;
                            case QTabWidget::South:
                                rect.adjust(0, 0, 0, -tabBar->sizeHint().height()-1);
                                break;
                            case QTabWidget::West:
                                rect.adjust(tabBar->sizeHint().width()-1, 0, 0, 0);
                                break;
                            case QTabWidget::East:
                                rect.adjust(0, 0, -tabBar->sizeHint().width()-1, 0);
                                break;
                            default:
                                break;
                            }
                            rect.adjust(0, 0, -1, -1);
                            QPen savePen = p->pen();
                            p->setPen(opt->palette.dark().color());
                            p->drawRect(rect);
                            p->setPen(savePen);

                            rect.adjust(1, 1, -1, -1);
                            qDrawShadeRect(p, rect, opt->palette, true, lw, mlw);
                        }
                        ret = true;
                    }
                    break;
                }
            default:
                break;
            }
        }
    }
    return ret;
}

// for ScrollBar
/*! \internal */
bool OfficePaintManager::drawScrollBar(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const
{
    if (const QStyleOptionSlider* scrollbar = qstyleoption_cast<const QStyleOptionSlider*>(opt))
    {
        // Make a copy here and reset it for each primitive.
        QStyleOptionSlider newScrollbar(*scrollbar);
        if (scrollbar->minimum == scrollbar->maximum)
            newScrollbar.state &= ~QStyle::State_Enabled; //do not draw the slider.

        QStyle::State saveFlags = scrollbar->state;

        newScrollbar.state = saveFlags;
        newScrollbar.rect = opt->rect;

        bool light = w && (qobject_cast<const RibbonGallery*>(w->parentWidget()) ||
            qobject_cast<const QDialog*>(w->topLevelWidget()) || ::isAncestor(w));

        QPixmap soImage;
        if (opt->state & QStyle::State_Horizontal) 
        {
            soImage = light ? cached("ControlGalleryScrollHorizontalLight.png")
                : cached("ControlGalleryScrollHorizontalDark.png");
            drawImage(soImage, *p, newScrollbar.rect, sourceRectImage(soImage.rect(), 0, 2),
                QRect(QPoint(0, 1), QPoint(0, 1)));
        }
        else
        {
            soImage = light ? cached("ControlGalleryScrollVerticalLight.png")
                : cached("ControlGalleryScrollVerticalDark.png");
            drawImage(soImage, *p, newScrollbar.rect, sourceRectImage(soImage.rect(), 0, 2),
                QRect(QPoint(1, 0), QPoint(1, 0)));
        }

        if (scrollbar->subControls & QStyle::SC_ScrollBarSubLine) 
        {
            newScrollbar.rect = scrollbar->rect;
            newScrollbar.state = saveFlags;
            newScrollbar.rect = baseStyle()->proxy()->subControlRect(QStyle::CC_ScrollBar, &newScrollbar, QStyle::SC_ScrollBarSubLine, w);

            if (opt->state & QStyle::State_Horizontal) 
                newScrollbar.rect.adjust(0, 1, 0, -1);
            else
                newScrollbar.rect.adjust(1, 0, -1, 0);

            if (newScrollbar.rect.isValid()) 
                baseStyle()->proxy()->drawControl(QStyle::CE_ScrollBarSubLine, &newScrollbar, p, w);
        }

        if (scrollbar->subControls & QStyle::SC_ScrollBarAddLine) 
        {
            newScrollbar.rect = scrollbar->rect;
            newScrollbar.state = saveFlags;
            newScrollbar.rect = baseStyle()->proxy()->subControlRect(QStyle::CC_ScrollBar, &newScrollbar, QStyle::SC_ScrollBarAddLine, w);

            if (opt->state & QStyle::State_Horizontal) 
                newScrollbar.rect.adjust(0, 1, 0, -1);
            else
                newScrollbar.rect.adjust(1, 0, -1, 0);

            if (newScrollbar.rect.isValid()) 
                baseStyle()->proxy()->drawControl(QStyle::CE_ScrollBarAddLine, &newScrollbar, p, w);
        }

        if (scrollbar->subControls & QStyle::SC_ScrollBarSubPage) 
        {
            newScrollbar.rect = scrollbar->rect;
            newScrollbar.state = saveFlags;
            newScrollbar.rect = baseStyle()->proxy()->subControlRect(QStyle::CC_ScrollBar, &newScrollbar, QStyle::SC_ScrollBarSubPage, w);

            if (newScrollbar.rect.isValid()) 
            {
                if (!(scrollbar->activeSubControls & QStyle::SC_ScrollBarSubPage))
                    newScrollbar.state &= ~(QStyle::State_Sunken | QStyle::State_MouseOver);
                baseStyle()->proxy()->drawControl(QStyle::CE_ScrollBarSubPage, &newScrollbar, p, w);
            }
        }

        if (scrollbar->subControls & QStyle::SC_ScrollBarAddPage) 
        {
            newScrollbar.rect = scrollbar->rect;
            newScrollbar.state = saveFlags;
            newScrollbar.rect = baseStyle()->proxy()->subControlRect(QStyle::CC_ScrollBar, &newScrollbar, QStyle::SC_ScrollBarAddPage, w);

            if (newScrollbar.rect.isValid()) 
            {
                if (!(scrollbar->activeSubControls & QStyle::SC_ScrollBarAddPage))
                    newScrollbar.state &= ~(QStyle::State_Sunken | QStyle::State_MouseOver);
                baseStyle()->proxy()->drawControl(QStyle::CE_ScrollBarAddPage, &newScrollbar, p, w);
            }
        }

        if (scrollbar->subControls & QStyle::SC_ScrollBarSlider) 
        {
            newScrollbar.rect = scrollbar->rect;
            newScrollbar.state = saveFlags;
            newScrollbar.rect = baseStyle()->proxy()->subControlRect(QStyle::CC_ScrollBar, &newScrollbar, QStyle::SC_ScrollBarSlider, w);

            if (opt->state & QStyle::State_Horizontal) 
                newScrollbar.rect.adjust(0, 1, 0, -1);
            else
                newScrollbar.rect.adjust(1, 0, -1, 0);

            if (newScrollbar.rect.isValid()) 
                baseStyle()->proxy()->drawControl(QStyle::CE_ScrollBarSlider, &newScrollbar, p, w);
        }
        return true;
    }
    return false;
}

/*! \internal */
static QRect offsetSourceRect(QRect rc, int state)
{
    rc.translate(0, (state - 1) * rc.height());
    return rc;
}

/*! \internal */
bool OfficePaintManager::drawScrollBarLine(QStyle::ControlElement element, const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    if (const QStyleOptionSlider* scrollbar = qstyleoption_cast<const QStyleOptionSlider*>(opt))
    {
        bool light = w && (qobject_cast<const RibbonGallery*>(w->parentWidget()) ||
            qobject_cast<const QDialog*>(w->topLevelWidget()) || ::isAncestor(w));

        QPixmap soImage;
        if (opt->state & QStyle::State_Horizontal)
            soImage = light ? cached("ControlGalleryScrollArrowsHorizontalLight.png")
            : cached("ControlGalleryScrollArrowsHorizontalDark.png"); 
        else
            soImage = light ? cached("ControlGalleryScrollArrowsVerticalLight.png")
            : cached("ControlGalleryScrollArrowsVerticalDark.png"); 

        bool enabled  = opt->state & QStyle::State_Enabled;
        bool selected = opt->state & QStyle::State_MouseOver;
        bool pressed  = opt->state & QStyle::State_Sunken;

        int state = -1;
        if (!enabled)
            state = 0;
        else if (selected && pressed)
        {
            if(((scrollbar->activeSubControls & QStyle::SC_ScrollBarAddLine) && (element == QStyle::CE_ScrollBarAddLine)) ||
                ((scrollbar->activeSubControls & QStyle::SC_ScrollBarSubLine) && (element == QStyle::CE_ScrollBarSubLine)) )
                state = 3;
            else
                state = 1;
        }
        else if(selected)
        {
            if(((scrollbar->activeSubControls & QStyle::SC_ScrollBarAddLine) && (element == QStyle::CE_ScrollBarAddLine)) ||
                ((scrollbar->activeSubControls & QStyle::SC_ScrollBarSubLine) && (element == QStyle::CE_ScrollBarSubLine)) )
                state = 2;
            else
                state = 1;
        }

        if (state != -1) 
            drawImage(soImage, *p, opt->rect, sourceRectImage(soImage.rect(), state, 4), QRect(QPoint(3, 3), QPoint(3, 3)), QColor(0xFF, 0, 0xFF));


        QPixmap soImageArrowGlyphs;
        if (!light)
            soImageArrowGlyphs = cached("ControlGalleryScrollArrowGlyphsDark.png");

        if (soImageArrowGlyphs.isNull())
            soImageArrowGlyphs = cached("ControlGalleryScrollArrowGlyphs.png");

        int number = -1;
        if (opt->state & QStyle::State_Horizontal) 
        {
            QRect rcSrc;
            if (element == QStyle::CE_ScrollBarAddLine)
                number = !enabled ? 16 : state != 0 ? 14 : 13;
            else
                number = !enabled ? 12 : state != 0 ? 10 : 9;
        }
        else
        {
            if (element == QStyle::CE_ScrollBarAddLine)
                number = !enabled ? 8 : state != 0 ? 6 : 5;
            else
                number = !enabled ? 4 : state != 0 ? 2 : 1;
        }
        QRect rcArrowGripperSrc(0, 0, 9, 9);
        QRect rcSrc = ::offsetSourceRect(rcArrowGripperSrc, number);
        QRect rcArrowGripper(QPoint((opt->rect.left() + opt->rect.right() - 8) / 2, (opt->rect.top() + opt->rect.bottom() - 8) / 2), QSize(9,9));

        QSize sz = baseStyle()->proxy()->sizeFromContents(QStyle::CT_ScrollBar, opt, scrollbar->rect.size(), w);   
        if (opt->state & QStyle::State_Horizontal) 
        {
            if (sz.height() % 2 == 0) //Horizontal
                rcArrowGripper.setHeight(rcArrowGripper.height()+1);
        }
        else 
        {
            if (sz.width() % 2 == 0) //for vertical
                rcArrowGripper.setWidth(rcArrowGripper.width()+1);
        }

        drawImage(soImageArrowGlyphs, *p, rcArrowGripper, rcSrc, QRect(QPoint(0, 0), QPoint(0, 0)), QColor(255, 0, 255));
        return true;
    }
    return false;
}

/*! \internal */
bool OfficePaintManager::drawScrollBarSlider(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    if (const QStyleOptionSlider* scrollbar = qstyleoption_cast<const QStyleOptionSlider*>(opt)) 
    {
        if (!opt->rect.isEmpty())
        {
            bool light = w && (qobject_cast<const RibbonGallery*>(w->parentWidget()) ||
                qobject_cast<const QDialog*>(w->topLevelWidget()) || ::isAncestor(w));

            bool enabled  = opt->state & QStyle::State_Enabled;
            bool selected = opt->state & QStyle::State_MouseOver;
            bool pressed  = opt->state & QStyle::State_Sunken;

            int state = -1;
            if (!enabled)
                state = 0;
            else if(pressed && scrollbar->activeSubControls & QStyle::SC_ScrollBarSlider)
                state = 2;
            else if(selected && scrollbar->activeSubControls & QStyle::SC_ScrollBarSlider)
                state = 1;
            else if((selected || pressed) && scrollbar->activeSubControls & QStyle::SC_ScrollBarSlider)
                state = 2;
            else
                state = 0;


            QRect rc(opt->rect);
            QPixmap scrollThumb;
            if (opt->state & QStyle::State_Horizontal)
            {
                if (!light)
                    scrollThumb = cached("ControlGalleryScrollThumbHorizontalDark.png");

                if (scrollThumb.isNull())
                    scrollThumb = cached("ControlGalleryScrollThumbHorizontal.png");
            }
            else
            {
                if (!light)
                    scrollThumb = cached("ControlGalleryScrollThumbVerticalDark.png");

                if (scrollThumb.isNull())
                    scrollThumb = cached("ControlGalleryScrollThumbVertical.png");
            }

            Q_ASSERT(!scrollThumb.isNull());

            bool show = opt->state & QStyle::State_Horizontal ? rc.width() > 7 : rc.height() > 7;
            if( !rc.isEmpty() && show )
                drawImage(scrollThumb, *p, rc, sourceRectImage(scrollThumb.rect(), state, 3), QRect(QPoint(5, 5), QPoint(5, 5)));

            QRect rcGripper(QPoint(opt->rect.center().x() - 3, opt->rect.center().y() - 3), QSize(8, 8));
            if (opt->state & QStyle::State_Horizontal) 
            {
                if (opt->rect.width() > 10)
                {
                    QPixmap soImageScrollThumbGripperHorizontal = cached("ControlGalleryScrollThumbGripperHorizontal.png");
                    Q_ASSERT(soImageScrollThumbGripperHorizontal);
                    drawImage(soImageScrollThumbGripperHorizontal, *p, rcGripper, sourceRectImage(soImageScrollThumbGripperHorizontal.rect(), state, 3));
                }
            }
            else
            {
                if (opt->rect.height() > 10)
                {
                    QPixmap soImageScrollThumbGripperVertical = cached("ControlGalleryScrollThumbGripperVertical.png");
                    Q_ASSERT(soImageScrollThumbGripperVertical);
                    drawImage(soImageScrollThumbGripperVertical, *p, rcGripper, sourceRectImage(soImageScrollThumbGripperVertical.rect(), state, 3));
                }
            }

        }
        return true;
    }
    return false;
}

/*! \internal */
bool OfficePaintManager::drawScrollBarPage(QStyle::ControlElement element, const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(element);
    Q_UNUSED(w);

    if (!(opt->state & QStyle::State_Sunken))
        return true;

    bool light = w && (qobject_cast<const RibbonGallery*>(w->parentWidget()) ||
        qobject_cast<const QDialog*>(w->topLevelWidget()) || ::isAncestor(w));

    if (opt->state & QStyle::State_Horizontal) 
    {
        QPixmap soImage = light ? cached("ControlGalleryScrollHorizontalLight.png") : cached("ControlGalleryScrollHorizontalDark.png"); 
        if (!soImage.isNull())
            drawImage(soImage, *p, opt->rect, sourceRectImage(soImage.rect(), 1, 2), QRect(QPoint(0, 1), QPoint(0, 1)));
    }
    else
    {
        QPixmap soImage = light ? cached("ControlGalleryScrollVerticalLight.png") : cached("ControlGalleryScrollVerticalDark.png"); 
        if (!soImage.isNull())
            drawImage(soImage, *p, opt->rect, sourceRectImage(soImage.rect(), 1, 2), QRect(QPoint(1, 0), QPoint(1, 0)));
    }
    return true;
}

// for stausBar
/*! \internal */
bool OfficePaintManager::drawPanelStatusBar(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle)
    QRect rc = opt->rect;
    p->fillRect(QRect(QPoint(rc.left(), rc.top()), QSize(rc.width(), 1)), d.m_clrBackgroundLight);
    // [1]
    //DrawHelpers::drawGradientFill(*p, QRect(QPoint(rc.left(), rc.top() + 1), QPoint(rc.right(), rc.top() + 9)),
    //    d.m_palStatusBarTop.color(QPalette::Light), d.m_palStatusBarTop.color(QPalette::Dark), true);

    // [2]
    //DrawHelpers::drawGradientFill(*p, QRect(QPoint(rc.left(), rc.top() + 9), QPoint(rc.right(), rc.bottom())),
    //    d.m_palStatusBarBottom.color(QPalette::Light), d.m_palStatusBarBottom.color(QPalette::Dark), true);
    return true;
}

// for SizeGrip
/*! \internal */
bool OfficePaintManager::drawSizeGrip(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle)

    QPoint pt(opt->rect.right() - 3, opt->rect.bottom() - 3);
    for (int y = 0; y < 3; y++)
    {
        for (int x = 0; x < 3 - y; x++)
        {
            p->fillRect(QRect(QPoint(pt.x() + 1 - x * 4, pt.y() + 1 - y * 4), QSize(2, 2)), d.m_clrAccent);
            p->fillRect(QRect(QPoint(pt.x() + 0 - x * 4, pt.y() + 0 - y * 4), QSize(2, 2)), d.m_clrToolbarSeparator);
        }
    }
    return true;
}

// for ViewItem
/*! \internal */
bool OfficePaintManager::drawPanelItemViewItem(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    if (const QStyleOptionViewItemV4* vopt = qstyleoption_cast<const QStyleOptionViewItemV4*>(opt))
    {
        const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(widget);
        if (!view)
            return false;

        QPixmap soImage = cached("ListBox.png");
        if (soImage.isNull())
        {
            Q_ASSERT(false);
            return false;
        }

        bool enabled  = vopt->state & QStyle::State_Enabled;
        bool selected = vopt->state & QStyle::State_Selected;
        bool hasFocus = vopt->state & QStyle::State_HasFocus;
        //        bool hasFocus = vopt->state & QStyle::State_Active;
        bool hover = vopt->state & QStyle::State_MouseOver;

        QRect itemRect = baseStyle()->subElementRect(QStyle::SE_ItemViewItemFocusRect, opt, widget).adjusted(-1, 0, 1, 0);
        itemRect.setTop(vopt->rect.top());
        itemRect.setBottom(vopt->rect.bottom());

        QSize sectionSize = itemRect.size();
        if (vopt->showDecorationSelected)
            sectionSize = vopt->rect.size();

        if (view->selectionBehavior() == QAbstractItemView::SelectRows)
            sectionSize.setWidth(vopt->rect.width());

        if (view->selectionMode() == QAbstractItemView::NoSelection)
            hover = false;

        int state = -1;
        if ((selected || hover) && enabled)
            state = selected && hover? 2 : hasFocus && selected ? 1 : !hasFocus && selected ? 3 : 0;

        if (vopt->backgroundBrush.style() != Qt::NoBrush) 
        {
            //            QPointF oldBO = p->brushOrigin();
            p->setBrushOrigin(vopt->rect.topLeft());
            p->fillRect(vopt->rect, vopt->backgroundBrush);
        }

        if (state != -1)
        {
            if (vopt->showDecorationSelected) 
            {
                //                QPixmap pixmap = soImage;
                //              const int frame = 2; //Assumes a 2 pixel pixmap border
                //                QRect srcRect = QRect(0, 0, sectionSize.width(), sectionSize.height());
                //                QRect pixmapRect = vopt->rect;
                bool reverse = vopt->direction == Qt::RightToLeft;
                bool leftSection = vopt->viewItemPosition == QStyleOptionViewItemV4::Beginning;
                bool rightSection = vopt->viewItemPosition == QStyleOptionViewItemV4::End;

                if (vopt->viewItemPosition == QStyleOptionViewItemV4::OnlyOne || vopt->viewItemPosition == QStyleOptionViewItemV4::Invalid)
                {
                    QRect rect(QPoint(vopt->rect.left(), vopt->rect.top()), sectionSize);
                    drawImage(soImage, *p, rect, sourceRectImage(soImage.rect(), state, 4), QRect(QPoint(4, 4), QPoint(4, 4)));
                }
                else if (reverse ? rightSection : leftSection)
                {
                    QRect rect(QPoint(vopt->rect.left(), vopt->rect.top()), sectionSize);
                    QRect rectImage = soImage.rect();
                    rectImage.adjust(0, 0, -4, 0);
                    drawImage(soImage, *p, rect, sourceRectImage(rectImage, state, 4), QRect(QPoint(4, 4), QPoint(4, 4)));
                } 
                else if (reverse ? leftSection : rightSection) 
                {
                    QRect rectImage = soImage.rect();
                    QPixmap copyPix = soImage.copy(4, 0, rectImage.width(), rectImage.height());
                    QRect rect(QPoint(vopt->rect.left(), vopt->rect.top()), sectionSize);
                    drawImage(copyPix, *p, rect, sourceRectImage(copyPix.rect(), state, 4), QRect(QPoint(4, 4), QPoint(4, 4)));
                }
                else if (vopt->viewItemPosition == QStyleOptionViewItemV4::Middle)
                {
                    QRect rectImage = soImage.rect();
                    QPixmap copyPix = soImage.copy(4, 0, rectImage.width()-8, rectImage.height());
                    QRect rect(QPoint(vopt->rect.left(), vopt->rect.top()), sectionSize);
                    drawImage(copyPix, *p, rect, sourceRectImage(copyPix.rect(), state, 4), QRect(QPoint(4, 4), QPoint(4, 4)));
                }
            }
            else
            {
                QRect rect(QPoint(vopt->rect.left(), vopt->rect.top()), sectionSize);
                drawImage(soImage, *p, rect, sourceRectImage(soImage.rect(), state, 4), QRect(QPoint(4, 4), QPoint(4, 4)));
            }
        }
        return true;
    }
    return false;
}

/*! \internal */
bool OfficePaintManager::drawHeaderSection(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle)

    bool columnPressed  = opt->state & QStyle::State_Sunken;

    DrawHelpers::drawGradientFill(*p, opt->rect, columnPressed ? d.m_palGradientColumnPushed.color(QPalette::Light) : d.m_palGradientColumn.color(QPalette::Light), 
        columnPressed ? d.m_palGradientColumnPushed.color(QPalette::Dark) : d.m_palGradientColumn.color(QPalette::Dark), true);

    p->fillRect(QRect(QPoint(opt->rect.left(), opt->rect.bottom()), QSize(opt->rect.width(), 1)), 
        d.m_palGradientColumn.color(QPalette::Shadow));

    p->fillRect(QRect(QPoint(opt->rect.right(), opt->rect.top()), QSize(1, opt->rect.height())), d.m_clrColumnSeparator);

    return true;
}

/*! \internal */
bool OfficePaintManager::drawHeaderEmptyArea(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle)

    DrawHelpers::drawGradientFill(*p, opt->rect, d.m_palGradientColumn.color(QPalette::Light), 
        d.m_palGradientColumn.color(QPalette::Dark), true);

    p->fillRect(QRect(QPoint(opt->rect.left(), opt->rect.bottom()), QSize(opt->rect.width(), 1)), 
        d.m_palGradientColumn.color(QPalette::Shadow));

    p->fillRect(QRect(QPoint(opt->rect.right(), opt->rect.top()), QSize(1, opt->rect.height())), 
        d.m_clrColumnSeparator);

    return true;
}

/*! \internal */
bool OfficePaintManager::drawIndicatorHeaderArrow(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle)
    if (const QStyleOptionHeader* header = qstyleoption_cast<const QStyleOptionHeader*>(opt)) 
    {
        QPoint pt(opt->rect.center());
        if (header->sortIndicator == QStyleOptionHeader::SortUp) 
        {
            DrawHelpers::drawTriangle(*p, QPoint(pt.x() - 4, pt.y() - 2),
                QPoint(pt.x(), pt.y() + 2), QPoint(pt.x() + 4, pt.y()  - 2), d.m_clrColumnSeparator);
        } 
        else if (header->sortIndicator == QStyleOptionHeader::SortDown) 
        {
            DrawHelpers::drawTriangle(*p, QPoint(pt.x() - 4, pt.y() + 2),
                QPoint(pt.x(), pt.y() - 2), QPoint(pt.x() + 4, pt.y()  + 2), d.m_clrColumnSeparator);
        }
        return true;
    }
    return false;
}

/*! \internal */
bool OfficePaintManager::drawIndicatorArrow(QStyle::PrimitiveElement pe, const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    switch(pe)
    {
        case QStyle::PE_IndicatorArrowUp:
        case QStyle::PE_IndicatorArrowDown:
        case QStyle::PE_IndicatorArrowRight:
        case QStyle::PE_IndicatorArrowLeft:
            {
                if (pe == QStyle::PE_IndicatorArrowRight && qstyleoption_cast<const QStyleOptionMenuItem*>(opt))
                {
                    QPixmap soIndicator = cached("MenuPopupGlyph.png");
                    QRect rcSrc = sourceRectImage(soIndicator.rect(), opt->state & QStyle::State_Enabled ? 0 : 1, 2);
                    int size = rcSrc.height();
                    int xOffset = opt->rect.x() + (opt->rect.width() -  size)/2;
                    int yOffset = opt->rect.y() + (opt->rect.height() - size)/2;

                    QRect rcEntry(QPoint(xOffset, yOffset), rcSrc.size());
                    drawImage(soIndicator, *p, rcEntry, rcSrc, QRect(QPoint(0, 0), QPoint(0, 0)), QColor(0xFF, 0, 0xFF));
                    return true;
                }
                else if (pe == QStyle::PE_IndicatorArrowDown)
                {
                    if (qobject_cast<const QToolButton*>(w))
                    {
                        // revision: 764
                        if (qobject_cast<QToolBar*>(w->parentWidget()) &&  !qobject_cast<const QTabBar*>(w->parentWidget()))
                        {
                            QPixmap soIndicator = cached("ToolbarButtonDropDownGlyph.png");
                            bool enabled = opt->state & QStyle::State_Enabled;    
                            bool selected = opt->state & QStyle::State_Selected;
                            QRect rcSrc = sourceRectImage(soIndicator.rect(), !enabled ? 3 : selected ? 1 : 0, 4);
                            QRect rcEntry(QPoint(opt->rect.topLeft()), rcSrc.size());
                            drawImage(soIndicator, *p, rcEntry, rcSrc, QRect(QPoint(0, 0), QPoint(0, 0)), QColor(0xFF, 0, 0xFF));
                            return true;
                        }
                    }
                }

                bool enabled = opt->state & QStyle::State_Enabled;    
                bool selected = opt->state & QStyle::State_Selected;
                QColor color = textColor(selected, false, enabled, false, false/*popuped*/, TypeNormal, BarTop);

                QPalette palette = opt->palette;
                palette.setColor(QPalette::ButtonText, color);

                if (opt->rect.width() <= 1 || opt->rect.height() <= 1)
                    break;

                QRect r = opt->rect;
                int size = qMin(r.height(), r.width());

                int border = (size/2) - 3;

                int sqsize = 2*(size/2);
                QImage image(sqsize, sqsize, QImage::Format_ARGB32_Premultiplied);
                image.fill(0);
                QPainter imagePainter(&image);

                QPolygon a;
                switch (pe) 
                {
                    case QStyle::PE_IndicatorArrowUp:
                        a.setPoints(3, border, sqsize/2,  sqsize/2, border,  sqsize - border, sqsize/2);
                        break;
                    case QStyle::PE_IndicatorArrowDown:
                        a.setPoints(3, border, sqsize/2,  sqsize/2, sqsize - border,  sqsize - border, sqsize/2);
                        break;
                    case QStyle::PE_IndicatorArrowRight:
                        a.setPoints(3, sqsize - border, sqsize/2,  sqsize/2, border,  sqsize/2, sqsize - border);
                        break;
                    case QStyle::PE_IndicatorArrowLeft:
                        a.setPoints(3, border, sqsize/2,  sqsize/2, border,  sqsize/2, sqsize - border);
                        break;
                    default:
                        break;
                }

                QRect bounds = a.boundingRect();
                int sx = sqsize / 2 - bounds.center().x() - 1;
                int sy = sqsize / 2 - bounds.center().y() - 1;
                imagePainter.translate(sx, sy);
                imagePainter.setPen(palette.buttonText().color());
                imagePainter.setBrush(palette.buttonText());


                if (!enabled) 
                {
                    imagePainter.translate(1, 1);
                    imagePainter.setBrush(palette.light().color());
                    imagePainter.setPen(palette.light().color());
                    imagePainter.drawPolygon(a);
                    imagePainter.translate(-1, -1);
                    imagePainter.setBrush(palette.mid().color());
                    imagePainter.setPen(palette.mid().color());
                }

                imagePainter.drawPolygon(a);
                imagePainter.end();

                QPixmap pixmap = QPixmap::fromImage(image);

                int xOffset = r.x() + (r.width() - size)/2;
                int yOffset = r.y() + (r.height() - size)/2;

                //                QRect rc(QPoint(xOffset, yOffset), pixmap.size());
                //                drawImage(pixmap, *p, rc, pixmap.rect(), QRect(QPoint(0, 0), QPoint(0, 0)));
                p->drawPixmap(xOffset, yOffset, pixmap);
                return true;
            }
            break;
        default:
            return false;
    }
    return false;
}

/*! \internal */
bool OfficePaintManager::drawGalleryToolButton(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    if (const QStyleOptionToolButton* toolbutton = qstyleoption_cast<const QStyleOptionToolButton*>(opt))
    {
        if (widget)
        {
            QPixmap soImage;
            if(widget->property(qtn_PopupButtonGallery).toBool())
                soImage = cached("ControlGalleryPopup.png");
            else if(widget->property(qtn_ScrollUpButtonGallery).toBool())
                soImage = cached("ControlGalleryUp.png");
            else if(widget->property(qtn_ScrollDownButtonGallery).toBool())
                soImage = cached("ControlGalleryDown.png");

            if (!soImage.isNull())
            {
                bool enabled  = opt->state & QStyle::State_Enabled;
                bool selected = opt->state & QStyle::State_MouseOver;
                bool pressed  = opt->state & QStyle::State_Sunken;
                //                bool popuped  = (toolbutton->activeSubControls & QStyle::SC_ToolButtonMenu) && (opt->state & State_Sunken);
            #define CALCSTATE \
                (!enabled ? 4 : pressed ? 3 : selected ? 2 : selected || pressed ? 1 : 0)

                drawImage(soImage, *p, toolbutton->rect, sourceRectImage(soImage.rect(), CALCSTATE, 5), QRect(QPoint(3, 3), QPoint(3, 3)), QColor(0xFF, 0, 0xFF));
                return true;
            }
        }
    }
    return false;
}

/*! \internal */
void OfficePaintManager::drawRectangle(QPainter* p, const QRect& rect, bool selected, bool pressed, bool enabled, bool checked, bool popuped,
                                       BarType barType, BarPosition barPos) const
{
    QTN_D_STYLE(const OfficeStyle);
    if (popuped)
    {
        // [1]
        DrawHelpers::drawGradientFill(*p, rect, d.m_clrPopupControl.color(QPalette::Light), d.m_clrPopupControl.color(QPalette::Dark),  true);
        DrawHelpers::draw3DRect(*p, d.m_clrMenubarBorder, d.m_clrMenubarBorder, rect.x(), rect.y(), rect.width()-1, rect.height()-1, true);
    }
    else if (barType != TypePopup)
    {
        if (!enabled)
        {
            if (/*isKeyboardSelected(selected)*/false)
            {
                // [1]
                DrawHelpers::drawGradientFill(*p, rect, checked ? d.m_palLunaPushed.color(QPalette::Light) : d.m_palLunaSelected.color(QPalette::Light), 
                    checked ? d.m_palLunaPushed.color(QPalette::Dark) : d.m_palLunaSelected.color(QPalette::Dark), true);
                DrawHelpers::draw3DRect(*p, d.m_clrHighlightPushedBorder, d.m_clrHighlightPushedBorder, rect.x(), rect.y(), rect.width()-1, rect.height()-1, true);
            }
            else if (checked)
            {
                // [1]
                DrawHelpers::drawGradientFill(*p, rect, d.m_palLunaChecked.color(QPalette::Light), d.m_palLunaChecked.color(QPalette::Dark), true);
                DrawHelpers::draw3DRect(*p, d.m_clrHighlightDisabledBorder, d.m_clrHighlightDisabledBorder, rect.x(), rect.y(), rect.width()-1, rect.height()-1, true);
            }
            return;
        }
        //        else if (checked && !selected && !pressed)
        //        {
        //            DrawHelpers::drawRectangle(*p, rect.adjusted(0, 0, -1, -1), m_clrHighlightPushedBorder, m_clr3DFace);
        //        }
        else if (checked && !selected && !pressed) 
        {
            // [1]
            DrawHelpers::drawGradientFill(*p, rect, d.m_palLunaChecked.color(QPalette::Light), d.m_palLunaChecked.color(QPalette::Dark), true);
            DrawHelpers::draw3DRect(*p, d.m_clrHighlightCheckedBorder, d.m_clrHighlightCheckedBorder, rect.x(), rect.y(), rect.width()-1, rect.height()-1, true);
        }
        else if (checked && selected && !pressed) 
        {
            // [1]
            DrawHelpers::drawGradientFill(*p, rect, d.m_palLunaPushed.color(QPalette::Light), d.m_palLunaPushed.color(QPalette::Dark),  true);
            DrawHelpers::draw3DRect(*p, d.m_clrHighlightPushedBorder, d.m_clrHighlightPushedBorder, rect.x(), rect.y(), rect.width()-1, rect.height()-1, true);
        }
        else if (/*isKeyboardSelected(pressed) ||*/ (selected && pressed)) 
        {
            // [1]
            DrawHelpers::drawGradientFill(*p, rect, d.m_palLunaPushed.color(QPalette::Light), d.m_palLunaPushed.color(QPalette::Dark), true);
            DrawHelpers::draw3DRect(*p, d.m_clrHighlightPushedBorder, d.m_clrHighlightPushedBorder, rect.x(), rect.y(), rect.width()-1, rect.height()-1, true);
        }
        else if (selected || pressed) 
        {
            // [1]
            DrawHelpers::drawGradientFill(*p, rect, d.m_palLunaSelected.color(QPalette::Light), d.m_palLunaSelected.color(QPalette::Dark), true);
            DrawHelpers::draw3DRect(*p, d.m_clrHighlightBorder, d.m_clrHighlightBorder, rect.x(), rect.y(), rect.width()-1, rect.height()-1, true);
        }
    }
    else //if (barPos == BarPopup && selected && barType == TypePopup)
    {
        QRect rc = rect;
        rc.adjust(0, 0, -1, -1);
        if (!enabled)
        {
            if (/*isKeyboardSelected(bSelected)*/false)
                DrawHelpers::drawRectangle(*p, rc, d.m_clrHighlightBorder, barPos != BarPopup ? (checked ? d.m_clrHighlightPushed : d.m_clrHighlight) : d.m_clrMenubarFace);
            else if (checked)
                DrawHelpers::drawRectangle(*p, rc, d.m_clrHighlightDisabledBorder, barPos != BarPopup ? d.m_clrHighlightChecked : QColor());
            return;
        }
        if (popuped) 
            DrawHelpers::drawRectangle(*p, rc, d.m_clrMenubarBorder, d.m_clrToolbarFace);
        //        else if (checked && !selected && !pressed) 
        //            DrawHelpers::drawRectangle(*p, rc, m_clrHighlightCheckedBorder, m_clr3DFace);
        else if (checked && !selected && !pressed) 
            DrawHelpers::drawRectangle(*p, rc, d.m_clrHighlightCheckedBorder, d.m_clrHighlightChecked);
        else if (checked && selected && !pressed) 
            DrawHelpers::drawRectangle(*p, rc, d.m_clrHighlightPushedBorder, d.m_clrHighlightPushed);
        else if (/*isKeyboardSelected(bPressed) ||*/ (selected && pressed)) 
            DrawHelpers::drawRectangle(*p, rc, d.m_clrHighlightPushedBorder, d.m_clrHighlightPushed);
        else if (selected || pressed) 
            DrawHelpers::drawRectangle(*p, rc, d.m_clrHighlightBorder, d.m_clrHighlight);
    }
}

/*! \internal */
void OfficePaintManager::setupPalette(QWidget* widget) const
{
    QTN_D_STYLE(const OfficeStyle);
    if(QMdiArea* mdiArea = qobject_cast<QMdiArea*>(widget))
    {
        QPalette palette = widget->palette();
        QColor color = helper().getColor(tr("TabManager"), tr("FrameBorder"));
        QColor light = color.lighter(230);
        QColor dark = color.darker(160);
        dark.setAlpha(100);
        palette.setColor(QPalette::Dark, dark);
        palette.setColor(QPalette::Light, light);
        widget->setPalette(palette);
        mdiArea->setBackground(QBrush(d.m_clrAppWorkspace));
    }
}

/*! \internal */
bool OfficePaintManager::pixelMetric(QStyle::PixelMetric pm, const QStyleOption* opt, const QWidget* w, int& ret) const
{
    Q_UNUSED(opt);
    Q_UNUSED(w);
    switch(pm)
    {
        case QStyle::PM_MenuButtonIndicator:
            if (qobject_cast<const QToolButton*>(w))
            {
                // revision: 764
//                if (qobject_cast<QToolBar*>(w->parentWidget()) &&  !qobject_cast<const QTabBar*>(w->parentWidget()))
                {
                    QPixmap soIndicator = cached("ToolbarButtonDropDownGlyph.png");
                    if (!soIndicator.isNull())
                    {
                        ret = soIndicator.width() + (qobject_cast<QToolBar*>(w->parentWidget()) ? 0 : 6);
                        return true;
                    }
                }
            }
            break;
        default:
            break;
    };
    return false;
}

/*! \internal */
void OfficePaintManager::drawMenuItemSeparator(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    QTN_D_STYLE(const OfficeStyle);
    if (const QStyleOptionMenuItem* menuitem = qstyleoption_cast<const QStyleOptionMenuItem*>(opt)) 
    {
        if (menuitem->text.isEmpty())
        {
            int x, y, w, h;
            menuitem->rect.getRect(&x, &y, &w, &h);

            // windows always has a check column, regardless whether we have an icon or not
            const int iconSize = baseStyle()->proxy()->pixelMetric(QStyle::PM_ToolBarIconSize, opt, widget);

            int yoff = (y-1 + h / 2);
            int xIcon = iconSize;    
            if(qobject_cast<const OfficePopupMenu*>(widget))
                xIcon = 0;

            QPen penSave = p->pen();

            p->setPen(menuitem->palette.dark().color());
            p->drawLine(x + 2 + xIcon, yoff, x + w - 4, yoff);
            p->setPen(menuitem->palette.light().color());
            p->drawLine(x + 2 + xIcon, yoff + 1, x + w - 4, yoff + 1);

            p->setPen(penSave);
        }
        else
        {
            p->save();
            DrawHelpers::drawGradientFill(*p, menuitem->rect, d.m_clrMenuGripper, d.m_clrMenuGripper, true);
            QRect rectSeparator = menuitem->rect;
            rectSeparator.setTop(rectSeparator.bottom() - 1);

            QRect rect1 = rectSeparator;
            QRect rect2;
            rect1.setTop( rect1.top() + rectSeparator.height() / 2 - 1);
            rect1.setBottom(rect1.top());
            rect2 = rect1;
            rect2.translate(0, 1);

            p->fillRect(rect1, d.m_clrMenuPopupGripperShadow);
            p->fillRect(rect2, QColor(245, 245, 245));

            QRect rectText = menuitem->rect;
            rectText.adjust(iTextMargin, 0, -iTextMargin, -iTextMargin);
//            rectText.setBottom( rectText.bottom() - 2);

            int text_flags = Qt::AlignVCenter | Qt::TextSingleLine;
            QFont font = menuitem->font;
            font.setBold(true);
            p->setFont(font);

            p->setPen(opt->state & QStyle::State_Enabled ? d.m_clrMenuPopupText : d.m_clrMenuBarGrayText);
            p->drawText(rectText, text_flags, menuitem->text);

            p->restore();
        }
    }
}

/*! \internal */
void OfficePaintManager::drawDropDownGlyph(QPainter* p, QPoint pt, bool selected, bool popuped, bool enabled, bool vert) const
{
    Q_UNUSED(vert);
    Q_UNUSED(popuped);
    QPixmap soGlyphImage = cached("ToolbarButtonDropDownGlyph.png"); 
    if (soGlyphImage.isNull())
    {
        Q_ASSERT(false);
        return;
    }
    QRect rcSrc = sourceRectImage(soGlyphImage.rect(), !enabled ? 3 : selected ? 1 : 0, 4);
    QRect rc(QPoint(pt.x() - 2, pt.y() - 2), QSize(rcSrc.size()));

    drawImage(soGlyphImage, *p, rc, rcSrc, QRect(QPoint(0, 0), QPoint(0, 0)), QColor(0xFF, 0, 0xFF));
}

void OfficePaintManager::modifyColors()
{
}


/*!
\class Qtitan::OfficePaintManager2013
\internal
*/
OfficePaintManager2013::OfficePaintManager2013(CommonStyle* baseStyle)
    : CommonPaintManager(baseStyle)
{
    loadPixmap();

    m_ImagesBlack = m_ImagesSrc;
    m_ImagesGray   = mapTo3dColors(m_ImagesSrc, QColor(0, 0, 0), QColor(128, 128, 128));
    m_ImagesDkGray = mapTo3dColors(m_ImagesSrc, QColor(0, 0, 0), QColor(72, 72, 72));
    m_ImagesLtGray = mapTo3dColors(m_ImagesSrc, QColor(0, 0, 0), QColor(192, 192, 192));
    m_ImagesWhite  = mapTo3dColors(m_ImagesSrc, QColor(0, 0, 0), QColor(255, 255, 255));
    m_ImagesBlack2 = mapTo3dColors(m_ImagesSrc, QColor(0, 0, 0), QColor(0, 0, 0));
}

OfficePaintManager2013::~OfficePaintManager2013()
{
}

QString OfficePaintManager2013::theme(const QString& str) const
{
    return ((OfficeStyle*)baseStyle())->qtn_d().theme(str);
}

QColor OfficePaintManager2013::textColor(bool selected, bool pressed, bool enabled, bool checked, bool popuped, BarType barType, BarPosition barPosition) const
{
    return ((OfficeStyle*)baseStyle())->getTextColor(selected, pressed, enabled, checked, popuped, barType, barPosition);
}

bool OfficePaintManager2013::drawWorkspace(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    p->fillRect(opt->rect, m_clrBarFace);
    return true;
}

// for TitleBar
/*! \internal */
bool OfficePaintManager2013::drawToolBar(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle);
    p->fillRect(opt->rect, d.m_clrDockBar);
    return true;
}

/*! \internal */
bool OfficePaintManager2013::drawIndicatorToolBarSeparator(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    Q_UNUSED(opt);
    Q_UNUSED(p);
    return false;
}

// for DockWidget
/*! \internal */
bool OfficePaintManager2013::drawIndicatorDockWidgetResizeHandle(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QRect rect = opt->rect;
    QColor clrSlider = m_clrBarShadow;
    if (!(opt->state & QStyle::State_Horizontal)|| rect.height() > rect.width() * 4)
    {
        p->fillRect(opt->rect, m_clrBarFace);

        rect.setLeft(rect.center().x() - 1);
        rect.setRight(rect.center().x() + 1);
        
        if (const QMainWindow* mainWindow = qobject_cast<const QMainWindow*>(w))
        {
            QRect rcFrame = mainWindow->rect();
            QRect screen = QApplication::desktop()->availableGeometry(mainWindow);

            rect.setTop(-rcFrame.top());
            rect.setBottom(rect.top() + screen.height() + 10);
        }
        DrawHelpers::drawGradientFill4(*p, rect, m_clrBarFace, clrSlider, clrSlider, m_clrBarFace);
    }
    else
        p->fillRect(opt->rect, clrSlider);

    return true;
}

// for menuBar
/*! \internal */
bool OfficePaintManager2013::drawPanelMenu(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    QTN_D_STYLE(const OfficeStyle)
    p->fillRect(opt->rect, d.m_clrBackgroundLight);
    if (qobject_cast<const QToolBar*>(w))
        DrawHelpers::draw3dRectEx(*p, opt->rect, d.m_clrMenubarBorder, d.m_clrMenubarBorder);
    return true;
}

/*! \internal */
bool OfficePaintManager2013::drawFrameMenu(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle)
    QRect rect = opt->rect;
    DrawHelpers::draw3dRectEx(*p, rect, d.m_clrMenubarBorder, d.m_clrMenubarBorder);
    rect.adjust(1, 1, -1, -1);
    DrawHelpers::draw3dRectEx(*p, rect, d.m_clrBackgroundLight, d.m_clrBackgroundLight);
    QRect rectLeft(1, 1, 2, rect.bottom() - 1);
    p->fillRect(rectLeft, m_clrBarHilite);
    return true;
}

/*! \internal */
bool OfficePaintManager2013::drawIndicatorMenuCheckMark(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QRect rc = opt->rect;
    bool enabled  = opt->state & QStyle::State_Enabled;
    bool highlighted = opt->state & QStyle::State_MouseOver;

    ImageIcons iImage = /*isRadio*/false ? OfficePaintManager2013::Icon_Radio : OfficePaintManager2013::Icon_Check;

    rc.adjust(0, 1, 0, -1);

    OfficePaintManager2013::ImageState imageState = OfficePaintManager2013::Gray;

    if (!enabled)
        imageState = OfficePaintManager2013::LtGray;
    else if (highlighted)
        imageState = OfficePaintManager2013::DkGray;

    drawIcon(p, rc, iImage, imageState, QSize(9, 9));
    return true;
}

// for Buttons
/*! \internal */
bool OfficePaintManager2013::drawIndicatorCheckRadioButton(QStyle::PrimitiveElement element, const QStyleOption* option, 
                                                           QPainter* p, const QWidget* widget) const
{
    Q_UNUSED(widget);
    QTN_D_STYLE(OfficeStyle)

    bool isRadio = (element == QStyle::PE_IndicatorRadioButton);

    bool enabled  = option->state & QStyle::State_Enabled;
    bool checked  = option->state & QStyle::State_On;
    bool noChange = option->state & QStyle::State_NoChange;
    bool highlighted = option->state & QStyle::State_MouseOver;
    bool pressed  = option->state & QStyle::State_Sunken;

    int stateChecked = checked ? 1 : 0;
    if (!isRadio && noChange)
        stateChecked = 2;

    QRect rc = option->rect;
    rc.adjust(0, 0, -1, -1);

    if ( isRadio )
    {
        rc.setWidth(11); rc.setHeight(11);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        bool oldQt4CompatiblePainting = p->testRenderHint(QPainter::Qt4CompatiblePainting);
        p->setRenderHint(QPainter::Qt4CompatiblePainting);
#else
        bool oldQt4CompatiblePainting = p->testRenderHint(QPainter::Antialiasing);
        p->setRenderHint(QPainter::Antialiasing);
#endif

        QColor fillColor(255, 255, 255); 
        if (pressed)
            fillColor = m_clrHighlightDn;
        else if (pressed || highlighted)
            fillColor = d.m_clrHighlight;

        QColor clrBorder = !enabled ? m_clrBarShadow : (highlighted || pressed) ? m_clrHighlightDn : m_clrBarDkShadow;
        QPen oldPen = p->pen();
        p->setPen(clrBorder);

        QBrush br(fillColor);
        QBrush oldBrush = p->brush();
        p->setBrush(br);
        p->drawArc(rc, 0, 5760);

        if (option->state & (QStyle::State_Sunken | QStyle::State_On)) 
        {
            int mg = rc.width () / 3;
            rc.adjust(mg, mg, -mg, -mg);
            p->setBrush(highlighted && enabled ? m_clrAccentText : m_clrBarDkShadow);
            p->drawEllipse(rc);
        }

        p->setBrush(oldBrush);
        p->setPen(oldPen);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        p->setRenderHint(QPainter::Qt4CompatiblePainting, oldQt4CompatiblePainting);
#else
        p->setRenderHint(QPainter::Antialiasing, oldQt4CompatiblePainting);
#endif
    } 
    else
    {
        rc.translate(0, 1);

        if (pressed)
            p->fillRect(rc, m_clrHighlightDn);
        else if (pressed || highlighted)
            p->fillRect(rc, d.m_clrHighlight);
        else
            p->fillRect(rc, QColor(255, 255, 255));

        QColor clrBorder = !enabled ? m_clrBarShadow : (highlighted || pressed) ? m_clrHighlightDn : m_clrBarDkShadow;

        QPen oldPen = p->pen();
        p->setPen(clrBorder);
        p->drawRect(rc.adjusted(0, 0, -1, -1));
        p->setPen(oldPen);

        if (stateChecked == 1)
        {
            QRect rcIndicator(QPoint(rc.x() + (rc.width() - iImageWidth)/2, rc.y() + (rc.height() - iImageHeight)/2), QSize(iImageWidth, iImageHeight));
            drawIcon(p, rcIndicator.topLeft(), Icon_Check, highlighted ? Black2 : Gray, QSize(9, 9));
        }
        else if (stateChecked == 2)
        {
            QBrush brushTmp(Qt::black, Qt::Dense4Pattern);
            p->fillRect(rc.adjusted(1, 1, -1, -1), brushTmp);
        }
    }

    return true;
}

/*! \internal */
bool OfficePaintManager2013::drawPanelButtonCommand(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
	// BEGIN ROBLOX CHANGES
    // Q_UNUSED(w);
	// END ROBLOX CHANGES
    QTN_D_STYLE(OfficeStyle)
    if (const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton*>(opt)) 
    {
        bool enabled  = opt->state & QStyle::State_Enabled;
        bool checked  = opt->state & QStyle::State_On;
        bool selected = opt->state & QStyle::State_MouseOver;
        bool pressed  = opt->state & QStyle::State_Sunken;
//        bool defaultBtn = btn->features & QStyleOptionButton::DefaultButton;
		// BEGIN ROBLOX CHANGES
		bool hasStyleSheet = w ? !w->styleSheet().isEmpty() : false;
		// END ROBLOX CHANGES
        if (enabled)
        {
			// BEGIN ROBLOX CHANGES
			if (hasStyleSheet)
			{
				QColor color = btn->palette.brush(QPalette::Window).color();
				if (selected)
					color = color.lighter(110);
				p->fillRect(opt->rect, color);
			}
			// END ROBLOX CHANGES
            else if (pressed)
            {
                if (!checked)
                    p->fillRect(opt->rect, m_clrHighlightDn);
            }
            else if (selected || checked || (opt->state & QStyle::State_HasFocus))
            {
                p->fillRect(opt->rect, d.m_clrHighlight);
            }
        }

        QPen penSave = p->pen();
        p->setPen(m_clrBarDkShadow);
        p->drawRect(btn->rect.adjusted(0, 0, -1, -1));
        p->setPen(penSave);
        return true;
    }
    return false;
}

// shared
/*! \internal */
bool OfficePaintManager2013::drawPanelTipLabel(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w)
    p->fillRect(opt->rect, m_clrBarHilite);
    QPen oldPen = p->pen();
    p->setPen(m_clrBarShadow);
    p->drawRect(opt->rect.adjusted(0, 0, -1, -1));
    p->setPen(oldPen);
    return true;
}

/*! \internal */
bool OfficePaintManager2013::drawControlEdit(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
	// BEGIN ROBLOX CHANGES
    //Q_UNUSED(w)
	// END ROBLOX CHANGES
    QTN_D_STYLE(const OfficeStyle)

    if (const QStyleOptionFrame* panel = qstyleoption_cast<const QStyleOptionFrame*>(opt)) 
    {
		// BEGIN ROBLOX CHANGES
        if(w && !qobject_cast<const QComboBox*>(w->parentWidget()))
		//if(!qobject_cast<const QComboBox*>(w->parentWidget()))
		// BEGIN ROBLOX CHANGES
        {
            bool enabled  = opt->state & QStyle::State_Enabled;
//            bool hasFocus = opt->state & QStyle::State_HasFocus;
            p->fillRect(panel->rect.adjusted(0, 0, -1, -1), enabled ? d.m_clrControlEditNormal : m_clrControlEditDisabled);

            if ( panel->lineWidth > 0 )
                baseStyle()->proxy()->drawPrimitive(QStyle::PE_FrameLineEdit, panel, p, w);
        }
        return true;
    }
    return false;
}

/*! \internal */
bool OfficePaintManager2013::drawFrameLineEdit(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle)

    if (const QStyleOptionFrame* panel = qstyleoption_cast<const QStyleOptionFrame*>(opt))
    {
        bool enabled     = panel->state & QStyle::State_Enabled;
        bool highlighted = panel->state & QStyle::State_MouseOver;
        bool hasFocus    = panel->state & QStyle::State_HasFocus;

        QColor colorBorder = d.m_clrEditCtrlBorder;

        if (!enabled)
            colorBorder = m_clrTextDisabled;
        else if (highlighted || hasFocus)
            colorBorder = d.m_clrAccent;

        QPen savePen = p->pen();
        p->setPen(colorBorder);
        p->drawRect(panel->rect.adjusted(0, 0, -1, -1));
        p->setPen(savePen);
        return true;
    }
    return false;
}

/*! \internal */
bool OfficePaintManager2013::drawComboBox(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const
{
    QTN_D_STYLE(const OfficeStyle)
    if (const QStyleOptionComboBox* cmb = qstyleoption_cast<const QStyleOptionComboBox*>(opt))
    {
        QStyle::State flags = cmb->state;
        QStyle::SubControls sub = cmb->subControls;

        bool enabled  = flags & QStyle::State_Enabled;
        bool highlighted = flags & QStyle::State_MouseOver;
        bool dropped  = cmb->activeSubControls == QStyle::SC_ComboBoxArrow && ((flags & QStyle::State_On) || (flags & QStyle::State_Sunken));
        bool dropButtonHot = cmb->activeSubControls == QStyle::SC_ComboBoxArrow && (flags & QStyle::State_MouseOver);
        bool hasFocus = flags & QStyle::State_HasFocus;

        if (cmb->frame) 
        {
            QRect r = cmb->rect.adjusted(0, 0, -1, -1);
            p->fillRect(r, enabled ? d.m_clrControlEditNormal : m_clrControlEditDisabled);

            QColor colorBorder = d.m_clrEditCtrlBorder;

            if (!enabled)
                colorBorder = m_clrTextDisabled;
            else if (highlighted || hasFocus)
                colorBorder = d.m_clrAccent;

            QPen savePen = p->pen();
            p->setPen(colorBorder);
            p->drawRect(r);
            p->setPen(savePen);
        }

        if (sub & QStyle::SC_ComboBoxArrow)
        {
            QRect rcBtn = baseStyle()->proxy()->subControlRect(QStyle::CC_ComboBox, opt, QStyle::SC_ComboBoxArrow, w);
            rcBtn.adjust(0, 1, -1, -1);

            QColor clrFill = m_clrBarHilite;
            if (dropped)
            {
                p->fillRect(rcBtn, m_clrHighlightDn);
                clrFill = m_clrHighlightDn;
            }
            else if (dropButtonHot || hasFocus)
            {
                p->fillRect(rcBtn, m_clrAccentLight);
                clrFill = m_clrAccentLight;
            }
            else
                p->fillRect(rcBtn, clrFill);
            drawIconByColor(p, OfficePaintManager2013::Icon_ArowDown, rcBtn, clrFill);
        }
        return true;
    }
    return false;
}

// for SpinBox
/*! \internal */
bool OfficePaintManager2013::drawSpinBox(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle)

    if (const QStyleOptionSpinBox* sb = qstyleoption_cast<const QStyleOptionSpinBox *>(opt))
    {
        bool highlighted = sb->state & QStyle::State_MouseOver;
        bool enabled  = sb->state & QStyle::State_Enabled;
        bool hasFocus = sb->state & QStyle::State_HasFocus;

        if (sb->frame && (sb->subControls & QStyle::SC_SpinBoxFrame)) 
        {
            QRect r = sb->rect.adjusted(0, 0, -1, -1);
            p->fillRect(r, enabled ? d.m_clrControlEditNormal : m_clrControlEditDisabled);

            QColor colorBorder = d.m_clrEditCtrlBorder;

            if (!enabled)
                colorBorder = m_clrTextDisabled;
            else if (highlighted || hasFocus)
                colorBorder = d.m_clrAccent;

            QPen savePen = p->pen();
            p->setPen(colorBorder);
            p->drawRect(r);
            p->setPen(savePen);
        }

        QStyleOptionSpinBox copy = *sb;
        copy.state = QStyle::State_None;
        copy.subControls |= QStyle::SC_SpinBoxUp;
        QRect rcTop = baseStyle()->proxy()->subControlRect(QStyle::CC_SpinBox, &copy, QStyle::SC_SpinBoxUp, w);

        copy = *sb;

        OfficePaintManager2013::ImageIcons index[2][2] = {{OfficePaintManager2013::Icon_ArowUp, OfficePaintManager2013::Icon_ArowDown}, 
            {OfficePaintManager2013::Icon_ArowRight, OfficePaintManager2013::Icon_ArowLeft}};

        if (sb->subControls & QStyle::SC_SpinBoxUp)
        {
            bool pressedButton = false;
            bool hotButton = false;
            bool enabledIndicator = true;

            rcTop = baseStyle()->proxy()->subControlRect(QStyle::CC_SpinBox, sb, QStyle::SC_SpinBoxUp, w);

            if (!(sb->stepEnabled & QAbstractSpinBox::StepUpEnabled) || !(sb->state & QStyle::State_Enabled))
                enabledIndicator = false;
            else if (sb->activeSubControls == QStyle::SC_SpinBoxUp && (sb->state & QStyle::State_Sunken))
                pressedButton = true;
            else if (sb->activeSubControls == QStyle::SC_SpinBoxUp && (sb->state & QStyle::State_MouseOver))
                hotButton = true;

            if (pressedButton || hotButton)
            {
                p->fillRect(rcTop.adjusted(0, 1, -1, -1), pressedButton ? m_clrHighlightDn : d.m_clrHighlight);
            }
            drawIcon(p, rcTop, index[false ? 1 : 0][0], enabledIndicator ? OfficePaintManager2013::Black : OfficePaintManager2013::Gray, QSize(9, 9));
        }

        if (sb->subControls & QStyle::SC_SpinBoxDown) 
        {
            bool pressedButton = false;
            bool hotButton = false;
            bool enabledIndicator = true;

            QRect rcBottom = baseStyle()->proxy()->subControlRect(QStyle::CC_SpinBox, sb, QStyle::SC_SpinBoxDown, w);

            if (!(sb->stepEnabled & QAbstractSpinBox::StepDownEnabled) || !(sb->state & QStyle::State_Enabled))
                enabledIndicator = false;
            else if (sb->activeSubControls == QStyle::SC_SpinBoxDown && (sb->state & QStyle::State_Sunken))
                pressedButton = true;
            else if (sb->activeSubControls == QStyle::SC_SpinBoxDown && (sb->state & QStyle::State_MouseOver))
                hotButton = true;

            if (pressedButton || hotButton)
            {
                p->fillRect(rcBottom.adjusted(0, 0, -1, -2), pressedButton ? m_clrHighlightDn : d.m_clrHighlight);
            }
            drawIcon(p, rcBottom, index[false ? 1 : 0][1], enabledIndicator ? OfficePaintManager2013::Black : OfficePaintManager2013::Gray, QSize(9, 9));
        }

        return true;
    }
    return false;
}

// for ToolBox
/*! \internal */
bool OfficePaintManager2013::drawToolBoxTabShape(const QStyleOption*, QPainter*, const QWidget*) const
{
    return false;
}

// for TabBar
/*! \internal */
bool OfficePaintManager2013::drawTabBarTabShape(const QStyleOption* option, QPainter* p, const QWidget* widget) const
{
    if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option))
    {
        QRect rect = option->rect;
        QRect fillRect = rect;
        bool isHot = tab->state & QStyle::State_MouseOver;
        bool selected = tab->state & QStyle::State_Selected;
        bool lastTab = tab->position == QStyleOptionTab::End;
        bool firstTab = tab->position == QStyleOptionTab::Beginning;
        bool onlyOne = tab->position == QStyleOptionTab::OnlyOneTab;
        int borderThickness = baseStyle()->proxy()->pixelMetric(QStyle::PM_DefaultFrameWidth, option, widget);
        int tabOverlap = baseStyle()->proxy()->pixelMetric(QStyle::PM_TabBarTabOverlap, option, widget);

        if ( tab->direction == Qt::RightToLeft && (tab->shape == QTabBar::RoundedNorth || tab->shape == QTabBar::RoundedSouth)) 
        {
            bool temp = firstTab;
            firstTab = lastTab;
            lastTab = temp;
        }

        bool begin = firstTab || onlyOne;
        bool end = lastTab || onlyOne;
        switch (tab->shape) 
        {
            case QTabBar::RoundedNorth:
                    rect.adjust(begin ? tabOverlap : 0, tabOverlap, end ? -tabOverlap : 0, 0); fillRect = rect;
                    fillRect.adjust(1, 1, 0, 0);
                break;
            case QTabBar::RoundedSouth:
                    rect.adjust(begin ? tabOverlap : 0, -tabOverlap, end ? -tabOverlap : 0, -tabOverlap); fillRect = rect;
                    fillRect.adjust(1, 0, 0, -1);
                break;
            case QTabBar::RoundedEast:
                    rect.adjust(-1, begin ? tabOverlap : 0, -tabOverlap, end ? -tabOverlap : 0); fillRect = rect;
                    fillRect.adjust(1, 1, 0, 0);
                break;
            case QTabBar::RoundedWest:
                    rect.adjust(tabOverlap, begin ? tabOverlap : 0, 0, end ? -tabOverlap : 0); fillRect = rect;
                    fillRect.adjust(1, 1, 0, 0);
                break;
            default:
                break;
        }

        if (selected)
        {
            QPen savePen = p->pen();
            p->setPen(QPen(m_clrBarDkShadow, borderThickness));
            p->drawRect(rect);
            p->setPen(savePen);
        }

        if (isHot && !selected)
            p->fillRect(rect.adjusted(1, 1, -1, -1), m_clrAccentLight);
        else 
        if (selected)
            p->fillRect(fillRect, m_clrBarFace);

    }
    return true;
}

// for QForm
/*! \internal */
bool OfficePaintManager2013::drawShapedFrame(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    bool ret = false;
    if (const QMdiArea* mdiArea = qobject_cast<const QMdiArea*>(w))
    {
        if (const QStyleOptionFrameV3* f = qstyleoption_cast<const QStyleOptionFrameV3*>(opt)) 
        {
            int frameShape  = f->frameShape;
            int frameShadow = QFrame::Plain;
            if (f->state & QStyle::State_Sunken)
                frameShadow = QFrame::Sunken;
            else if (f->state & QStyle::State_Raised)
                frameShadow = QFrame::Raised;

            switch (frameShape) 
            {
            case QFrame::Panel:
                {
                    if (frameShadow == QFrame::Sunken)
                    {
                        QRect rect = opt->rect;
                        if (QTabBar* tabBar = mdiArea->findChild<QTabBar*>())
                        {
                            p->fillRect(opt->rect, m_clrBarFace);

                            switch (mdiArea->tabPosition()) 
                            {
                            case QTabWidget::North:
                                rect.adjust(0, tabBar->sizeHint().height(), 0, 0);
                                break;
                            case QTabWidget::South:
                                rect.adjust(0, 0, 0, -tabBar->sizeHint().height());
                                break;
                            case QTabWidget::West:
                                rect.adjust(tabBar->sizeHint().width(), 0, 0, 0);
                                break;
                            case QTabWidget::East:
                                rect.adjust(0, 0, -tabBar->sizeHint().width(), 0);
                                break;
                            default:
                                break;
                            }
                            DrawHelpers::draw3dRectEx(*p, rect, m_clrBarDkShadow, m_clrBarDkShadow);
                        }
                        ret = true;
                    }
                    break;
                }
            default:
                break;
            }
        }
    }
    return ret;
}

// for ScrollBar
/*! \internal */
bool OfficePaintManager2013::drawScrollBar(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const
{
    QTN_D_STYLE(const OfficeStyle)
    if (const QStyleOptionSlider* scrollbar = qstyleoption_cast<const QStyleOptionSlider*>(opt))
    {
        // Make a copy here and reset it for each primitive.
        QStyleOptionSlider newScrollbar(*scrollbar);
        if (scrollbar->minimum == scrollbar->maximum)
            newScrollbar.state &= ~QStyle::State_Enabled; //do not draw the slider.

        QStyle::State saveFlags = scrollbar->state;

        newScrollbar.state = saveFlags;
        newScrollbar.rect = opt->rect;

        p->fillRect(scrollbar->rect, m_clrBarShadow);

        QPen savePen = p->pen();
        p->setPen(d.m_clrMenubarBorder);
        if (scrollbar->subControls & QStyle::SC_ScrollBarSubLine) 
        {
            QRect rc = baseStyle()->proxy()->subControlRect(QStyle::CC_ScrollBar, &newScrollbar, QStyle::SC_ScrollBarSubLine, w);
            if (opt->state & QStyle::State_Horizontal)
                p->drawRect(scrollbar->rect.adjusted(rc.width() + 1, 0, -1, -1));
            else
                p->drawRect(scrollbar->rect.adjusted(0, rc.height() + 1, -1, -1));
        }

        if (scrollbar->subControls & QStyle::SC_ScrollBarAddLine) 
        {
            QRect rc = baseStyle()->proxy()->subControlRect(QStyle::CC_ScrollBar, &newScrollbar, QStyle::SC_ScrollBarAddLine, w);
            if (opt->state & QStyle::State_Horizontal)
                p->drawRect(scrollbar->rect.adjusted(0, 0, -(rc.width() + 1), -1));
            else
                p->drawRect(scrollbar->rect.adjusted(0, 0, -1, -(rc.height() + 1)));
        }
        p->setPen(savePen);

        if (scrollbar->subControls & QStyle::SC_ScrollBarSubLine) 
        {
            newScrollbar.rect = scrollbar->rect;
            newScrollbar.state = saveFlags;
            newScrollbar.rect = baseStyle()->proxy()->subControlRect(QStyle::CC_ScrollBar, &newScrollbar, QStyle::SC_ScrollBarSubLine, w);

            if (opt->state & QStyle::State_Horizontal) 
                newScrollbar.rect.adjust(0, 0, 0, -1);
            else
                newScrollbar.rect.adjust(0, 0, -1, 0);

            if (newScrollbar.rect.isValid()) 
                baseStyle()->proxy()->drawControl(QStyle::CE_ScrollBarSubLine, &newScrollbar, p, w);
        }

        if (scrollbar->subControls & QStyle::SC_ScrollBarAddLine) 
        {
            newScrollbar.rect = scrollbar->rect;
            newScrollbar.state = saveFlags;
            newScrollbar.rect = baseStyle()->proxy()->subControlRect(QStyle::CC_ScrollBar, &newScrollbar, QStyle::SC_ScrollBarAddLine, w);

            if (opt->state & QStyle::State_Horizontal) 
                newScrollbar.rect.adjust(0, 0, -1, -1);
            else
                newScrollbar.rect.adjust(0, 0, -1, -1);

            if (newScrollbar.rect.isValid()) 
                baseStyle()->proxy()->drawControl(QStyle::CE_ScrollBarAddLine, &newScrollbar, p, w);
        }

        if (scrollbar->subControls & QStyle::SC_ScrollBarSlider) 
        {
            newScrollbar.rect = scrollbar->rect;
            newScrollbar.state = saveFlags;
            newScrollbar.rect = baseStyle()->proxy()->subControlRect(QStyle::CC_ScrollBar, &newScrollbar, QStyle::SC_ScrollBarSlider, w);

            if (opt->state & QStyle::State_Horizontal) 
                newScrollbar.rect.adjust(0, 0, 0, -1);
            else
                newScrollbar.rect.adjust(0, 0, -1, 0);

            if (newScrollbar.rect.isValid()) 
                baseStyle()->proxy()->drawControl(QStyle::CE_ScrollBarSlider, &newScrollbar, p, w);
        }

        return true;
    }
    return false;
}

/*! \internal */
bool OfficePaintManager2013::drawScrollBarLine(QStyle::ControlElement element, const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    if (const QStyleOptionSlider* scrollbar = qstyleoption_cast<const QStyleOptionSlider*>(opt))
    {
        bool enabled  = opt->state & QStyle::State_Enabled;
        bool highlighted = opt->state & QStyle::State_MouseOver;
        bool pressed  = opt->state & QStyle::State_Sunken;

        bool pressButton = false;
//        bool highlightedButton = false;
        if (highlighted && pressed)
        {
            if (((scrollbar->activeSubControls & QStyle::SC_ScrollBarAddLine) && (element == QStyle::CE_ScrollBarAddLine)) ||
                ((scrollbar->activeSubControls & QStyle::SC_ScrollBarSubLine) && (element == QStyle::CE_ScrollBarSubLine)) )
                pressButton = true;
        }
/*
        else if (highlighted)
        {
            if (((scrollbar->activeSubControls & QStyle::SC_ScrollBarAddLine) && (element == QStyle::CE_ScrollBarAddLine)) ||
                ((scrollbar->activeSubControls & QStyle::SC_ScrollBarSubLine) && (element == QStyle::CE_ScrollBarSubLine)) )
                highlightedButton = true;
        }
*/
        QColor clrFill = pressButton || !enabled ? m_clrBarShadow : m_clrControl;
        p->fillRect(scrollbar->rect, clrFill);

        QPen savePen = p->pen();
        p->setPen(highlighted ? m_clrEditBoxBorder : m_clrBarDkShadow);
        p->drawRect(scrollbar->rect);
        p->setPen(savePen);

        OfficePaintManager2013::ImageIcons index;
        if (opt->state & QStyle::State_Horizontal)
            index = element == QStyle::CE_ScrollBarAddLine ? OfficePaintManager2013::Icon_ArowRightTab3d : OfficePaintManager2013::Icon_ArowLeftTab3d;
        else
            index = element == QStyle::CE_ScrollBarAddLine ? OfficePaintManager2013::Icon_ArowDownLarge : OfficePaintManager2013::Icon_ArowUpLarge;

        OfficePaintManager2013::ImageState state = enabled ? stateByColor(clrFill, true) : OfficePaintManager2013::Gray;
        drawIcon(p, opt->rect, index, state, QSize(9, 9));
        return true;
    }
    return false;
}

/*! \internal */
bool OfficePaintManager2013::drawScrollBarSlider(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    if (const QStyleOptionSlider* scrollbar = qstyleoption_cast<const QStyleOptionSlider*>(opt)) 
    {
        if (!scrollbar->rect.isEmpty())
        {
            bool enabled     = scrollbar->state & QStyle::State_Enabled;
            bool highlighted = scrollbar->state & QStyle::State_MouseOver;
            bool pressed     = scrollbar->state & QStyle::State_Sunken;

            QRect rc(scrollbar->rect);
            bool show = scrollbar->state & QStyle::State_Horizontal ? rc.width() > 7 : rc.height() > 7;
            if (show)
            {
                QColor clrFill = pressed || !enabled ? m_clrBarShadow : m_clrBarLight;//m_clrControl;
                p->fillRect(rc, clrFill);

                QPen savePen = p->pen();
                QPen newPen(QBrush(highlighted ? m_clrEditBoxBorder : m_clrBarDkShadow), 2);
                p->setPen(newPen);
                if (opt->state & QStyle::State_Horizontal)
                    p->drawRect(rc.adjusted(0, 0, 0, 1));
                else
                    p->drawRect(rc.adjusted(0, 0, 1, 0));
                p->setPen(savePen);
            }
        }
        return true;
    }
    return false;
}

/*! \internal */
bool OfficePaintManager2013::drawScrollBarPage(QStyle::ControlElement element, const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(element);
    Q_UNUSED(opt);
    Q_UNUSED(p);
    Q_UNUSED(w);
    return true;
}

// for stausBar
/*! \internal */
bool OfficePaintManager2013::drawPanelStatusBar(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);

    QTN_D_STYLE(const OfficeStyle);
    p->fillRect(opt->rect,  d.m_clrBackgroundLight);
    return true;
}

// for SizeGrip
/*! \internal */
bool OfficePaintManager2013::drawSizeGrip(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    QTN_D_STYLE(const OfficeStyle);
    QPoint pt(opt->rect.right() - 3, opt->rect.bottom() - 3);
    for (int y = 0; y < 3; y++)
    {
        for (int x = 0; x < 3 - y; x++)
            p->fillRect(QRect(QPoint(pt.x() + 1 - x * 4, pt.y() + 1 - y * 4), QSize(2, 2)), d.m_clrAccent);
    }
    return true;
}

// for ViewItem
/*! \internal */
bool OfficePaintManager2013::drawPanelItemViewItem(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(opt);
    Q_UNUSED(p);
    Q_UNUSED(w);
    return false;
}

/*! \internal */
bool OfficePaintManager2013::drawHeaderSection(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    bool isPressed = opt->state & QStyle::State_Sunken;
    bool isHighlighted = opt->state & QStyle::State_MouseOver;

    QColor clrFill  = m_clrBarFace;
    QColor clrBorder = m_clrBarShadow;

    if (isPressed)
    {
        clrFill  = m_clrHighlightDn;
        clrBorder = QColor(255, 255, 255);
    }
    else if (isHighlighted)
    {
        clrFill  = m_clrBarLight;
        clrBorder = m_clrAccentLight;
    }

    p->fillRect(opt->rect, clrFill);

    QPen penSave = p->pen();
    p->setPen(clrBorder);
    p->drawRect(opt->rect.adjusted(0, 0, -1, -1));
    p->setPen(penSave);
    return true;
}

/*! \internal */
bool OfficePaintManager2013::drawHeaderEmptyArea(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    p->fillRect(opt->rect, m_clrBarFace);

    p->fillRect(QRect(QPoint(opt->rect.left(), opt->rect.bottom()), QSize(opt->rect.width(), 1)), 
        m_clrBarShadow);

    p->fillRect(QRect(QPoint(opt->rect.right(), opt->rect.top()), QSize(1, opt->rect.height())), 
        m_clrBarShadow);

    return true;
}

/*! \internal */
bool OfficePaintManager2013::drawIndicatorHeaderArrow(const QStyleOption*, QPainter*, const QWidget*) const
{
    return false;
}

/*! \internal */
bool OfficePaintManager2013::drawIndicatorArrow(QStyle::PrimitiveElement pe, const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    Q_UNUSED(w);
    switch(pe)
    {
        case QStyle::PE_IndicatorArrowUp:
        case QStyle::PE_IndicatorArrowDown:
        case QStyle::PE_IndicatorArrowRight:
        case QStyle::PE_IndicatorArrowLeft:
            {
                bool enabled = opt->state & QStyle::State_Enabled;    
                bool selected = opt->state & QStyle::State_Selected;
                QColor color = textColor(selected, false, enabled, false, false/*popuped*/, TypeNormal, BarTop);

                QPalette palette = opt->palette;
                palette.setColor(QPalette::ButtonText, color);

                if (opt->rect.width() <= 1 || opt->rect.height() <= 1)
                    break;

                QRect r = opt->rect;
                int size = qMin(r.height(), r.width());

                int border = (size/2) - 3;

                int sqsize = 2*(size/2);
                QImage image(sqsize, sqsize, QImage::Format_ARGB32_Premultiplied);
                image.fill(0);
                QPainter imagePainter(&image);

                QPolygon a;
                switch (pe) 
                {
                    case QStyle::PE_IndicatorArrowUp:
                        a.setPoints(3, border, sqsize/2,  sqsize/2, border,  sqsize - border, sqsize/2);
                        break;
                    case QStyle::PE_IndicatorArrowDown:
                        a.setPoints(3, border, sqsize/2,  sqsize/2, sqsize - border,  sqsize - border, sqsize/2);
                        break;
                    case QStyle::PE_IndicatorArrowRight:
                        a.setPoints(3, sqsize - border, sqsize/2,  sqsize/2, border,  sqsize/2, sqsize - border);
                        break;
                    case QStyle::PE_IndicatorArrowLeft:
                        a.setPoints(3, border, sqsize/2,  sqsize/2, border,  sqsize/2, sqsize - border);
                        break;
                    default:
                        break;
                }

                QRect bounds = a.boundingRect();
                int sx = sqsize / 2 - bounds.center().x() - 1;
                int sy = sqsize / 2 - bounds.center().y() - 1;
                imagePainter.translate(sx, sy);
                imagePainter.setPen(palette.buttonText().color());
                imagePainter.setBrush(palette.buttonText());

                if (!enabled) 
                {
                    imagePainter.translate(1, 1);
                    imagePainter.setBrush(palette.light().color());
                    imagePainter.setPen(palette.light().color());
                    imagePainter.drawPolygon(a);
                    imagePainter.translate(-1, -1);
                    imagePainter.setBrush(palette.mid().color());
                    imagePainter.setPen(palette.mid().color());
                }

                imagePainter.drawPolygon(a);
                imagePainter.end();

                QPixmap pixmap = QPixmap::fromImage(image);

                int xOffset = r.x() + (r.width() - size)/2;
                int yOffset = r.y() + (r.height() - size)/2;

                p->drawPixmap(xOffset, yOffset, pixmap);
                return true;
            }
            break;
        default:
            return false;
    }
    return false;
}

/*! \internal */
bool OfficePaintManager2013::drawGalleryToolButton(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    QTN_D_STYLE(const OfficeStyle)
    if (const QStyleOptionToolButton* toolbutton = qstyleoption_cast<const QStyleOptionToolButton*>(opt))
    {
        if (widget)
        {
            OfficePaintManager2013::ImageIcons id = OfficePaintManager2013::Icon_Check;
            if(widget->property(qtn_PopupButtonGallery).toBool())
                id = OfficePaintManager2013::Icon_CustomizeArowDown;
            else if(widget->property(qtn_ScrollUpButtonGallery).toBool())
                id = OfficePaintManager2013::Icon_ArowUp;
            else if(widget->property(qtn_ScrollDownButtonGallery).toBool())
                id = OfficePaintManager2013::Icon_ArowDown;

            if (id != OfficePaintManager2013::Icon_Check)
            {
                bool enabled     = opt->state & QStyle::State_Enabled;
                bool highlighted = opt->state & QStyle::State_MouseOver;
                bool pressed     = opt->state & QStyle::State_Sunken;

                QRect rect = toolbutton->rect.adjusted(0, 1, -1, -1);
                if (pressed && enabled)
                    p->fillRect(rect, m_clrHighlightDn);
                else if ((pressed || highlighted) && enabled)
                    p->fillRect(rect, d.m_clrHighlight);
                else
                    p->fillRect(rect, QColor(255, 255, 255));

                drawIcon(p, rect, id, OfficePaintManager2013::White);
                drawIcon(p, rect, id, enabled ? OfficePaintManager2013::Black : OfficePaintManager2013::Gray);

                return true;
            }
        }
    }
    return false;
}

/*! \internal */
void OfficePaintManager2013::drawMenuItemSeparator(const QStyleOption* opt, QPainter* p, const QWidget* widget) const
{
    QTN_D_STYLE(const OfficeStyle)
    if (const QStyleOptionMenuItem* menuitem = qstyleoption_cast<const QStyleOptionMenuItem*>(opt)) 
    {
        if (menuitem->text.isEmpty())
        {
            int x, y, w, h;
            menuitem->rect.getRect(&x, &y, &w, &h);

            // windows always has a check column, regardless whether we have an icon or not
            const int iconSize = baseStyle()->proxy()->pixelMetric(QStyle::PM_ToolBarIconSize, opt, widget);
            int yoff = (y-1 + h / 2);
            int xIcon = iconSize;    
            if(qobject_cast<const OfficePopupMenu*>(widget))
                xIcon = 0;

            QPen penSave = p->pen();
            p->setPen(d.m_clrMenuPopupSeparator);
            p->drawLine(x + 2 + xIcon, yoff, x + w - 4, yoff);
            p->setPen(penSave);
        }
        else
        {
            p->save();

            p->fillRect(menuitem->rect, m_clrBarShadow);

            QRect rectText = menuitem->rect;
            rectText.adjust(iTextMargin, 0, -iTextMargin, -iTextMargin);
//            rectText.setBottom( rectText.bottom() /*- 2*/);

            int text_flags = Qt::AlignVCenter | Qt::TextSingleLine;
            QFont font = menuitem->font;
            font.setBold(true);
            p->setFont(font);

            p->setPen(opt->state & QStyle::State_Enabled ? d.m_clrMenuPopupText : d.m_clrMenuBarGrayText);
            p->drawText(rectText, text_flags, menuitem->text);

            p->restore();
        }
    }
}

/*! \internal */
void OfficePaintManager2013::drawRectangle(QPainter* p, const QRect& rect, bool selected, bool pressed, bool enabled, bool checked, bool popuped,
    BarType barType, BarPosition barPos) const
{
    Q_UNUSED(popuped);
    Q_UNUSED(enabled);
    Q_UNUSED(pressed);
    QTN_D_STYLE(OfficeStyle)
    if (barType != TypePopup)
    {
        QRect rc = rect;
        rc.adjust(1, 1, -1, -1);
        QColor color = QColor();
        if (checked)
            color = m_clrHighlightDn;
        else if (selected)
            color = d.m_clrHighlight;

        if (color.isValid())
            p->fillRect(rc, color);

        if (checked && selected)
        {
            QPen oldPen = p->pen();
            p->setPen(m_clrHighlightChecked);
            p->drawRect(rc.adjusted(-1, -1, 0, 0));
            p->setPen(oldPen);
        }
    }
    else if (barPos == BarPopup && selected && barType == TypePopup)
    {
        QRect rc = rect;
        rc.adjust(1, 0, -1, 0);
        p->fillRect(rc, m_clrHighlightMenuItem);
        DrawHelpers::draw3dRectEx(*p, rc, m_clrMenuItemBorder, m_clrMenuItemBorder);
    }
}

/*! \internal */
void OfficePaintManager2013::setupPalette(QWidget* widget) const
{
    Q_UNUSED(widget);
}

/*! \internal */
bool OfficePaintManager2013::pixelMetric(QStyle::PixelMetric pm, const QStyleOption* opt, const QWidget* w, int& ret) const
{
    Q_UNUSED(opt);
    Q_UNUSED(w);
    switch(pm)
    {
        case QStyle::PM_MenuButtonIndicator:
            if (qobject_cast<const QToolButton*>(w))
            {
                ret = iImageWidth + (qobject_cast<QToolBar*>(w->parentWidget()) ? 0 : 6);
                return true;
            }
            break;
        default:
            break;
    };
    return false;

}

/*! \internal */
void OfficePaintManager2013::modifyColors()
{
    QTN_D_STYLE(OfficeStyle)
    m_clrAccentLight   = DrawHelpers::colorMakeLighter(d.m_clrAccent.rgb(), 0.5);
    m_clrAccentHilight = DrawHelpers::colorMakeLighter(d.m_clrAccent.rgb(), 0.2);
    m_clrAccentText    = d.m_clrAccent.rgb();

    d.m_clrFrameBorderActive0 = m_clrAccentText;
    d.m_clrFrameBorderInactive0 = QColor(131, 131, 131);
    d.m_clrSelectedText = m_clrAccentText;

    d.m_clrHighlight = DrawHelpers::colorMakeLighter(d.m_clrAccent.rgb(), 0.5);
    m_clrHighlightDn = DrawHelpers::colorMakeLighter(QColor(d.m_clrAccent.saturation(), d.m_clrAccent.saturation(), d.m_clrAccent.saturation()).rgb(), 0.6);
    m_clrHighlightChecked = d.m_clrAccent;
    
    m_clrBarShadow   = helper().getColor("Window", "BarShadow");
    m_clrBarText     = helper().getColor("Window", "BarText");
    m_clrBarHilite   = helper().getColor("Window", "BarHilite");
    m_clrBarDkShadow = helper().getColor("Window", "BarDkShadow");
    m_clrBarFace     = helper().getColor("Window", "BarFace"); 
    m_clrBarLight    = helper().getColor("Window", "BarLight"); 


    d.m_clrStatusBarShadow   = d.m_clrBackgroundLight;
    d.m_clrControlEditNormal = m_clrBarHilite; //m_clrBarLight;
    d.m_clrNormalCaptionText = m_clrBarText;
    d.m_clrActiveCaptionText = m_clrAccentText;
    d.m_clrEditCtrlBorder    = QColor(171, 171, 171);

    d.m_clrMenubarBorder     = DrawHelpers::colorMakeLighter(m_clrBarDkShadow.rgb()); // m_clrMenuBorder
    d.m_clrStatusBarText     = QColor(255, 255, 255);
    d.m_clrBackgroundLight   = QColor(255, 255, 255); // m_clrMenuLight
    d.m_clrDockBar           = QColor(255, 255, 255); 
    d.m_clrControlGalleryLabel = m_clrBarShadow;
    d.m_crBorderShadow = m_clrBarShadow;
    d.m_clrGridLineColor = m_clrBarShadow;
    d.m_clrMenuPopupText = m_clrBarText;
//    d.m_clrColumnSeparator
//    d.m_clrToolbarSeparator   = m_clrSeparator   
//    d.m_clrMenuPopupSeparator = m_clrMenuSeparator

    m_clrHighlightMenuItem   = m_clrAccentLight; // m_clrHighlightMenuItem
    m_clrMenuItemBorder      = d.m_clrHighlight; // m_clrMenuItemBorder 

    m_clrTextDisabled        = m_clrBarDkShadow;
    m_clrControlEditDisabled = m_clrBarFace;
    m_clrControl             = m_clrBarHilite;
    m_clrEditBoxBorder       = QColor(171, 171, 171);

    m_ImagesBlack  = mapTo3dColors(m_ImagesSrc, QColor(0, 0, 0), QColor(59, 59, 59));
    m_ImagesBlack2 = mapTo3dColors(m_ImagesSrc, QColor(0, 0, 0), m_clrAccentText);
}

/*! \internal */
OfficePaintManager2013::ImageState OfficePaintManager2013::stateByColor(const QColor& color, bool backgroundColor) const
{
    if (!color.isValid())
        return Black;

    qreal h, l, s, a;
    color.getHslF(&h, &s, &l, &a);

    if (backgroundColor)
        return l < 0.7 ? White : Black;
    else
        return l > 0.7 ? White : Black;
}

/*! \internal */
void OfficePaintManager2013::drawIcon(QPainter* p, const QRect& rectImage, ImageIcons index, ImageState state, const QSize& sizeImageDest) const
{
    const QSize sizeImage = (sizeImageDest == QSize (0, 0)) ? QSize(9, 9) : sizeImageDest;

    QPoint ptImage(rectImage.left() + (rectImage.width()  - sizeImage.width()) / 2  + ((rectImage.width()  - sizeImage.width())  % 2), 
                   rectImage.top()  + (rectImage.height() - sizeImage.height()) / 2 + ((rectImage.height() - sizeImage.height()) % 2));

    drawIcon(p, ptImage, index, state, sizeImage);
}

/*! \internal */
void OfficePaintManager2013::drawIcon(QPainter* p, const QPoint& pos, ImageIcons index, ImageState state, const QSize& sz) const
{
    QPixmap images = (state == Black)  ? m_ImagesBlack  :
        (state == Gray)   ? m_ImagesGray   : 
        (state == DkGray) ? m_ImagesDkGray : 
        (state == LtGray) ? m_ImagesLtGray : 
        (state == White)  ? m_ImagesWhite  : m_ImagesBlack2;

    QRect rcSrc(QPoint((int)index * sz.width(), 0), sz);
    p->drawPixmap(pos, images, rcSrc);
}

/*! \internal */
QPixmap OfficePaintManager2013::mapTo3dColors(const QPixmap& px, const QColor& clrSrc, const QColor& clrDest) const
{
    QImage img = px.toImage();
    for (int y = 0; y < img.height(); ++y) 
    {
        QRgb *scanLine = (QRgb*)img.scanLine(y);
        for (int x = 0; x < img.width(); ++x) 
        {
            QRgb pixel = *scanLine;
            if (pixel == clrSrc.rgb()) 
                *scanLine = clrDest.rgb();
            ++scanLine;
        }
    }
    return QPixmap::fromImage(img);
}

/*! \internal */
void OfficePaintManager2013::loadPixmap()
{
    m_ImagesSrc = QPixmap(":/res/menuimg-pro24.png");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QImage img = m_ImagesSrc.toImage();
    img.setAlphaChannel(img.createMaskFromColor(clrTransparent.rgb(), Qt::MaskOutColor));
    m_ImagesSrc = QPixmap::fromImage(img);
#else
    m_ImagesSrc.setAlphaChannel(m_ImagesSrc.createMaskFromColor(clrTransparent, Qt::MaskOutColor));
#endif
}
