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
#ifndef QTN_RIBBONSTYLEPRIVATE_H
#define QTN_RIBBONSTYLEPRIVATE_H

#include "QtnOfficeStylePrivate.h"
#include "QtnRibbonStyle.h"

/* IRibbonPaintManager */
class IRibbonPaintManager
{
public:
    virtual ~IRibbonPaintManager() {}

public:
    // for RibbonBar
    virtual void drawRibbonBar(const QStyleOption* option, QPainter* p, const QWidget* widget) const = 0;
    virtual void drawRibbonGroups(const QStyleOption* option, QPainter* p, const QWidget* widget) const = 0;
    virtual void drawGroup(const QStyleOption* option, QPainter* p, const QWidget* widget) const = 0;
    virtual void drawReducedGroup(const QStyleOption* option, QPainter* p, const QWidget* widget) const = 0;
    // for TitleBar
    virtual bool drawToolBar(const QStyleOption*, QPainter*, const QWidget*) const = 0;
    virtual bool drawIndicatorToolBarSeparator(const QStyleOption*, QPainter*, const QWidget*) const = 0;
    virtual bool drawPanelButtonTool(const QStyleOption*, QPainter*, const QWidget*) const = 0; 
    virtual void drawContextHeaders(const QStyleOption* opt, QPainter* p) const = 0;

    // for ribbon tab
    virtual void drawTabShape(const QStyleOption* opt, QPainter* p, const QWidget* widget) const = 0;
    virtual void drawTabShapeLabel(const QStyleOption* opt, QPainter* p, const QWidget* widget) const = 0;

    // for TabBar
    virtual bool drawTabBarTabShape(const QStyleOption*, QPainter*, const QWidget*) const = 0;
    // for QForm
    virtual bool drawFrame(const QStyleOption*, QPainter*, const QWidget*) const = 0;
    virtual bool drawShapedFrame(const QStyleOption*, QPainter*, const QWidget*) const = 0;
    // for Slider
    virtual bool drawSlider(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const = 0;

    // for menuBar
    virtual bool drawFrameMenu(const QStyleOption* opt, QPainter* p, const QWidget* w) const = 0;

    // for file menu
    virtual void drawSystemButton(const QStyleOption* option, QPainter* p, const QWidget* widget) const = 0;
	virtual void drawQuickAccessButton(const QStyleOption* option, QPainter* p, const QWidget* widget) const = 0;
    virtual void drawOptionButton(const QStyleOption* option, QPainter* p, const QWidget* widget) const = 0;
    virtual void drawPopupResizeGripper(const QStyleOption* option, QPainter* p, const QWidget* widget) const = 0;

    // for stausBar
    virtual bool drawPanelStatusBar(const QStyleOption*, QPainter*, const QWidget*) const = 0;
    // for SizeGrip
    virtual bool drawSizeGrip(const QStyleOption*, QPainter*, const QWidget*) const = 0; 

    virtual bool drawIndicatorArrow(QStyle::PrimitiveElement, const QStyleOption*, QPainter*, const QWidget*) const = 0;

    virtual void drawRectangle(QPainter* p, const QRect& rect, bool selected, bool pressed, bool enabled, bool checked, bool popuped,
        Qtitan::BarType barType, Qtitan::BarPosition barPos) const = 0;

    virtual void drawKeyTip(const QStyleOption* opt, QPainter* p, const QWidget* widget) const = 0;
    virtual void drawBackstage(const QStyleOption* opt, QPainter* p, const QWidget* widget) const = 0;
    virtual void drawRibbonBackstageMenu(const QStyleOption* opt, QPainter* p, const QWidget* widget) const = 0;
    virtual void drawRibbonBackstageCloseButton(const QStyleOption* opt, QPainter* p, const QWidget* widget) const = 0;
    virtual bool drawRibbonBackstageSeparator(const QStyleOption* opt, QPainter* p, const QWidget* widget) const = 0;

    virtual void setupPalette(QWidget* widget) const = 0;

public:
    virtual void drawRibbonSliderButton(const QStyleOption* opt, QPainter* p, const QWidget* widget) const = 0;
    virtual bool isTopLevelMenuItemUpperCase(const QWidget*) const  = 0;
    virtual bool isRibbonBackstageHideTabs() const = 0;
};

Q_DECLARE_INTERFACE(IRibbonPaintManager, "qtn-project.Qtn.IRibbonPaintManager")

namespace Qtitan
{
    class RibbonPaintManager;
    /* RibbonPagePrivate */
    class RibbonStylePrivate : public OfficeStylePrivate
    {
    public:
        QTN_DECLARE_EX_PUBLIC(RibbonStyle)
    public:
        explicit RibbonStylePrivate();

    public:
        void initialization();
        void refreshMetrics(QWidget* widget);
        void updateColors();

