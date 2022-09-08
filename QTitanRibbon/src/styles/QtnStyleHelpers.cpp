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
#include <QSettings>
#include <QVariant>
#include <QPainter>
#include <QBrush>
#include <QStyleOption>
#include <QPixmapCache>
#include <QBitmap>
#include <QPushButton>
#include <QDialog>
#include <QMdiSubWindow>
#include <QStyleFactory>
#include <QBasicTimer>
#include <qevent.h>

#include <QToolButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QScrollBar>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif /* Q_OS_WIN*/

#include "QtnStyleHelpers.h"
#include "../src/ribbon/QtnRibbonPrivate.h"

using namespace Qtitan;


/*!
\class Qtitan::DrawHelpers
\internal
*/
DrawHelpers::DrawHelpers()
{
}

DrawHelpers::~DrawHelpers()
{
}

void DrawHelpers::draw3DRect(QPainter& pnt, QColor& col, int x, int y, int w, int h, bool up)
{
    QPen savePen = pnt.pen();

    QColor light = col.light(135);
    QColor dark = col.dark(140);
    pnt.setPen(up ? light : dark);
    pnt.drawLine(x, y, x+w, y);
    pnt.drawLine(x, y, x, y+h);
    pnt.setPen(up ? dark : light);
    pnt.drawLine(x, y+h, x+w, y+h);
    pnt.drawLine(x+w, y, x+w, y+h);
    pnt.setPen(col);
    pnt.drawPoint(x+w, y);
    pnt.drawPoint(x, y+h);

    pnt.setPen(savePen);
}

void DrawHelpers::draw3DRect(QPainter& pnt, const QColor& colLight, const QColor& colDark, int x, int y, int w, int h, bool up)
{
    QPen savePen = pnt.pen();

    QColor light = colLight;
    QColor dark = colDark;
    pnt.setPen(up ? light : dark);
    pnt.drawLine(x, y, x + w, y);
    pnt.drawLine(x, y, x, y + h);

    pnt.setPen(up ? dark : light);
    pnt.drawLine(x, y + h, x + w, y + h);
    pnt.drawLine(x + w, y, x + w, y + h);

    pnt.setPen(colLight);
    pnt.drawPoint(x+w, y);
    pnt.drawPoint(x, y+h);

    pnt.setPen(savePen);
}

void DrawHelpers::draw3dRectEx(QPainter& pnt, const QRect& rect, const QColor& clrTopLeft, const QColor& clrBottomRight)
{
    DrawHelpers::draw3dRectEx(pnt, rect.x(), rect.y(), rect.width(), rect.height(), clrTopLeft, clrBottomRight);
}

void DrawHelpers::draw3dRectEx(QPainter& pnt, int x, int y, int cx, int cy, const QColor& clrTopLeft, const QColor& clrBottomRight)
{
    pnt.fillRect(x, y, cx - 1, 1, clrTopLeft);
    pnt.fillRect(x, y, 1, cy - 1, clrTopLeft);
    pnt.fillRect(x + cx, y, -1, cy, clrBottomRight);
    pnt.fillRect(x, y + cy, cx, -1, clrBottomRight);
}

void DrawHelpers::drawRectangle(QPainter& pnt, const QRect& rect, const QColor& pen, const QColor& brush)
{
    if (brush.isValid()) 
        pnt.fillRect(rect, QBrush(brush));

    if (pen != brush && pen.isValid()) 
        draw3DRect(pnt, pen, pen, rect.x(), rect.y(), rect.width(), rect.height(), true);
}

void DrawHelpers::drawTriangle(QPainter& pnt, const QPoint& pt0, const QPoint& pt1, const QPoint& pt2, const QColor& color)
{
//    pnt.setRenderHint(QPainter::Antialiasing);
    QPen oldPen = pnt.pen();
    QBrush oldBrush = pnt.brush();

    pnt.setPen(color);
    pnt.setBrush(QBrush(color));

    QPoint pts[] = {pt0, pt1, pt2};
    pnt.drawPolygon(pts, 3);

    pnt.setBrush(oldBrush);
    pnt.setPen(oldPen);
}

void DrawHelpers::drawGradientFill(QPainter& pnt, const QRect& rect, const QColor& crFrom, const QColor& crTo, bool horz)
{
    pnt.save();
    pnt.setBrush(generateGradient(rect, crFrom, crTo, horz));
    pnt.setPen(Qt::transparent);
    pnt.drawRect(rect);
    pnt.restore();
}

