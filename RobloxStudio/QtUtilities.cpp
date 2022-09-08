/**
 * QtUtilities.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "QtUtilities.h"

// Qt Headers
#include <QBitmap>
#include <QPainter>
#include <QLabel>
#include <QProgressBar>
#include <QCloseEvent>
#include <QPixmapCache>
#include <QResource>
#include <QString>
#include <QPushButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QAbstractButton>
#include <QPushButton>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

// Roblox Studio Headers
#include "RobloxMainWindow.h"
#include "UpdateUIManager.h"

namespace QtUtilities
{
    /**
     * Gets a pixmap from a sprite sheet.
     * 
     * @param   resourceName    name of qrc resource for image
     * @param   index           index of sprite
     * @param   iconSize        width/height of sprite
     * @param   bMask           true to return a mask of the sprite
     */
	QPixmap getPixmap(const QString& resourceName, int index, int iconSize, bool bMask)
	{
        static QMap< QString, QList<QPixmap> > sPixmaps;
		QList<QPixmap> pixmapList;

		if(sPixmaps.contains(resourceName))
			pixmapList = sPixmaps[resourceName];

		if(pixmapList.isEmpty())
		{
			QPixmap completePixmap(resourceName);
			for (int count=0; iconSize * count < completePixmap.width(); count++) 
				pixmapList.append(completePixmap.copy(iconSize*count, 0, iconSize, iconSize));

			if(completePixmap.width())
				sPixmaps[resourceName] = pixmapList;
		}

		QPixmap pixmap;
		if(index >= 0 || index < pixmapList.size())
		{
			pixmap = pixmapList[index];
			if(bMask)
			{
				QImage pixImg = pixmap.toImage();
				QColor maskColor(pixImg.pixel(0,0));
				pixmap.setMask(pixmap.createMaskFromColor(maskColor, Qt::MaskInColor));
			}
		}

		return pixmap;
	}

    /**
     * Gets a pixmap used by Qt to draw a tree control's branch indicator.
     *  Caches image so this function is very fast for multiple calls.
     */
	QPixmap getBranchIndicator(QStyle::State iconState)
	{
        QPixmap pix;
        if ( !QPixmapCache::find(QString::number(iconState),&pix) )
        {
			const int BranchIndicatorBorder = 2;
			pix = QPixmap(BranchIndicatorSize,BranchIndicatorSize);
		    pix.fill(Qt::transparent);

		    QStyleOptionViewItemV4 branchOption;
		    branchOption.rect = QRect(
                BranchIndicatorBorder,
                BranchIndicatorBorder,
                BranchIndicatorSize / 2 + BranchIndicatorBorder,
                BranchIndicatorSize / 2 + BranchIndicatorBorder );
		    branchOption.state = iconState;

		    QPainter p(&pix);
		    QApplication::style()->drawPrimitive(QStyle::PE_IndicatorBranch,&branchOption,&p);
            QPixmapCache::insert(QString::number(iconState),pix);
        }

		return pix;
	}

	QPixmap getBreakpointPixmap(int breakpointState, int size)
	{
		QPixmap pix;
		if ( !QPixmapCache::find(QString("BreakpointState%1").arg(breakpointState),&pix) )
        {
			int scaledSize = QtUtilities::BranchIndicatorSize*6;
			pix = QPixmap(scaledSize, scaledSize);
			pix.fill(Qt::transparent);

			QPoint center(scaledSize/2, scaledSize/2);

			QPainter painter;
			painter.begin(&pix);
			QColor outerClr(186,46,73);
			if (breakpointState == 1)
			{
				QColor innerClr(210,72,100);
				QColor lightClr(230,153,168);

				QPoint focalPt(0.4*scaledSize/2, 0.4*scaledSize/2);
				QRadialGradient gradient(center.x(), center.y(), (scaledSize/2), focalPt.x(), focalPt.y());
				gradient.setColorAt(0.4, lightClr); 
				gradient.setColorAt(0.8, innerClr); 
				gradient.setColorAt(1, outerClr); 
				painter.setBrush(gradient);
				painter.setPen(Qt::NoPen);
			}
			else
			{
				QPen newPen(outerClr);
				newPen.setWidth(6);
				painter.setPen(newPen);
			}

			painter.drawEllipse(center, scaledSize/2, scaledSize/2);
			painter.end();

			pix = pix.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			QPixmapCache::insert(QString("BreakpointState%1").arg(breakpointState), pix);
		}

		return pix;
	}
	
	QString getMacOSXVersion()
	{
		QString strMacVersion = "";
#ifdef Q_WS_MAC
		QSysInfo::MacVersion macVersion = QSysInfo::MacintoshVersion;
		switch (macVersion) 
		{
			case QSysInfo::MV_LEOPARD:
				strMacVersion = "Mac OS X 10.5";
				break;
			case QSysInfo::MV_SNOWLEOPARD:
				strMacVersion = "Mac OS X 10.6";
				break;
			default:
				// Roblox Studio is not allowed to Run on 10.4 or below
				// So the OSX is probably 10.7 or higher
				// We cannot query for Lion/Mountain Lion as Qt 4.7 did not support Lion/Mountain Lion
				// We need to move upto Qt 4.8 to support Lion
				// Qt 5 which is in Beta will officially supports Mountain Lion
				strMacVersion = "Mac OS X 10.7 or Higher";
		}
#endif
		return strMacVersion;
		
	}

    /**
     * Converts a G3D::Color3 to a QColor.
     */
    QColor toQColor(const G3D::Color3& color)
    {
        return QColor(
            color.r * 255,
            color.g * 255,
            color.b * 255 );
    }

	int toInt(Q_PID pId)
	{
		int processId = 0;
#ifdef Q_WS_WIN32
			if (pId)
				processId = pId->dwProcessId;
#else
			processId = pId;
#endif
		return processId;
	}

    QByteArray getResourceFileBytes(const QString& resourceName)
    {
        const QResource resource(resourceName);
        RBXASSERT(resource.data());
        RBXASSERT(resource.size() > 0);

        QByteArray data((const char*)resource.data(),(int)resource.size());

        // uncompress if necessary
        if ( resource.isCompressed() )
            data = qUncompress(data);

        return data;
    }

    /**
     * Given a resource file name, load the text of the resource.
     *  The resource data must be in text.
     *  
     * @param   resourceName    name of resource in the form ":/path/file.ext"
     * @return  contents of resource file as a text string
     */
    QString getResourceFileText(const QString& resourceName)
    {
        return getResourceFileBytes(resourceName);
    }

    /**
     * Sets the shortcut keys and adds key sequences to tooltip.
     */
    void setActionShortcuts(QAction& action,const QList<QKeySequence> shortcuts)
    {
        QString tooltip = action.toolTip();

        // set status tip
        if ( action.statusTip().isEmpty() )
            action.setStatusTip(tooltip);

		// do not set action shortcut in case of ribbon bar (it get's added by ribbon bar)
		if (!UpdateUIManager::Instance().getMainWindow().isRibbonStyle())
		{
			// strip off old shortcut if found in tooltip
			// some common commands have tooltips already set from assignAccelerators
			int index = tooltip.lastIndexOf(" (");
			if ( index != -1 )
				tooltip = tooltip.left(index);

			tooltip += " (";
			for ( int i = 0 ; i < shortcuts.size() ; ++i )
			{
				if ( i > 0 )
					tooltip += ", ";
				tooltip += shortcuts[i].toString(QKeySequence::NativeText);
			}
			tooltip += ")";

			action.setToolTip(tooltip);
		}

		action.setShortcuts(shortcuts);
    }

    /**
     * Inserts newlines to attempt to wrap text to the given width.
     * 
     * @param   text    input text
     * @param   width   column width of new text
     * @return  text with newlines inserted to attempt to make no long wider than the width
     */
    QString wrapText(const QString& text,int width)
    {
        QString s = text;
        QString result;
        QRegExp regExp("[\\s\\n,\\.]");

        while ( !s.isEmpty() )
        {
            int index;
            
            if ( s.length() > width )
            {
                // search from width and find the last break
                index = s.lastIndexOf(regExp,width);
                if ( index == -1 )
                {
                    // otherwise try searching forward to the next break
                    index = s.indexOf(regExp,width);
                    if ( index == -1 )
                    {
                        // give up searching
                        index = s.length();
                    }
                }
            }
            else
            {
                result += s;
                break;
            }

            result += s.left(index + 1);
            result += "\n";

            s = s.right(s.length() - index - 1);
        }

        return result;
    }
	
    /*****************************************************************************/
    // ProgressDialog
    /*****************************************************************************/
	
	ProgressDialog::ProgressDialog(QWidget *pParent, QWidget* lastFocusWidget)
	    : QDialog(pParent, Qt::WindowTitleHint)
	    , m_lastFocusWidget(lastFocusWidget)
	    , m_bExecutionComplete(false)
        , m_eventLoop(NULL)
	{
		if(m_lastFocusWidget == NULL)
		{
			m_lastFocusWidget = QApplication::focusWidget();
			if(m_lastFocusWidget && m_lastFocusWidget->focusProxy())
			{
				m_lastFocusWidget = m_lastFocusWidget->focusProxy();
			}
		}

		updateWindowTitle(this);

		setFixedSize(275, 100);
		setWindowModality(Qt::WindowModal);
		
		m_pLabel = new QLabel(this);
		m_pLabel->setAlignment(Qt::AlignCenter);
		
		m_pProgressBar = new QProgressBar(this);
		m_pProgressBar->setRange(0,0);
		m_pProgressBar->setTextVisible(false);
						
		QGridLayout *pGridLayout = new QGridLayout(this);
        pGridLayout->setObjectName(QString::fromUtf8("gridLayout"));
		pGridLayout->setVerticalSpacing(4);
		pGridLayout->setHorizontalSpacing(2);
        pGridLayout->addWidget(m_pLabel, 0, 0, 1, 1);
		pGridLayout->addWidget(m_pProgressBar, 1, 0, 1, 1);
		
#ifdef Q_WS_MAC
		UpdateUIManager::Instance().setMenubarEnabled(false);
#endif

		m_pForceTimer = new QTimer(this);
		QObject::connect(m_pForceTimer, SIGNAL(timeout()), this, SLOT(showDialog()));		
	}
	
	ProgressDialog::~ProgressDialog()
	{
#ifdef Q_WS_MAC
		UpdateUIManager::Instance().setMenubarEnabled(true);
#endif
		if(m_lastFocusWidget)
			m_lastFocusWidget->setFocus();

		close();
	}
	
	void ProgressDialog::setLabel(const QString &text)
	{ m_pLabel->setText(text); }
	
	void ProgressDialog::executeInNewThread(boost::function<void()> userWorkFunc)
	{
        if ( m_eventLoop )
            delete m_eventLoop;
		m_eventLoop = new QEventLoop(this);

		m_workerThread.reset(new boost::thread(boost::bind(&ProgressDialog::execute, this, userWorkFunc)));
		
		m_pForceTimer->start(300);
		
		m_progressMutex.lock();
		bool execDone = m_bExecutionComplete;
		m_progressMutex.unlock();
		
		if (!execDone)
			m_eventLoop->exec();
	}
	
	void ProgressDialog::execute(boost::function<void()> userWorkFunc)
	{
		try 
		{
			userWorkFunc();
		}
		catch(...)
		{
            // TODO - silent failure??
			//RBXCRASH();
		}
		
		m_progressMutex.lock();
		m_bExecutionComplete = true;
		m_progressMutex.unlock();
		
		m_eventLoop->exit();
	}
	
	void ProgressDialog::showDialog()
	{
		m_pForceTimer->stop();
		show();
	}
	
	void ProgressDialog::closeEvent(QCloseEvent *evt)
	{ evt->ignore(); }
	
	void ProgressDialog::keyPressEvent(QKeyEvent *evt)
	{ 
        if (evt->key() != Qt::Key_Escape) 
            QDialog::keyPressEvent(evt); 
    }

    /*****************************************************************************/
    // RBXMessageBox
    /*****************************************************************************/

	RBXMessageBox::RBXMessageBox(QWidget* parent)
		:QMessageBox(parent ? parent : &UpdateUIManager::Instance().getMainWindow())
	{
		updateWindowTitle(this);
	}

	RBXMessageBox::RBXMessageBox(QMessageBox::Icon icon, const QString & title, const QString & text, 
		QMessageBox::StandardButtons buttons, QWidget *parent, Qt::WindowFlags f)
		:QMessageBox(icon, title, text, buttons, parent, f)
	{
		if(!parentWidget())
		{
			setParent(&UpdateUIManager::Instance().getMainWindow());
		}

		if(title.isNull() || title.isEmpty())
			updateWindowTitle(this);
	}

    /**
     * Updates the window title of a widget based on the widget's parent or the main window.
     */
	void updateWindowTitle(QWidget* pWidget)
	{
		if(!pWidget)
			return;

		QString title = pWidget->windowTitle();
		if( title.isEmpty() )
		{
			QWidget* parent = pWidget->parentWidget();
            RobloxMainWindow& main_window = UpdateUIManager::Instance().getMainWindow();

			if ( parent && parent != &main_window )
				title = parent->windowTitle();

			if ( title.isEmpty() )
				title = main_window.getDialogTitle();

			pWidget->setWindowTitle(title);
		}
	}

	/*****************************************************************************/
    // RBXMessageBox
    /*****************************************************************************/
	RBXConfirmationMessageBox::RBXConfirmationMessageBox(QString question, QString yesButtonName, QString noButtonName,  QWidget* parent)
		:QDialog(parent ? parent : &UpdateUIManager::Instance().getMainWindow())
		,clickedYes(false)
	{
		updateWindowTitle(this);
		label = new QLabel(question);
		dontAskCheckbox = new QCheckBox(tr("Don't ask me again"));

		// if label contains a link then let label open it in an external browser
		if (question.contains("href="))
			label->setOpenExternalLinks(true);

		QHBoxLayout *messageLine = new QHBoxLayout;
		QHBoxLayout *checkboxLine = new QHBoxLayout;
		QHBoxLayout *buttonLine = new QHBoxLayout;
		QVBoxLayout *mainLayout = new QVBoxLayout;

		label->setWordWrap(true);
		buttonBox = new QDialogButtonBox;
		yesButton = new QPushButton(yesButtonName);
		noButton = new QPushButton(noButtonName);
		buttonBox->addButton(yesButton, QDialogButtonBox::ActionRole);
		buttonBox->addButton(noButton, QDialogButtonBox::ActionRole);
		messageLine->addWidget(label);
		checkboxLine->addWidget(dontAskCheckbox);
		buttonLine->addWidget(buttonBox);
		mainLayout->addLayout(messageLine);
		mainLayout->addLayout(checkboxLine);
		mainLayout->addLayout(buttonLine);
		setLayout(mainLayout);
		setModal(true);
		connect(yesButton, SIGNAL(clicked()), this, SLOT(markYes()));
		connect(noButton, SIGNAL(clicked()), this, SLOT(markNo()));
		connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(close()));
	
	}

	void RBXConfirmationMessageBox::markYes()
	{
		clickedYes = true;
	}

	void RBXConfirmationMessageBox::markNo()
	{
		clickedYes = false;
	}

	/*****************************************************************************/
	// StackedWidgetAnimator
	/*****************************************************************************/
	StackedWidgetAnimator::StackedWidgetAnimator(QStackedWidget* pStackedWidget)
	: QObject(pStackedWidget)
	, m_pStackedWidget(pStackedWidget)
	, m_FromIndex(-1)
	, m_PendingRequestedIndex(-1)
	, m_ToIndex(-1)
	, m_bAnimInProgress(false)
	{

	}

	void StackedWidgetAnimator::setCurrentIndex(int index)
	{
		if (m_bAnimInProgress)
		{
			m_PendingRequestedIndex = index;
			return;
		}

		moveIndex(m_pStackedWidget->currentIndex(), index);
	}

	void StackedWidgetAnimator::moveIndex(int fromIndex, int toIndex)
	{
		if (fromIndex == toIndex)
			return;		

		m_bAnimInProgress = true;

		QWidget* fromWidget = m_pStackedWidget->widget(fromIndex);
		QWidget* toWidget   = m_pStackedWidget->widget(toIndex);

		int offsetx = m_pStackedWidget->frameRect().width(); 
		int offsety = m_pStackedWidget->frameRect().height();

		toWidget->setGeometry (0,  0, offsetx, offsety);

		if (fromIndex < toIndex)
		{
			offsetx = -offsetx;
			offsety = 0;
		}
		else
		{
			offsety = 0;
		}

		QPoint fromWidgetPos= fromWidget->pos();
		QPoint toWidgetPos= toWidget->pos();		
		m_fromWidgetPos = fromWidgetPos;

		toWidget->move(toWidgetPos.x() - offsetx, toWidgetPos.y() - offsety);
		toWidget->show();
		toWidget->raise();

		QParallelAnimationGroup *animationGroup = new QParallelAnimationGroup;
		animationGroup->addAnimation(getAnimation(fromWidget, QPoint(fromWidgetPos.x(), fromWidgetPos.y()), QPoint(offsetx + fromWidgetPos.x(), offsety + fromWidgetPos.y())));
		animationGroup->addAnimation(getAnimation(toWidget, QPoint(-offsetx + toWidgetPos.x(), offsety + toWidgetPos.y()), QPoint(toWidgetPos.x(), toWidgetPos.y())));

		m_ToIndex   = toIndex;
		m_FromIndex = fromIndex;

		connect(animationGroup, SIGNAL(finished()), this, SLOT(onAnimationFinished()));
		animationGroup->start(QAbstractAnimation::DeleteWhenStopped);
	}


	void StackedWidgetAnimator::onAnimationFinished()
	{
		m_pStackedWidget->setCurrentIndex(m_ToIndex);
		m_pStackedWidget->widget(m_FromIndex)->hide();
		m_pStackedWidget->widget(m_FromIndex)->move(m_fromWidgetPos);

		m_ToIndex = -1;
		m_FromIndex = -1;
		m_bAnimInProgress = false;

		int pendingIndex = m_PendingRequestedIndex;
		m_PendingRequestedIndex = -1;

		if (pendingIndex > 0)
			setCurrentIndex(pendingIndex);
	}

	QPropertyAnimation* StackedWidgetAnimator::getAnimation(QWidget* pWidget, const QPoint& startVal, const QPoint& endVal)
	{
		QPropertyAnimation *pPropertyAnimation = new QPropertyAnimation(pWidget, "pos");
		pPropertyAnimation->setDuration(500);
		pPropertyAnimation->setEasingCurve(QEasingCurve::Linear);
		pPropertyAnimation->setStartValue(startVal);
		pPropertyAnimation->setEndValue(endVal);
		return pPropertyAnimation;
	}
}

