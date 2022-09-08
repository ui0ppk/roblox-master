/**
 * QtUtilities.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QStyle>
#include <QDialog>
#include <QMutex>
#include <QMessageBox>
#include <QPointer>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QProcess>

// 3rd Party Headers
#include "boost/function.hpp"
#include "boost/thread.hpp"
#include "boost/scoped_ptr.hpp"

// Roblox Headers
#include "g3d/Color3.h"

class QPixmap;
class QEventLoop;
class QProgressBar;
class QLabel;
class QDialogButtonBox;
class QPushButton;
class QStackedWidget;
class QPropertyAnimation;

namespace QtUtilities
{
#ifdef Q_WS_MAC
	const int BranchIndicatorSize = 18;
#else
	const int BranchIndicatorSize = 14;
#endif

	QPixmap getBranchIndicator(QStyle::State iconState);
	QPixmap getBreakpointPixmap(int breakpointState, int size);

	QPixmap getPixmap(const QString& resourceName, int index, int iconSize = 16, bool bMask = false);

	void updateWindowTitle(QWidget* pWidget);
	
	QString getMacOSXVersion();

    QColor toQColor(const G3D::Color3& color);

	int toInt(Q_PID pId);

    /**
     * Given a resource file name, load the text of the resource.
     *  The resource data must be in text.
     *  
     * @param   resourceName    name of resource in the form ":/path/file.ext"
     * @return  contents of resource file as a text string
     */
    QString getResourceFileText(const QString& resourceName);

    QByteArray getResourceFileBytes(const QString& resourceName);

    /**
     * Sets the shortcut keys and adds key sequences to tooltip.
     */
    void setActionShortcuts(QAction& action,const QList<QKeySequence> shortcuts);

    /**
     * Inserts newlines to attempt to wrap text to the given width.
     * 
     * @param   text    input text
     * @param   width   column width of new text
     * @return  text with newlines inserted to attempt to make no long wider than the width
     */
    QString wrapText(const QString& text,int width);
	
	static void sleep(int ms)
	{
		#ifdef _WIN32
			::Sleep(ms);
		#else
			::usleep(1000 * ms);
		#endif
	}

	class ProgressDialog: public QDialog
	{
		Q_OBJECT
	public:
		ProgressDialog(QWidget *pParent, QWidget* lastFocusWidget = 0);
		virtual ~ProgressDialog();
		
		void setLabel(const QString &text);
		void executeInNewThread(boost::function<void()> userWorkFunc);
	
	private Q_SLOTS:
		void showDialog();
		
	private:
		/*override*/ void closeEvent(QCloseEvent *evt);
		/*override*/ void keyPressEvent(QKeyEvent *evt);
		
		void execute(boost::function<void()> userWorkFunc);
		
		boost::scoped_ptr<boost::thread>    m_workerThread;
		QEventLoop*                         m_eventLoop;
		
		QProgressBar*                       m_pProgressBar;
		QLabel*                             m_pLabel;

		QPointer<QWidget>				    m_lastFocusWidget;
		
		QTimer*                             m_pForceTimer;
		
		QMutex                              m_progressMutex;
		volatile bool                       m_bExecutionComplete;
	};

	class RBXMessageBox : public QMessageBox
	{
	public:
		RBXMessageBox(QWidget* parent = 0);
		RBXMessageBox(QMessageBox::Icon icon, const QString & title, const QString & text, 
			QMessageBox::StandardButtons buttons = NoButton, QWidget *parent = 0, 
			Qt::WindowFlags f = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint );
	};

	class RBXConfirmationMessageBox : public QDialog
	{
		Q_OBJECT
	private:
		QLabel *label;
		QDialogButtonBox* buttonBox;
		QCheckBox* dontAskCheckbox;
		QPushButton* yesButton;
		QPushButton* noButton;
		bool clickedYes;
	public:
		RBXConfirmationMessageBox(QString question, QString yesButtonName = QObject::tr("Yes"), QString noButtonName = QObject::tr("No"),  QWidget* parent = 0);
		bool getClickedYes() { return clickedYes; }
		bool getCheckBoxState() { return dontAskCheckbox->isChecked(); }
	public Q_SLOTS:
		void markYes();
		void markNo();
	};

    inline QString qstringFromStdString( const std::string& s )
    {
        return QString::fromStdString(s);
    }

    inline QString qstringFromStdString( const std::wstring& str )
    {
#ifdef _MSC_VER
            return QString::fromUtf16((const ushort *)str.c_str());
#else
            return QString::fromStdWString(str);
#endif
    }

	class StackedWidgetAnimator : public QObject
	{
		Q_OBJECT
	public:
		StackedWidgetAnimator(QStackedWidget* pStackedWidget);
		void setCurrentIndex(int index);

	private Q_SLOTS:
		void onAnimationFinished();

	private:
		void moveIndex(int fromIndex, int toIndex);
		QPropertyAnimation* getAnimation(QWidget* pWidget, const QPoint& startVal, const QPoint& endVal);

		QStackedWidget*        m_pStackedWidget;
		QPoint                 m_fromWidgetPos;

		int                    m_PendingRequestedIndex;

		int                    m_FromIndex;
		int                    m_ToIndex;

		bool                   m_bAnimInProgress;
	};
}