void DrawHelpers::drawGradientFill4(QPainter& pnt, const QRect& rect, const QColor& crFrom1, const QColor& crTo1, 
                                    const QColor& crFrom2, const QColor& crTo2, bool horz, int percentage)
{
    percentage = qMin(qMax(0, percentage), 100);

    QRect rectFirst = rect;
    QRect rectSecond = rect;

    if (!horz)
    {
        rectFirst.setRight(rectFirst.left() + rectFirst.width() * percentage / 100);
        rectSecond.setLeft(rectFirst.right());
    }
    else
    {
        rectFirst.setBottom(rectFirst.top() + rectFirst.height() * percentage / 100);
        rectSecond.setTop(rectFirst.bottom());
    }

    drawGradientFill(pnt, rectFirst, crFrom1, crTo1, horz);
    drawGradientFill(pnt, rectSecond, crFrom2, crTo2, horz);
}

QRgb DrawHelpers::lightColor(QRgb clr, int nTint)
{
    double h, s, l;

    RGBtoHSL(clr, h, s, l);
    clr = HSLtoRGB(h, s, l + (1.0 - l) * (double)nTint / 100.0);

    return clr;
}

QRgb DrawHelpers::darkColor(QRgb clr, int nShade)
{
    double h, s, l;

    RGBtoHSL(clr, h, s, l);
    clr = HSLtoRGB(h, s, l * (1 + (double)nShade / 100.0));

    return clr;
}

void DrawHelpers::RGBtoHSL(QRgb clr, double& h, double& s, double& l)
{
    double r = (double)qRed(clr)/255.0;
    double g = (double)qGreen(clr)/255.0;
    double b = (double)qBlue(clr)/255.0;

    double maxcolor = qMax(r, qMax(g, b));
    double mincolor = qMin(r, qMin(g, b));

    l = (maxcolor + mincolor)/2;

    if (maxcolor == mincolor)
    {
        h = 0;
        s = 0;
    }
    else
    {
        if (l < 0.5)
            s = (maxcolor-mincolor)/(maxcolor + mincolor);
        else
            s = (maxcolor-mincolor)/(2.0-maxcolor-mincolor);

        if (r == maxcolor)
            h = (g-b)/(maxcolor-mincolor);
        else if (g == maxcolor)
            h = 2.0+(b-r)/(maxcolor-mincolor);
        else
            h = 4.0+(r-g)/(maxcolor-mincolor);

        h /= 6.0;
        if (h < 0.0) 
            h += 1;
    }
}