        void tabLayout(const QStyleOptionTabV3* opt, const QWidget* widget, QRect* textRect, QRect* iconRect) const;
        void tabHoverUpdate(QTabBar* tabBar, QEvent* event);

        void setMacSmallSize(QWidget* widget);
        void unsetMacSmallSize(QWidget* widget);

        IRibbonPaintManager* ribbonPaintManager() const;
        virtual void makePaintManager();

    public:
        QColor       m_clrRibbonGroupCaptionText;
        // [Ribbon]
        QColor       m_clrRibbonFace;
        QColor       m_clrRibbonLight;
        QColor       m_clrRibbonText;
        QColor       m_clrGroupClientText;
        QColor       m_clrGroupClientGrayText;
        QColor       m_clrRibbonInactiveFace;
        QColor       m_clrRibbonSeparator;
        QColor       m_clrRibbonGrayText;
        QColor       m_clrFileButtonText;
        QColor       m_clrMinimizedFrameEdgeHighLight;
        QColor       m_clrMinimizedFrameEdgeShadow;

        int m_baseHeight;
        int m_tabsHeight;
        int m_rowHeight;
        int m_fileButtonImageCount;
        bool m_flatFrame;
        bool m_destroyKeyTips;
        bool m_completeKey;
        bool m_blockKeyTips;
        bool m_dpiAware;
        QRect m_oldHoverRectTab;
        QFont m_fontRegular;  // regular font.
        QHash<const QWidget*, bool> m_macSmallSizeWidgets;
        static double logPixel;

    private:
        Q_DISABLE_COPY(RibbonStylePrivate)
    };


