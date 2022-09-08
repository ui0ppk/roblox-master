/**
 * RobloxBrowser.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxBrowser.h"
#include "FastLog.h"

// Qt Headers
#include <QContextMenuEvent>
#include <QDialog>
#include <QFileInfo>
#include <QWebFrame>

// Roblox Studio Headers
#include "RobloxMainWindow.h"
#include "UpdateUIManager.h"

LOGVARIABLE(BrowserActivity, 0)
FASTFLAGVARIABLE(WebkitLocalStorageEnabled, false)
FASTFLAGVARIABLE(WebkitDeveloperToolsEnabled, false)

RobloxBrowser::RobloxBrowser(QWidget* parent) 
    : QWebView(parent)
    , m_pPopup(NULL)
    , m_pPopupDlg(NULL)
    , m_loadingTimer(NULL)
    , m_refreshIncr(0.0f)
{
    connect(this, SIGNAL(loadStarted()), this, SLOT(loadStarted()));
    connect(this, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));
    setAcceptDrops(true);
}

RobloxBrowser::~RobloxBrowser()
{
}

void RobloxBrowser::dropEvent(QDropEvent *evt)
{
	const QMimeData *pMimeData = evt->mimeData();

	if (pMimeData && pMimeData->hasUrls())
	{
		QList<QUrl> urlList = pMimeData->urls();
		for (int i = 0; i < urlList.size() && i < 6; ++i) 
		{
			QString fileName = urlList.at(i).toLocalFile();
			if (!fileName.isEmpty())
			{
				if (fileName.endsWith(".rbxl", Qt::CaseInsensitive) || fileName.endsWith(".rbxlx", Qt::CaseInsensitive))
				{
					UpdateUIManager::Instance().getMainWindow().handleFileOpen(fileName, IRobloxDoc::IDE);
				}
			}
		}
	}

	evt->acceptProposedAction();
}

bool RobloxBrowser::close()
{
	if(m_pPopupDlg) 
		m_pPopupDlg->close();
	return true; 
}

void RobloxBrowser::loadStarted()
{
    FASTLOGS(FLog::BrowserActivity, "URL: %s", url().toString().toStdString().c_str());
    resetLoadingTimer();
}

void RobloxBrowser::loadFinished(bool)
{
    if (m_loadingTimer)
    {
        m_loadingTimer->stop();
        delete m_loadingTimer;
        m_loadingTimer = NULL;
    }
    
    update();
}

void RobloxBrowser::resetLoadingTimer()
{
    if (m_loadingTimer)
    {
        m_loadingTimer->stop();
        delete m_loadingTimer;
        m_loadingTimer = NULL;
    }
    
    m_loadingTimer = new QTimer(this);
    connect(m_loadingTimer, SIGNAL(timeout()), this, SLOT(update()));
    m_loadingTimer->start(1000/30);
    m_refreshIncr = 0;
}

void RobloxBrowser::drawLoadingWatermark()
{
    if (m_loadingTimer)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::HighQualityAntialiasing);
        
        // Used to fade in the graphic at the beginning of the load timer.
        float fadeIn = m_refreshIncr * 0.5f;
        if (fadeIn > 1.0f)
            fadeIn = 1.0f;
        
        // The center of the graphic.
        const float centerX = rect().left() + rect().width() * 0.5f;
        const float centerY = rect().top() + rect().height() * 0.5f;
        
        // Size of the rect around the loading fins.
        const float rectSize = 40;
        
        // Number of loading fins to draw.
        const int numFins = 8;
        
        QColor loadingFinColor(0, 0, 255, 128);
        QColor rectPenColor(230, 230, 230, 208);
        QColor rectFillColor(230, 230, 230, 180);
        
        rectPenColor.setAlpha(rectPenColor.alpha() - ((1.0f - fadeIn) * rectPenColor.alpha()));
        rectFillColor.setAlpha(rectFillColor.alpha() - ((1.0f - fadeIn) * rectFillColor.alpha()));
        
        QPen rectPen(rectPenColor);
        rectPen.setWidth(3);
        rectPen.setCosmetic(true);
        
        painter.setPen(rectPen);
        painter.setBrush(rectFillColor);
        
        QRect roundRect(centerX - rectSize * 0.5f,
                        centerY - rectSize * 0.5f,
                        rectSize,
                        rectSize);
        painter.drawRoundRect(roundRect, 18);
        
        QPen pen;
        pen.setWidth(2);
        pen.setCapStyle(Qt::RoundCap);
        pen.setCosmetic(true);
        painter.setBrush(Qt::NoBrush);
        
		const float outerRadius = 10.0f;
		const float innerRadius = 4.0f;
        
        for (int i = 0; i < numFins; i++)
        {
            float pos = float(i) / float(numFins);
            pos = pos * 2.0f - 1.0f;
            
            float val = sin(m_refreshIncr * 0.5f + pos * M_PI) * 0.5f + 0.5f;
            
            pen.setColor(QColor(loadingFinColor.red(),
                                loadingFinColor.green(),
                                loadingFinColor.blue(),
                                val * loadingFinColor.alpha() - ((1.0f - fadeIn) * (val * loadingFinColor.alpha()))));
            painter.setPen(pen);
            
            float sinpos = sin(m_refreshIncr + pos * M_PI);
            float cospos = cos(m_refreshIncr + pos * M_PI);
            painter.drawLine(floor(centerX + sinpos * innerRadius + 0.5f),
                             floor(centerY + cospos * innerRadius + 0.5f),
                             floor(centerX + sinpos * outerRadius + 0.5f),
                             floor(centerY + cospos * outerRadius + 0.5f));
        }
        m_refreshIncr += 0.08f;
    }
}

void RobloxBrowser::paintEvent(QPaintEvent* event)
{
    QWebView::paintEvent(event);
    drawLoadingWatermark();
}
