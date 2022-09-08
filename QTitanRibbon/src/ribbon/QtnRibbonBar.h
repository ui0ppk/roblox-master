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
#ifndef QTN_RIBBONBAR_H
#define QTN_RIBBONBAR_H

#include <QMenuBar>

#include "QtitanDef.h"


class QMenu;
class QToolButton;

namespace Qtitan
{
    class RibbonPage;
    class RibbonBarPrivate;
    class RibbonSystemButton;
    class RibbonQuickAccessBar;
#ifdef CUSTOMIZE_V4_WORK
    class RibbonCustomizationManager;
#endif // CUSTOMIZE_V4_WORK

    /* RibbonBar */
    class QTITAN_EXPORT RibbonBar : public QMenuBar
    {
        Q_OBJECT
        Q_ENUMS(QuickAccessBarPosition)
        Q_PROPERTY(QuickAccessBarPosition quickAccessBarPosition READ quickAccessBarPosition WRITE setQuickAccessBarPosition)
        Q_PROPERTY(int currentIndexPage READ currentIndexPage WRITE setCurrentPage NOTIFY currentPageChanged)
        Q_PROPERTY(bool frameThemeEnabled READ isFrameThemeEnabled WRITE setFrameThemeEnabled)
        Q_PROPERTY(bool minimized READ isMinimized WRITE setMinimized NOTIFY minimizationChanged)
        Q_PROPERTY(bool maximized READ isMaximized WRITE setMaximized NOTIFY minimizationChanged)
    public:
        enum QuickAccessBarPosition
        {
            QATopPosition,
            QABottomPosition
        };
    public:
        RibbonBar(QWidget* parent = Q_NULL);
        virtual ~RibbonBar();

    public:
        bool isVisible() const;

    public:
        RibbonPage* addPage(const QString& text);
        void addPage(RibbonPage* page);

        RibbonPage* insertPage(int index, const QString& text);
        void insertPage(int index, RibbonPage* page);

        void movePage(RibbonPage* page, int newIndex);
        void movePage(int index, int newIndex);

        void removePage(RibbonPage* page);
        void removePage(int index);

        void detachPage(RibbonPage* page);
        void detachPage(int index);

        void clearPages();

        bool isKeyTipsShowing() const;
        bool keyTipsEnabled() const;
        void setKeyTipsEnable(bool enable);
        bool isKeyTipsComplement() const;
        void setKeyTipsComplement(bool complement);
        void setKeyTip(QAction* action, const QString& keyTip);

        void setLogoPixmap(Qt::AlignmentFlag alignment, QPixmap& logotype);
        const QPixmap& logoPixmap(Qt::AlignmentFlag& alignment) const;

        void setTitleBackground(const QPixmap& px);
        const QPixmap& titleBackground() const;

        RibbonQuickAccessBar* getQuickAccessBar() const;

        void showQuickAccess(bool show = true);
        bool isQuickAccessVisible() const;

        void setQuickAccessBarPosition(QuickAccessBarPosition position);
        QuickAccessBarPosition quickAccessBarPosition() const;

        void minimize();
        bool isMinimized() const;
        void setMinimized(bool flag);
        
        void maximize();
        bool isMaximized() const;
        void setMaximized(bool flag);

        void setMinimizationEnabled(bool enabled);
        bool isMinimizationEnabled() const;

        void setAllowPopups(bool allow);
        bool allowPopups() const;

		int availableWidth() const;

#ifdef CUSTOMIZE_V4_WORK
        RibbonCustomizationManager* customizeManager() const;
#endif // CUSTOMIZE_V4_WORK

    public Q_SLOTS:
        void setCurrentPage(int index);

    public:
        int currentIndexPage() const;
        RibbonPage* getPage(int index) const;
        int getPageCount() const;

    public:
        QMenu* addMenu(const QString& text);
        QAction* addAction(const QIcon& icon, const QString& text, Qt::ToolButtonStyle style, QMenu* menu = Q_NULL);
        QAction* addAction(QAction* action, Qt::ToolButtonStyle style);

        QAction* addSystemButton(const QString& text);
        QAction* addSystemButton(const QIcon& icon, const QString& text);

        RibbonSystemButton* getSystemButton() const;

        bool isBackstageVisible() const;

        void setFrameThemeEnabled(bool enable = true);
        bool isFrameThemeEnabled() const;

        bool isTitleBarVisible() const;
        void setTitleBarVisible(bool show);

        int titleBarHeight() const;
        int topBorder() const;

    Q_SIGNALS:
        void minimizationChanged(bool minimized);
        void currentPageChanged(int index);
        void currentPageChanged(RibbonPage* page);
        void keyTipsShowed(bool showed);
        void showRibbonContextMenu(QMenu* menu, bool& showMenu);
        void ribbonCustomize(bool& userDlg);

    protected:
        int backstageViewTop() const;
        int tabBarHeight(bool isVisible = false) const;
        void setMinimizedFlag(bool flag);

    public:
        virtual int heightForWidth(int) const;
        virtual QSize sizeHint() const;

    protected:
        virtual bool event(QEvent* event);
        virtual bool eventFilter(QObject* object, QEvent* event);
        virtual void paintEvent(QPaintEvent* p);
        virtual void changeEvent(QEvent* event);
        virtual void resizeEvent(QResizeEvent* event);

        virtual void mouseDoubleClickEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);
        virtual void wheelEvent(QWheelEvent* event);
#ifdef CUSTOMIZE_V4_WORK
        virtual void contextMenuEvent(QContextMenuEvent* event);
#endif // CUSTOMIZE_V4_WORK

#ifdef Q_OS_WIN
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        virtual bool nativeEvent(const QByteArray& eventType, void* message, long* result);
#else
        virtual bool winEvent(MSG* message, long* result);
#endif
#endif // Q_OS_WIN
    private:
        friend class RibbonPagePrivate;
        friend class RibbonTabBarPrivate;
        friend class RibbonSystemPopupBar;
        friend class RibbonBackstageViewPrivate;
        friend class RibbonBackstageView;
        friend class RibbonStyle;
        #ifdef Q_OS_WIN
        friend class OfficeFrameHelperWin;
        #endif // Q_OS_WIN
        QTN_DECLARE_PRIVATE(RibbonBar)
        Q_DISABLE_COPY(RibbonBar)
    };

}; //namespace Qtitan


#endif // QTN_RIBBONBAR_H