QRgb DrawHelpers::HSLtoRGB(double h, double s, double l)
{
    double r, g, b;
    double temp1, temp2;

    if (s == 0)
    {
        r = g = b = l;
    }
    else
    {
        if (l < 0.5)
            temp2 = l*(1.0 + s);
        else
            temp2 = l + s-l*s;

        temp1 = 2.0 * l-temp2;

        r = HueToRGB(temp1, temp2, h + 1.0/3.0);
        g = HueToRGB(temp1, temp2, h);
        b = HueToRGB(temp1, temp2, h - 1.0/3.0);
    }

    return qRgb((int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
}

QRgb DrawHelpers::blendColors(QRgb crA, QRgb crB, double fAmountA)
{
    double fAmountB = (1.0 - fAmountA);
    int btR = (int)(qRed(crA) * fAmountA + qRed(crB) * fAmountB);
    int btG = (int)(qGreen(crA) * fAmountA + qGreen(crB) * fAmountB);
    int btB = (int)(qBlue(crA) * fAmountA + qBlue(crB) * fAmountB);

    return qRgb(qMin(255, btR), qMin(255, btG), qMin(255, btB));
}

double DrawHelpers::HueToRGB(double temp1, double temp2, double temp3)
{
    if (temp3 < 0)
        temp3 = temp3 + 1.0;
    if (temp3 > 1)
        temp3 = temp3-1.0;

    if (6.0*temp3 < 1)
        return (temp1+(temp2-temp1)*temp3 * 6.0);

    else if (2.0*temp3 < 1)
        return temp2;

    else if (3.0*temp3 < 2.0)
        return (temp1+(temp2-temp1)*((2.0/3.0)-temp3)*6.0);

    return temp1;
}


int DrawHelpers::HueToRGB(int m1, int m2, int h)
{
    if (h < 0)
        h += 255;

    if (h > 255)
        h -= 255;

    if ((6 * h) < 255)
        return ((m1 + ((m2 - m1) / 255 * h * 6)) / 255);

    if ((2 * h) < 255)
        return m2 / 255;

    if ((3 * h) < (2 * 255))
        return ((m1 + (m2 - m1) / 255 * ((255 * 2 / 3) - h) * 6) / 255);

    return (m1 / 255);
}

QRgb DrawHelpers::HLStoRGB_ONE(double h, double l, double s)
{
    double r, g, b;
    double m1, m2;

    if(s==0) 
    {
        r=g=b=l;
    } 
    else 
    {
        if(l <=0.5)
            m2 = l*(1.0+s);
        else
            m2 = l+s-l*s;
        m1 = 2.0*l-m2;
        r = HueToRGB(m1, m2, h+1.0/3.0);
        g = HueToRGB(m1, m2, h);
        b = HueToRGB(m1, m2, h-1.0/3.0);
    }
    return qRgb((unsigned char)(r*255), (unsigned char)(g*255), (unsigned char)(b*255));
}

QRgb DrawHelpers::colorMakePale(const QRgb& clr, double lum)
{
    double h, s, l;
    DrawHelpers::RGBtoHSL(clr, h, s, l);
    return DrawHelpers::HLStoRGB_ONE(h, lum, s);
}

QRgb DrawHelpers::colorMakeDarker(const QRgb& clr, double ratio)
{
    double h;
    double s;
    double l;

    ratio = qMax(0., 1. - ratio);
    RGBtoHSL(clr, h, s, l);

    return HLStoRGB_ONE(h, qMin(1., l * ratio), qMin(1., s * ratio));
}

inline int qtn_round(double val)
{
    return int((val - int(val - 0.5)) >= 0.5 ? val + 0.5 : val - 0.5);
}

QRgb DrawHelpers::colorMakeLighter(const QRgb& clr, double ratio)
{
    double h;
    double s;
    double l;
    double min = 0.01;

    DrawHelpers::RGBtoHSL(clr, h, s, l);

    if (h < min && s < min && l < min)
    {
        int val = qtn_round(ratio * 255.0);
        return qRgb(val, val, val);
    }
    else
    {
        ratio += 1.;
        return HLStoRGB_ONE (h, qMin(1., l * ratio), qMin(1., s * ratio));
    }
}

QRgb DrawHelpers::pixelAlpha(const QRgb& srcPixel, int percent)
{
    QRgb clrFinal = qRgb( qMin(255, (qRed(srcPixel) * percent) / 100), 
        qMin(255, (qGreen(srcPixel) * percent) / 100), 
        qMin(255, (qBlue(srcPixel) * percent) / 100));
    return clrFinal;
}

QLinearGradient DrawHelpers::generateGradient(const QRect& rect, const QColor& crFrom, const QColor& crTo, bool bHorz)
{
    QLinearGradient gradient(rect.topLeft(), bHorz ? rect.bottomLeft() : rect.topRight());
    gradient.setColorAt(0.0, crFrom);
    gradient.setColorAt(1.0, crTo);
    return gradient;
}

#if defined(Q_WS_MAC)
float qtn_mac_get_scalefactor();
#endif

qreal DrawHelpers::dpiScaled(qreal value)
{
    static qreal scale = -1;
    if (scale < 0) 
    {
        scale = 1.0;
#if defined(Q_WS_WIN)
        {
            HDC hdcScreen = GetDC(0);
            int dpi = GetDeviceCaps(hdcScreen, LOGPIXELSX);
            ReleaseDC(0, hdcScreen);
            scale = dpi/96.0;
        }
#elif defined(Q_WS_MAC)
        scale = qtn_mac_get_scalefactor();
#endif
    }
    return value * scale;
}

int DrawHelpers::getDPI()
{
#ifdef Q_OS_WIN
    HDC displayDC = GetDC(0);
    int currentlogPixel = GetDeviceCaps(displayDC, LOGPIXELSY);
    ReleaseDC(0, displayDC);
    return currentlogPixel;
#endif /* Q_OS_WIN*/
    return -1;
}

int DrawHelpers::getDPIToPercent()
{
#ifdef Q_OS_WIN
    int percent = (double)getDPI()/0.96;
    return percent;
#endif /* Q_OS_WIN*/
    return -1;
}

QIcon DrawHelpers::createIconStyleWidget(QWidget* widget)
{
    if (/*qobject_cast<RibbonSeparator*>(widget)*/widget->metaObject()->className() == QString("Qtitan::RibbonSeparator"))
        return QIcon();

    else if (qobject_cast<QCheckBox*>(widget) || qobject_cast<QRadioButton*>(widget))
    {
        bool isRadio = qobject_cast<QRadioButton*>(widget);
        QStyleOptionButton opt;
        opt.initFrom(widget);
        int h = qApp->style()->proxy()->pixelMetric(QStyle::PM_IndicatorHeight, &opt, widget);
        opt.rect.setSize(QSize(h, h));

        QPixmap px(opt.rect.width(), opt.rect.height());
        px.fill(Qt::transparent);
        QPainter p(&px);

        opt.state &= ~QStyle::State_Off;
        opt.state &= ~QStyle::State_On;
        opt.state &= ~QStyle::State_NoChange;
        opt.state |= QStyle::State_On;

        qApp->style()->drawPrimitive(isRadio ? QStyle::PE_IndicatorRadioButton : QStyle::PE_IndicatorCheckBox, &opt, &p, 0);

        return QIcon(px);
    }
    return QIcon();
}

QPixmap DrawHelpers::createIconExStyleWidget(QWidget* widget, int width, int height, bool& iconView)
{
    if (widget == Q_NULL)
        return QPixmap();

    if (qobject_cast<QToolButton*>(widget))
    {
        iconView = true;

        class QtnHackToolButton : public QToolButton { friend class Qtitan::DrawHelpers; };

        QStyleOptionToolButton opt;
        ((QtnHackToolButton*)widget)->initStyleOption(&opt);

        if (opt.features & QStyleOptionToolButton::HasMenu)
        {
            int mbi = qApp->style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, widget);
            opt.rect.setSize(QSize(mbi, mbi));

            QPixmap px(opt.rect.width(), opt.rect.height());
            px.fill(Qt::transparent);
            QPainter p(&px);
            qApp->style()->drawPrimitive(QStyle::PE_IndicatorArrowRight, &opt, &p, widget);
            return px;
        }
    }
    else if (qobject_cast<QLineEdit*>(widget))
    {
        iconView = false;

        class QtnHackLineEdit : public QLineEdit { friend class Qtitan::DrawHelpers; };
        QStyleOptionFrameV2 opt;
        ((QtnHackLineEdit*)widget)->initStyleOption(&opt);
        if ( opt.lineWidth <= 0 )
            opt.lineWidth = 1;

        opt.rect = QRect(QPoint(0, 0), QSize(width, height));

        QPixmap px(opt.rect.width(), opt.rect.height());
        px.fill(Qt::transparent);
        QPainter p(&px);
        qApp->style()->drawPrimitive(QStyle::PE_PanelLineEdit, &opt, &p, widget);

        QRect rectCaret = opt.rect;
        rectCaret.adjust(3, 2, -3, -2);
        rectCaret.setRight(rectCaret.left() + 4);

        QPen oldPen = p.pen();
        p.setPen(Qt::black);

        QPainterPath path;
        path.moveTo(rectCaret.left(), rectCaret.top());
        path.lineTo(rectCaret.right(), rectCaret.top());

        path.moveTo(rectCaret.center().x(), rectCaret.top());
        path.lineTo(rectCaret.center().x(), rectCaret.bottom());

        path.moveTo(rectCaret.left(), rectCaret.bottom());
        path.lineTo(rectCaret.right(), rectCaret.bottom());

        p.drawPath(path);
        p.setPen(oldPen);

        return px;
    }
    else if (ExWidgetWrapper* widgetWrapper = qobject_cast<ExWidgetWrapper*>(widget))
    {
        return createIconExStyleWidget(widgetWrapper->buddy(), width, height, iconView);
    }
    else if (QSpinBox* wd = qobject_cast<QSpinBox*>(widget))
    {
        class QtnHackSpinBox : public QSpinBox { friend class Qtitan::DrawHelpers; };
        return createIconExStyleWidget(((QtnHackSpinBox*)wd)->lineEdit(), width, height, iconView);
    }
    else if (QComboBox* cb = qobject_cast<QComboBox*>(widget))
    {
        iconView = false;

        class QtnHackComboBox : public QComboBox { friend class Qtitan::DrawHelpers; };
        QStyleOptionComboBox opt;
        ((QtnHackComboBox*)cb)->initStyleOption(&opt);

        opt.rect = QRect(QPoint(0, 0), QSize(width, height));

        QPixmap px(opt.rect.width(), opt.rect.height());
        px.fill(Qt::transparent);
        QPainter p(&px);
        qApp->style()->drawComplexControl(QStyle::CC_ComboBox, &opt, &p, widget);

        return px;
    }

    return QPixmap();
}


/*!
    \class Qtitan::StyleOptionFrame
    \internal
*/
StyleOptionFrame::StyleOptionFrame()
{ 
    hdc = 0;
    frameBorder = 4; 
    titleBarSize = 30; 
    statusHeight = 0; 
    active = false; 
    hasStatusBar = false; 
    titleVisible = true; 
    maximizeFlags = false;
    isBackstageVisible = false;
}

StyleOptionFrame::StyleOptionFrame(const StyleOptionFrame& other) 
: QStyleOption(Version, Type) 
{ 
    *this = other; 
}

StyleOptionFrame::~StyleOptionFrame() 
{
}

/*!
    \class Qtitan::StyleOptionBackstageButton
    \internal
*/
StyleOptionBackstageButton::StyleOptionBackstageButton()
{
    tabStyle = false;
    flatStyle = false;
}

StyleOptionBackstageButton::StyleOptionBackstageButton(const StyleOptionBackstageButton& other)
    : QStyleOptionToolButton(other)
{
    *this = other; 
}

/*!
    \class Qtitan::StyleOptionRibbonBackstage
    \internal
*/
StyleOptionRibbonBackstage::StyleOptionRibbonBackstage()
{
    menuWidth = 132;
}

StyleOptionRibbonBackstage::StyleOptionRibbonBackstage(const StyleOptionRibbonBackstage& other)
    : QStyleOption(Version, Type) 
{
    *this = other; 
}


/*!
    \class Qtitan::StyleOptionRibbon
    \internal
*/
StyleOptionRibbon::StyleOptionRibbon()
{ 
    frameHelper = Q_NULL; 
    titleBarHeight = 0;
    minimized = false;
    isBackstageVisible = false;
}

StyleOptionRibbon::StyleOptionRibbon(const StyleOptionRibbon& other) 
    : QStyleOption(Version, Type) 
{ 
    *this = other; 
}

StyleOptionRibbon::~StyleOptionRibbon() 
{
}


/*!
    \class Qtitan::StyleOptionTitleBar
    \internal
*/
StyleOptionTitleBar::StyleOptionTitleBar()
{
    frameHelper = Q_NULL;
    drawIcon = true;
    quickAccessVisibleCount = 0;
    quickAccessOnTop = true;
    quickAccessVisible = true;
    existSysButton = true;
    normalSysButton = false;
}

StyleOptionTitleBar::StyleOptionTitleBar(const StyleOptionTitleBar &other) 
    : QStyleOptionTitleBar(other) 
{ 
    *this = other; 
}

/*!
    \class Qtitan::StyleCaptionButton
    \internal
*/
StyleCaptionButton::StyleCaptionButton()
{
}

StyleCaptionButton::StyleCaptionButton(const StyleCaptionButton& other)
    : QStyleOptionToolButton(other)
{
    *this = other; 
}


/*!
    \class Qtitan::StyleOptionTitleBar
    \internal
*/
StyleSystemToolButton::StyleSystemToolButton()
{
    colorBackground = QColor(QColor::Invalid);
}

StyleSystemToolButton::StyleSystemToolButton(const StyleSystemToolButton& other)
    : QStyleOptionToolButton(other)
{
    *this = other; 
}


/*!
    \class Qtitan::StyleHintReturnThemeColor
    \internal
*/
StyleHintReturnThemeColor::StyleHintReturnThemeColor(const QString& secName, const QString& kName, QColor col)
    : QStyleHintReturn()
{
    sectionName = secName;
    keyName = kName;
    color = col;
    isValid = true;
}

StyleHintReturnThemeColor::~StyleHintReturnThemeColor()
{
}

/*!
    \class Qtitan::StyleRibbonQuickAccessBar
    \internal
*/
StyleRibbonQuickAccessBar::StyleRibbonQuickAccessBar()
{
    quickAccessBarPosition = RibbonBar::QATopPosition;
}

StyleRibbonQuickAccessBar::StyleRibbonQuickAccessBar(const StyleRibbonQuickAccessBar& other) 
    : QStyleOption(other) 
{ 
    *this = other; 
}