    /* RibbonPaintManager */
    class RibbonPaintManager : public OfficePaintManager,
                               public IRibbonPaintManager 
    {
        Q_OBJECT
        Q_INTERFACES(IRibbonPaintManager)
    public:
        explicit RibbonPaintManager(CommonStyle* baseStyle);
        virtual ~RibbonPaintManager();

    public:
        // for RibbonBar
        virtual void drawRibbonBar(const QStyleOption* option, QPainter* p, const QWidget* widget) const;
        virtual void drawRibbonGroups(const QStyleOption* option, QPainter* p, const QWidget* widget) const;
        virtual void drawGroup(const QStyleOption* option, QPainter* p, const QWidget* widget) const;
        virtual void drawReducedGroup(const QStyleOption* option, QPainter* p, const QWidget* widget) const;

        // for TitleBar
        virtual bool drawToolBar(const QStyleOption*, QPainter*, const QWidget*) const;
        virtual bool drawIndicatorToolBarSeparator(const QStyleOption*, QPainter*, const QWidget*) const;
        virtual bool drawPanelButtonTool(const QStyleOption*, QPainter*, const QWidget*) const; 
        virtual void drawContextHeaders(const QStyleOption* opt, QPainter* p) const;

        // for ribbon tab
        virtual void drawTabShape(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
        virtual void drawTabShapeLabel(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;

        // for TabBar
        virtual bool drawTabBarTabShape(const QStyleOption*, QPainter*, const QWidget*) const;
        // for QForm
        virtual bool drawFrame(const QStyleOption*, QPainter*, const QWidget*) const;
        virtual bool drawShapedFrame(const QStyleOption*, QPainter*, const QWidget*) const;
        // for Slider
        virtual bool drawSlider(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const;
        // for menuBar
        virtual bool drawFrameMenu(const QStyleOption* opt, QPainter* p, const QWidget* w) const;

        // for file menu
        virtual void drawSystemButton(const QStyleOption* option, QPainter* p, const QWidget* widget) const;
		virtual void drawQuickAccessButton(const QStyleOption* option, QPainter* p, const QWidget* widget) const;
        virtual void drawOptionButton(const QStyleOption* option, QPainter* p, const QWidget* widget) const;
        virtual void drawPopupResizeGripper(const QStyleOption* option, QPainter* p, const QWidget* widget) const;

        // for stausBar
        virtual bool drawPanelStatusBar(const QStyleOption*, QPainter*, const QWidget*) const;
        // for SizeGrip
        virtual bool drawSizeGrip(const QStyleOption*, QPainter*, const QWidget*) const; 

        virtual bool drawIndicatorArrow(QStyle::PrimitiveElement, const QStyleOption*, QPainter*, const QWidget*) const;

        virtual void drawRectangle(QPainter* p, const QRect& rect, bool selected, bool pressed, bool enabled, bool checked, bool popuped,
            Qtitan::BarType barType, Qtitan::BarPosition barPos) const;

        virtual void drawKeyTip(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
        virtual void drawBackstage(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
        virtual void drawRibbonBackstageMenu(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
        virtual void drawRibbonBackstageCloseButton(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
        virtual bool drawRibbonBackstageSeparator(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;

        virtual void setupPalette(QWidget* widget) const;

    public:
        virtual void drawRibbonSliderButton(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
        virtual bool isTopLevelMenuItemUpperCase(const QWidget*) const { return false; }
        virtual bool isRibbonBackstageHideTabs() const { return false; }

    protected:
        void fillRibbonTabControl(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;

        // for BackstageButton
        void drawPanelBackstageButton(const QStyleOption*, QPainter*, const QWidget*) const; 
        QString pathForBackstageImages() const;

    private:
        Q_DISABLE_COPY(RibbonPaintManager)
    };

    /* RibbonPaintManager */
    class RibbonPaintManager2013 : public OfficePaintManager2013,
                                   public IRibbonPaintManager  
    {
        Q_OBJECT
        Q_INTERFACES(IRibbonPaintManager)
    public:
        explicit RibbonPaintManager2013(CommonStyle* baseStyle);
        virtual ~RibbonPaintManager2013();

    public:
        // for RibbonBar
        virtual void drawRibbonBar(const QStyleOption* option, QPainter* p, const QWidget* widget) const;
        virtual void drawRibbonGroups(const QStyleOption* option, QPainter* p, const QWidget* widget) const;
        virtual void drawGroup(const QStyleOption* option, QPainter* p, const QWidget* widget) const;
        virtual void drawReducedGroup(const QStyleOption* option, QPainter* p, const QWidget* widget) const;
        // for TitleBar
        virtual bool drawToolBar(const QStyleOption*, QPainter*, const QWidget*) const;
        virtual bool drawIndicatorToolBarSeparator(const QStyleOption*, QPainter*, const QWidget*) const;
        virtual bool drawPanelButtonTool(const QStyleOption*, QPainter*, const QWidget*) const;
        virtual void drawContextHeaders(const QStyleOption* opt, QPainter* p) const;
        // for ribbon tab
        virtual void drawTabShape(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
        virtual void drawTabShapeLabel(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
        // for TabBar
        virtual bool drawTabBarTabShape(const QStyleOption*, QPainter*, const QWidget*) const;
        // for QForm
        virtual bool drawFrame(const QStyleOption*, QPainter*, const QWidget*) const;
        virtual bool drawShapedFrame(const QStyleOption*, QPainter*, const QWidget*) const;
        // for Slider
        virtual bool drawSlider(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const;
        // for menuBar
        virtual bool drawFrameMenu(const QStyleOption* opt, QPainter* p, const QWidget* w) const;

        // for file menu
        virtual void drawSystemButton(const QStyleOption* option, QPainter* p, const QWidget* widget) const;
		virtual void drawQuickAccessButton(const QStyleOption* option, QPainter* p, const QWidget* widget) const;
        virtual void drawOptionButton(const QStyleOption* option, QPainter* p, const QWidget* widget) const;
        virtual void drawPopupResizeGripper(const QStyleOption* option, QPainter* p, const QWidget* widget) const;

        // for stausBar
        virtual bool drawPanelStatusBar(const QStyleOption*, QPainter*, const QWidget*) const;
        // for SizeGrip
        virtual bool drawSizeGrip(const QStyleOption*, QPainter*, const QWidget*) const; 

        virtual bool drawIndicatorArrow(QStyle::PrimitiveElement, const QStyleOption*, QPainter*, const QWidget*) const;

        virtual void drawRectangle(QPainter* p, const QRect& rect, bool selected, bool pressed, bool enabled, bool checked, bool popuped,
            Qtitan::BarType barType, Qtitan::BarPosition barPos) const;

        virtual void drawKeyTip(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
        virtual void drawBackstage(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
        virtual void drawRibbonBackstageMenu(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
        virtual void drawRibbonBackstageCloseButton(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
        virtual bool drawRibbonBackstageSeparator(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;

        virtual void setupPalette(QWidget* widget) const;

    public:
        virtual void modifyColors();

    public:
        virtual void drawRibbonSliderButton(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
        virtual bool isTopLevelMenuItemUpperCase(const QWidget* w) const;
        virtual bool isRibbonBackstageHideTabs() const { return true; }

        void fillRibbonButton(const QStyleOption* option, QPainter* p) const;
        void fillRibbonTabControl(const QStyleOption* opt, QPainter* p, const QWidget* widget) const;
    protected:
        // for BackstageButton
        void drawPanelBackstageButton(const QStyleOption*, QPainter*, const QWidget*) const; 
    protected:
        QColor m_clrRibbonGroupsFill;

    private:
        Q_DISABLE_COPY(RibbonPaintManager2013)
    };

    template<class T>
    const T* getParentWidget(const QWidget* widget)
    {
        while (widget)
        {
            if (const T* parent = qobject_cast<const T*>(widget))
                return parent;
            widget = widget->parentWidget();
        }
        return Q_NULL;
    }

}; //namespace Qtitan


#endif // QTN_RIBBONSTYLEPRIVATE_H
