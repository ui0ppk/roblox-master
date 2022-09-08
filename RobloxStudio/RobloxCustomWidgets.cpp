/**
 * RobloxCustomWidgets.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxCustomWidgets.h"

// Qt Headers
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <QHelpEvent>
#include <QToolTip>
#include <QStyleOption>
#include <QStylePainter>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QDesktopWidget>
#include <QtCore>

// Roblox Headers
#include "util/BrickColor.h"
#include "V8DataModel/ToolsPart.h"
#include "V8DataModel/Commands.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/ContentProvider.h"
#include "appdraw/Draw.h"

// Roblox Studio Headers
#include "PropertyItems.h"
#include "RobloxGameExplorer.h"
#include "RobloxPropertyWidget.h"
#include "UpdateUIManager.h"
#include "RobloxDocManager.h"
#include "RobloxIDEDoc.h"
#include "RobloxTreeWidget.h"
#include "RobloxMainWindow.h"


#define COLOR_COLS      8
#define MATERIAL_COLS   4
#define MARGIN_Y        4
#define MARGIN_X        4

#define COLOR_SPACE     6
#define MATERIAL_SPACE  6

#define COLOR_SIZE     30
#define MAT_IMAGE_SIZE 26

#define COLOR_POLY_SIDE 14
#define COLOR_POLY_DIFF 2

const QRectF ColorPickerFrame::m_selectionRectangleAdjustment = QRectF(-2, -2, 3, 3);
const QRectF ColorPickerFrame::m_secondaryRectangleAdjustment = QRectF(-1, -1, 0, 0);

ColorPickerFrame::ColorPickerFrame(QWidget *parent)
: PickerFrame(parent)
, m_selectedColorIndex(12)
, m_hoveredColorIndex(12)
, m_closeOnMouseRelease(false)
{
	int maxPolygonsInARow = getMaxPolygonsInARow();
	int numRows(maxPolygonsInARow), numCols(2*maxPolygonsInARow - 1), numColorsUsed(0);
	QPolygonF polygon;
        
    m_colorsInfo.reserve(RBX::BrickColor::colorPalette().size());
		
	for (int row = 0; row < numRows; row++)
		for (int col = 0; col < numCols; col++)
		{
			if((qAbs(numRows/2 - row) + qAbs(numCols/2 - col) < maxPolygonsInARow) && (row % 2 == col % 2))
			{
				polygon = getPolygon(PolygonSize_Normal);
				polygon.translate(qSqrt(3)/2 * (COLOR_POLY_SIDE + COLOR_POLY_DIFF) * col + 18, (COLOR_POLY_SIDE + COLOR_POLY_DIFF) * 1.5 * row + 24);

				RBX::BrickColor bColor = RBX::BrickColor::colorPalette().operator[](numColorsUsed++);
				m_colorsInfo.push_back(ColorKeyInfo(QColor(bColor.color3uint8().r, bColor.color3uint8().g, bColor.color3uint8().b), 
						                            polygon, 
						                            bColor.name().c_str(),
													bColor.asInt()));
			}
		}

	static RBX::BrickColor::Number lowerLineColorIDs[12] = {RBX::BrickColor::brick_199, RBX::BrickColor::brick_194, RBX::BrickColor::roblox_1002, 
			                                                RBX::BrickColor::brick_325, RBX::BrickColor::brick_348, RBX::BrickColor::brick_26, 
															RBX::BrickColor::brick_302, RBX::BrickColor::brick_311, RBX::BrickColor::brick_320,  
															RBX::BrickColor::brick_335, RBX::BrickColor::roblox_1001, RBX::BrickColor::brick_1};

	int colorCount = RBX::BrickColor::colorPalette().size(), lowerLineColorCount = 0; 
	
	for (int ii = 0; ii < numCols - 1; ++ii)
	{
		if (!(ii % 2))
			continue;
				
		polygon = getPolygon(PolygonSize_Normal);
		polygon.translate(qSqrt(3)/2 * (COLOR_POLY_SIDE + COLOR_POLY_DIFF) * ii + 18, (COLOR_POLY_SIDE + COLOR_POLY_DIFF) * 1.5 * numRows + 24 + 4);

		RBX::BrickColor bColor;
		if (numColorsUsed < colorCount)
			bColor = RBX::BrickColor::colorPalette().operator[](numColorsUsed++);
		else 
			bColor = RBX::BrickColor(lowerLineColorIDs[lowerLineColorCount++]);

		m_colorsInfo.push_back(ColorKeyInfo(QColor(bColor.color3uint8().r, bColor.color3uint8().g, bColor.color3uint8().b), polygon, bColor.name().c_str(), bColor.asInt()));
	}

	// check if we want to compare color names instead of using index
	m_selectedColorIndex = 123;
	m_hoveredColorIndex = 123;

	int width  = maxPolygonsInARow * 2 * (qSqrt(3)/2 * (COLOR_POLY_SIDE + COLOR_POLY_DIFF)) + 8;
	int height = (maxPolygonsInARow + 1) * (COLOR_POLY_SIDE + COLOR_POLY_DIFF) * 1.5 + MARGIN_Y + 20;

	resize(width, height);
}

void ColorPickerFrame::setSelectedItemQString(QString selectedItem)
{
	m_selectedColorIndex = -1;
	for(int index = 0; index < (int)m_colorsInfo.size(); index++)
	{
		if (m_colorsInfo[index].colorName == selectedItem)
		{
			m_selectedColorIndex = index;
			m_hoveredColorIndex = index;
			break;
		}
	}
}

QString ColorPickerFrame::getSelectedItemQString()
{	return m_selectedColorIndex < 0 ? QString("") : m_colorsInfo[m_selectedColorIndex].colorName; }

void ColorPickerFrame::setSelectedItem(int selectedItem)
{
	m_selectedColorIndex = -1;
	for(size_t index = 0; index < m_colorsInfo.size(); index++)
	{
		if (m_colorsInfo[index].colorValue == selectedItem)
		{
			m_selectedColorIndex = index;
			m_hoveredColorIndex = index;
			break;
		}
	}
}

int ColorPickerFrame::getSelectedItem()
{	return m_selectedColorIndex < 0 ? -1 : m_colorsInfo[m_selectedColorIndex].colorValue; }

void ColorPickerFrame::setSelectedIndex(int index)
{
	m_selectedColorIndex = -1;
	if (index >=0 && index < (int)m_colorsInfo.size())
		m_selectedColorIndex = index;
}

int ColorPickerFrame::getSelectedIndex()
{
	if (m_selectedColorIndex >=0 && m_selectedColorIndex < (int)m_colorsInfo.size())
		return m_selectedColorIndex;
	return -1;
}

void ColorPickerFrame::paintEvent(QPaintEvent *)
{
	QPainter painter( this );
	painter.save();

	painter.setPen(QPen(Qt::black));
	painter.drawRect(1, 1, width() - 1, height() - 1);
	painter.fillRect( QRect(1, 1, width() - 1, height() - 1), QApplication::palette().window());

	paintPolygonGrid(painter);
	painter.restore();
}

void ColorPickerFrame::mousePressEvent(QMouseEvent* evt)
{
	int colorIndex = getColorIndex(evt->pos());
	if (colorIndex >= 0)
	{
		m_selectedColorIndex = colorIndex;

		Q_EMIT currentBrickColorChanged(m_colorsInfo[colorIndex].colorValue);
	}
    
    // Ideally, we would like to close the picker here. We can't, because then
    // MouseRelease event goes to the window below it and messes up the logic there.
    // Instead, we raise a flag that the picker should close on MouseRelease and
    // then close it there.
    m_closeOnMouseRelease = true;
}

void ColorPickerFrame::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_closeOnMouseRelease)
    {
        this->close();
        m_closeOnMouseRelease = false;
    }
}

bool ColorPickerFrame::event(QEvent *evt)
{
	if (evt->type() == QEvent::ToolTip) 
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(evt);
		int colorIndex = getColorIndex(helpEvent->pos());

		if (colorIndex >= 0)
		{
			RBX::BrickColor color(m_colorsInfo[colorIndex].colorValue);
			QToolTip::showText(helpEvent->globalPos(), color.name().c_str());
		}
		else
		{
			QToolTip::hideText();
		}

		return true;
	}

	return QFrame::event(evt);
}

int ColorPickerFrame::getColorIndex(const QPoint& pos)
{
	for(int index = 0; index < (int)m_colorsInfo.size(); index++)
	{
		if(m_colorsInfo[index].polygon.containsPoint(pos, Qt::OddEvenFill))
			return index;
	}

	return -1;
}

void ColorPickerFrame::paintPolygonGrid(QPainter& painter)
{
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setRenderHint(QPainter::HighQualityAntialiasing, true);

	bool itemHovered = false;	
	for(size_t colorIndex = 0; colorIndex < m_colorsInfo.size(); colorIndex++)
	{
		painter.setPen(QPen(Qt::black, 2));
		painter.drawPolyline(m_colorsInfo[colorIndex].polygon);

		QPainterPath path;
		path.addPolygon(m_colorsInfo[colorIndex].polygon);
		QColor color(m_colorsInfo.at(colorIndex).color);
		painter.fillPath(path, QBrush(color));

		if (colorIndex == m_selectedColorIndex)
		{
			painter.setPen(QPen(Qt::red, 2));
			painter.drawPolyline(m_colorsInfo[colorIndex].polygon);

			QPolygonF smaller(getPolygon(PolygonSize_Small));
			smaller.translate((m_colorsInfo[colorIndex].polygon[4] + m_colorsInfo[colorIndex].polygon[1])/2);
			painter.setPen(QPen(Qt::yellow, 2));
			painter.drawPolyline(smaller);
		}

		if (m_colorsInfo[colorIndex].polygon.containsPoint(mapFromGlobal(QCursor::pos()), Qt::OddEvenFill))
		{
			itemHovered = true;

			painter.setPen(QPen(Qt::blue, 2));
			painter.drawPolyline(m_colorsInfo[colorIndex].polygon);

			QPolygonF smaller(getPolygon(PolygonSize_Small));
			smaller.translate((m_colorsInfo[colorIndex].polygon[4] + m_colorsInfo[colorIndex].polygon[1])/2);
			painter.setPen(QPen(Qt::white, 2));
			painter.drawPolyline(smaller);

			if (colorIndex != m_hoveredColorIndex)
			{
				m_hoveredColorIndex = colorIndex;
				Q_EMIT currentBrickColorChanged(m_colorsInfo[m_hoveredColorIndex].colorValue);				
			}
		}
	}

	if (!itemHovered)
	{
		m_hoveredColorIndex = -1;
		Q_EMIT currentBrickColorChanged(-1);
	}
}

int ColorPickerFrame::getMaxPolygonsInARow()
{
	int colorCount = RBX::BrickColor::colorPalette().size();
	int temp = 3, result = 0;
	while (true)
	{
		if ((temp*temp - temp/2*(temp/2+1)) > colorCount)
			break;
		result = temp;
		temp += 2;
	}
	return result;
}

static QPolygonF getPolygonFromSide(int side)
{
	QPolygonF polygon;
	qreal dx = qSqrt(3)/2 * side;
	polygon << QPointF(dx, -side/2) << QPointF(0, -side) << QPointF(-dx, -side/2)
		    << QPointF(-dx, side/2) << QPointF(0, side) << QPointF(dx, side/2) << QPointF(dx, -side/2);
	return polygon;
}

QPolygonF ColorPickerFrame::getPolygon(PolygonSize polySize)
{
	if (polySize == PolygonSize_Small)
	{
		static QPolygonF smallPolygon(getPolygonFromSide(COLOR_POLY_SIDE - COLOR_POLY_DIFF));
		return smallPolygon;
	}
	
	RBXASSERT(polySize == PolygonSize_Normal);
	static QPolygonF normalPolygon(getPolygonFromSide(COLOR_POLY_SIDE));
	return normalPolygon;
}

MaterialPickerFrame::MaterialPickerFrame( QWidget *parent ) 
: PickerFrame(parent)
, m_selectedMaterial(0)
{
	m_materialValues.append(RBX::PLASTIC_MATERIAL);
	m_materialValues.append(RBX::WOOD_MATERIAL);
	m_materialValues.append(RBX::WOODPLANKS_MATERIAL);
	m_materialValues.append(RBX::SLATE_MATERIAL);
	m_materialValues.append(RBX::CONCRETE_MATERIAL);
	m_materialValues.append(RBX::METAL_MATERIAL);
	m_materialValues.append(RBX::RUST_MATERIAL);
	m_materialValues.append(RBX::DIAMONDPLATE_MATERIAL);
	m_materialValues.append(RBX::ALUMINUM_MATERIAL);
	m_materialValues.append(RBX::GRASS_MATERIAL);
	m_materialValues.append(RBX::ICE_MATERIAL);
	m_materialValues.append(RBX::BRICK_MATERIAL);
	m_materialValues.append(RBX::SAND_MATERIAL);
	m_materialValues.append(RBX::FABRIC_MATERIAL);
	m_materialValues.append(RBX::GRANITE_MATERIAL);
	m_materialValues.append(RBX::MARBLE_MATERIAL);
	m_materialValues.append(RBX::PEBBLE_MATERIAL);
	m_materialValues.append(RBX::COBBLESTONE_MATERIAL);
	m_materialValues.append(RBX::SMOOTH_PLASTIC_MATERIAL);	
	
	//create individual images
	QImage materialsImage(":/images/MaterialPalette.PNG");

    int materialWidth = materialsImage.width();

    for (int count=0; count < qFloor(materialWidth / MAT_IMAGE_SIZE); count++)
		m_materialImages.append(materialsImage.copy(MAT_IMAGE_SIZE*count, 0, MAT_IMAGE_SIZE, MAT_IMAGE_SIZE));

	//resize frame with the created image size taking margins into consideration
    int width = (qMin(MATERIAL_COLS, m_materialImages.size()) * (MATERIAL_SPACE + MAT_IMAGE_SIZE)) + (MARGIN_X * 4) - MATERIAL_SPACE;
    int height = (qCeil((float)m_materialImages.size() / (float)MATERIAL_COLS) * (MATERIAL_SPACE + MAT_IMAGE_SIZE)) + (MARGIN_Y * 4) - MATERIAL_SPACE;

    resize(width, height);
}

void MaterialPickerFrame::setSelectedItemQString(const QString& selectedItem)
{
	m_selectedMaterial = -1;
	for(int index = 0; index < m_materialNames.count(); index++)
	{
		if(m_materialNames[index] == selectedItem)
		{
			m_selectedMaterial = index;
			break;
		}
	}
}

QString MaterialPickerFrame::getSelectedItemQString()
{	return m_selectedMaterial < 0 ? QString("") : m_materialNames[m_selectedMaterial]; }

void MaterialPickerFrame::setSelectedItem(int selectedItem)
{
	m_selectedMaterial = -1;
	for(int index = 0; index < m_materialValues.count(); index++)
	{
		if(m_materialValues[index] == selectedItem)
		{
			m_selectedMaterial = index;
			break;
		}
	}
}

int  MaterialPickerFrame::getSelectedItem()
{	return m_selectedMaterial < 0 ? -1 : m_materialValues[m_selectedMaterial]; }

void MaterialPickerFrame::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.save();

    painter.fillRect(QRect(1, 1, width() - 1, height() - 1), QApplication::palette().window());

    QPointF mousePos = this->mapFromGlobal(QCursor::pos());

    for (int materialIndex = 0; materialIndex < m_materialImages.size(); materialIndex++)
    {
        int xOffset = (2 * MARGIN_X) + (materialIndex % MATERIAL_COLS) * (MAT_IMAGE_SIZE + MATERIAL_SPACE);
        int yOffset = (2 * MARGIN_Y) + (materialIndex / MATERIAL_COLS) * (MAT_IMAGE_SIZE + MATERIAL_SPACE);

        painter.setPen(QPen(Qt::black, 1));
        painter.drawRect(xOffset, yOffset, 26, 26);

        painter.drawImage(xOffset, yOffset, m_materialImages.at(materialIndex));

        QRectF currentMaterialRect(xOffset, yOffset, MAT_IMAGE_SIZE, MAT_IMAGE_SIZE);

        if (materialIndex == m_selectedMaterial)
        {
            painter.setPen(QPen(Qt::red, 1));
            painter.drawRect(currentMaterialRect.adjusted(-MATERIAL_SPACE / 2, -MATERIAL_SPACE / 2, MATERIAL_SPACE / 2, MATERIAL_SPACE / 2));
        }

        if(currentMaterialRect.contains(mousePos))
        {
            painter.setPen(QPen(Qt::blue, 1));
            painter.drawRect(currentMaterialRect.adjusted(-MATERIAL_SPACE / 2, -MATERIAL_SPACE / 2, MATERIAL_SPACE / 2, MATERIAL_SPACE / 2));
        }
    }

    painter.restore();
}

void MaterialPickerFrame::mousePressEvent(QMouseEvent *evt)
{
	int materialIndex = getMaterialIndex(evt->pos());
	if (materialIndex >= 0)
	{
		m_selectedMaterial = materialIndex;
		Q_EMIT currentMaterialChanged(m_materialValues[materialIndex]);
	}
	
	this->close();
}

bool MaterialPickerFrame::event(QEvent *evt)
{
	if (evt->type() == QEvent::ToolTip) 
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(evt);
		int materialIndex = getMaterialIndex(helpEvent->pos());
		if (materialIndex >= 0)
		{
			std::string materialName = RBX::Reflection::EnumDesc<RBX::PartMaterial>::singleton().convertToString((RBX::PartMaterial)m_materialValues[materialIndex]);
			QToolTip::showText(helpEvent->globalPos(), materialName.c_str());
		}
		else
			QToolTip::hideText();

		return true;
	}

	return QFrame::event(evt);
}

int MaterialPickerFrame::getMaterialIndex(const QPoint& pos)
{
	if(rect().contains(pos)) 
	{
		int materialIndex = ((pos.x() - MARGIN_X)/(MAT_IMAGE_SIZE + MATERIAL_SPACE)) + MATERIAL_COLS * ((pos.y() - MARGIN_Y)/(MAT_IMAGE_SIZE + MATERIAL_SPACE));

		if (materialIndex >= 0 && materialIndex < m_materialValues.size())
			return materialIndex;
	}

	return -1;
}

CustomToolButton::CustomToolButton(QWidget *parent) 
: QToolButton(parent)
, m_pPickerFrame(NULL)
, m_bMenuButtonPressed(false)
, m_toolButtonStyle(Qt::ToolButtonIconOnly)
{
	setPopupMode(QToolButton::MenuButtonPopup);
}

CustomToolButton::~CustomToolButton()
{
}

void CustomToolButton::paintEvent (QPaintEvent *evt)
{
	if (!m_bMenuButtonPressed)
	{
		QToolButton::paintEvent(evt);
		return;
	}

	//initialize options to have push on menu behavior
	QStyleOptionToolButton styleOptions;
	initStyleOptions(&styleOptions);

	//draw tool button appropriately
	QStylePainter pPainter(this);	
	pPainter.drawComplexControl(QStyle::CC_ToolButton, styleOptions);
}

void CustomToolButton::mousePressEvent ( QMouseEvent *e )
{	
	if (e->button() == Qt::LeftButton) 
	{
		QStyleOptionToolButton styleOption;
		initStyleOption(&styleOption);

        QRect popupRect = style()->subControlRect(QStyle::CC_ToolButton, &styleOption, QStyle::SC_ToolButtonMenu, this);
        if (popupRect.isValid() && popupRect.contains(e->pos())) 
		{
			showPopupFrame();
			return;
        }
    }

	QToolButton::mousePressEvent(e);	
}

void CustomToolButton::initStyleOptions(QStyleOptionToolButton *pOption)
{	
	if (!pOption)
		return;

	pOption->initFrom(this);
    
	pOption->icon = icon();
	pOption->iconSize = iconSize();
	pOption->pos = pos();
	pOption->font = font();
    pOption->text = text();

	pOption->features = QStyleOptionToolButton::HasMenu|QStyleOptionToolButton::MenuButtonPopup;		
	pOption->subControls = QStyle::SC_ToolButtonMenu;

	pOption->activeSubControls = QStyle::SC_ToolButtonMenu;

	pOption->state |= QStyle::State_Sunken;
	pOption->toolButtonStyle = toolButtonStyle();
}

void CustomToolButton::showPopupFrame()
{
	PickerFrame *pPickerFrame = getPickerFrame();
	if (!pPickerFrame)
		return;

	m_bMenuButtonPressed = true;

	QEventLoop eventLoop;
	pPickerFrame->setEventLoop(&eventLoop);

	//TODO: Calculate correct position
	pPickerFrame->move(getPopupLocation(pPickerFrame->size()));
	pPickerFrame->show();
	repaint();
	
	Q_EMIT frameShown();

	//start event loop
	eventLoop.exec();
	
	Q_EMIT frameHidden();

	m_bMenuButtonPressed = false;
	repaint();
}

QPoint CustomToolButton::getPopupLocation(const QSize &pickerFrameSize)
{
	QPoint location;
	QRect screen = QApplication::desktop()->availableGeometry(this);
	QRect rect = this->rect();

	if (mapToGlobal(QPoint(0, rect.bottom())).y() + pickerFrameSize.height() <= screen.height()) 
		location = mapToGlobal(rect.bottomLeft()); 
	else 
		location = mapToGlobal(rect.topLeft() - QPoint(0, pickerFrameSize.height()));

	location.rx() = qMax(screen.left(), qMin(location.x(), screen.right() - pickerFrameSize.width()));
	location.ry() += 1;

	return location;
}

//delayed creation of picker frames
PickerFrame* MaterialPickerToolButton::getPickerFrame()
{
	if (!m_pPickerFrame)
	{
		m_pPickerFrame = new MaterialPickerFrame(this);
		connect(m_pPickerFrame, SIGNAL(changed(const QString &)), this, SIGNAL(changed(const QString &)));
	}

	return m_pPickerFrame;
}

PickerFrame* ColorPickerToolButton::getPickerFrame()
{
	if (!m_pPickerFrame)
	{
		m_pPickerFrame = new ColorPickerFrame(this);
		connect(m_pPickerFrame, SIGNAL(changed(const QString &)), this, SIGNAL(changed(const QString &)));
	}

	//set the current color
	m_pPickerFrame->setSelectedItem(RBX::ColorVerb::getCurrentColor().asInt());

	return m_pPickerFrame;
}

PickerFrame* FillColorPickerToolButton::getPickerFrame()
{
	if (!m_pPickerFrame)
	{
		m_pPickerFrame = new ColorPickerFrame(this);
		connect(m_pPickerFrame, SIGNAL(changed(const QString &)), this, SLOT(onPickerFrameChanged(const QString &)));
		// just in case picker frame gets deleted in between!
		connect(m_pPickerFrame, SIGNAL(destroyed()), this, SLOT(onPickerFrameDestroyed()));
	}

	//set the current color
	if (m_selectedIndex >= 0)
	{
		m_pPickerFrame->setSelectedIndex(m_selectedIndex);
	}
	else
	{
		m_pPickerFrame->setSelectedItem(RBX::FillTool::color.get().asInt());
	}

	return m_pPickerFrame;
}

void FillColorPickerToolButton::onPickerFrameChanged(const QString& selectedColor)
{
	if (m_pPickerFrame)
		m_selectedIndex = m_pPickerFrame->getSelectedIndex();
	Q_EMIT changed(selectedColor);
}

void FillColorPickerToolButton::onPickerFrameDestroyed()
{ m_pPickerFrame = NULL; }

PopupLaunchEditor::PopupLaunchEditor(PropertyItem *pParentItem, QWidget *pParent, const QIcon &itemIcon, const QString &labelText, int buttonWidth, QWidget *proxyWidget)
: QWidget(pParent)
, m_pParentItem(pParentItem)
{
	QHBoxLayout *pLayout = new QHBoxLayout(this);

	QPixmap pix = itemIcon.pixmap(QSize(16, 16));

	QToolButton *pButton = new QToolButton(this);
	pButton->setText(tr("..."));
	pButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
	pButton->setFixedWidth(buttonWidth);

	if (!pix.isNull())
		pButton->setIcon(QIcon(pix));
	
	pLayout->addWidget(pButton);
	pLayout->setMargin(0);
	pLayout->setSpacing(4);
	
	if (proxyWidget)
	{
		pLayout->addWidget(proxyWidget);
	}
	else 
	{
		QLabel *pTextLabel  = new QLabel(labelText, this);
		pTextLabel->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
		pLayout->addWidget(pTextLabel);
	}

	setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
	setFocusProxy(proxyWidget ? proxyWidget : pButton);
	
	connect(pButton, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
	connect(proxyWidget, SIGNAL(editingFinished()), this, SLOT(onEditFinished()));
}

void PopupLaunchEditor::onEditFinished()
{
	if (m_pParentItem)
	{
		m_pParentItem->setModelDataSafe(this);
		m_pParentItem->getTreeWidget()->closeEditor(this, QAbstractItemDelegate::NoHint);
	}
}

void PopupLaunchEditor::onButtonClicked()
{
	if (m_pParentItem)
	{
		m_pParentItem->buttonClicked();
		m_pParentItem->getTreeWidget()->closeEditor(this, QAbstractItemDelegate::NoHint);
	}
}

SignalConnector::SignalConnector(PropertyItem *pPropertyItem)
: m_pPropertyItem(pPropertyItem)
{
}

bool SignalConnector::connectSignalArg1(QObject *pSender, const char * signal)
{	return connect(pSender, signal, this, SLOT(genericSlotArg1(const QString &))); }

void SignalConnector::genericSlotArg1(const QString &arg)
{ 	m_pPropertyItem->onEvent(sender(), arg); }

ImagePickerWidget::ImagePickerWidget(QWidget* parent)
	: PickerFrame(parent)
	, lineEdit(new QLineEdit(this))
{
}

QString ImagePickerWidget::getImageUrl(const QString& lastValue, const QPoint& location, boost::shared_ptr<RBX::DataModel> dm)
{
	RBX::ContentProvider* cp = dm->find<RBX::ContentProvider>();

	resultString = lastValue;

	std::vector<QString> imageNames;
	UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER)
		.getListOfImages(&imageNames);

	QGridLayout* topLayout = new QGridLayout(this);

	QLabel* otherLabel = new QLabel(this);
	otherLabel->setText("URL:");
	topLayout->addWidget(otherLabel, 0, 0);

	lineEdit->setText(lastValue);
	topLayout->addWidget(lineEdit, 0, 1);

	QObject::connect(lineEdit, SIGNAL(returnPressed()),
		             this,     SLOT  (acceptLineEditValue()));

	QListWidget* scrollList = new QListWidget(this);

	for (size_t i = 0; i < imageNames.size(); ++i)
	{
		QListWidgetItem* item = new QListWidgetItem();
		item->setSizeHint(QSize(-1, 64));

		QFrame* widget = new QFrame();
		widget->setLayout(new QHBoxLayout());

		QString url = QString("rbxgameasset://%1").arg(imageNames[i]);
		QLabel* image = new QLabel();

		if (shared_ptr<const std::string> data =
			cp->requestContentString(RBX::ContentId(url.toStdString()),
				RBX::ContentProvider::PRIORITY_MFC))
		{
			QPixmap p;
			p.loadFromData((uchar*)data->c_str(), data->size());
			p = p.scaled(QSize(64, 64), Qt::KeepAspectRatio);
			image->setPixmap(p);
		}
		else
		{
			image->setText("(Loading...)");
		}

		widget->layout()->addWidget(image);
		// Trim leading "Images/" in name
		widget->layout()->addWidget(new QLabel(imageNames[i].right(imageNames[i].length() - 7)));

		item->setData(Qt::UserRole + 1, url);

		scrollList->addItem(item);
		scrollList->setItemWidget(item, widget);
	}

	// "Add" button
	QListWidgetItem* addItem = new QListWidgetItem("Add Image...");
	addItem->setSizeHint(QSize(-1, 64));
	scrollList->addItem(addItem);

	QObject::connect(scrollList, SIGNAL(itemClicked          (QListWidgetItem*)),
		             this,       SLOT  (handleListItemClicked(QListWidgetItem*)));

	topLayout->addWidget(scrollList, 1, 0, 1, 2);

	setLayout(topLayout);

	// We need to know the actual size of this widget in order to fit it into the monitor,
	// so do a fake show()/hide() it so that all of the geometry calculations happen and size()
	// gives a correct value.
	setAttribute(Qt::WA_DontShowOnScreen, true);
	show();
	hide();
	setAttribute(Qt::WA_DontShowOnScreen, false);
	move(moveLocationIntoMonitor(location));
	show();

	lineEdit->setFocus();
	lineEdit->selectAll();

	QEventLoop* eventLoop = new QEventLoop(this);
	setEventLoop(eventLoop);
	eventLoop->exec();

	return resultString;
}

QPoint ImagePickerWidget::moveLocationIntoMonitor(const QPoint& location)
{
	QRect monitorGeometry = QApplication::desktop()->screenGeometry(location);
	QSize frameSize = size();
	QPoint minLocation = monitorGeometry.topLeft();
	QPoint maxLocation = monitorGeometry.bottomRight() - QPoint(frameSize.width(), frameSize.height());
	return QPoint(std::max(minLocation.x(), std::min(location.x(), maxLocation.x())),
		std::max(minLocation.y(), std::min(location.y(), maxLocation.y())));
}

void ImagePickerWidget::acceptLineEditValue()
{
	resultString = lineEdit->text();
	close();
}

void ImagePickerWidget::handleListItemClicked(QListWidgetItem* item)
{
	QVariant url = item->data(Qt::UserRole + 1);
	bool hasUrl = url.isValid();

	if (!hasUrl)
	{
		// "Add" button clicked

		bool created = false;
		QString newName;
		AddImageDialog addImageDialog;
		addImageDialog.runModal(item->listWidget(), &created, &newName);
		if (created)
		{
			hasUrl = true;
			url = QString("rbxgameasset://%1").arg(newName);
		}
	}

	if (hasUrl)
	{
		resultString = url.toString();
		close();
	}
}


//////////////////////////////////////////////////////////////////////////


NumberSequencePropertyWidget::NumberSequencePropertyWidget( QWidget* parent, Callbacks* c ): QWidget(parent), cb(c)
{
    textEditor = new QLineEdit( "<ns text>" );
    editButton = new QPushButton( "..." );
    editButton->setFixedWidth(30);

    QGridLayout* layout = new QGridLayout(); // [Text] | [...]
    layout->addWidget(textEditor, 0, 0);
    layout->addWidget(editButton, 0, 1);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);

    this->setContentsMargins(0,0,0,0);
    this->setLayout(layout);
    this->setFocusProxy(textEditor);

    connect( textEditor, SIGNAL(editingFinished()), this, SLOT(onText()) );
    connect( editButton, SIGNAL(clicked(bool)), this, SLOT(onButton(bool)) );
}



void NumberSequencePropertyWidget::onText()
{
    cb->onTextAccepted();
}

void NumberSequencePropertyWidget::onButton(bool)
{
    cb->onCurveClicked();
}
//--------------------------------------------------------------------------------------------
// PartSelectionTool
//--------------------------------------------------------------------------------------------
const char* const sPartSelectionTool = "PartSelectionTool";
PartSelectionTool::PartSelectionTool(RBX::Workspace* workspace)
	: RBX::Named<RBX::MouseCommand, sPartSelectionTool>(workspace)
	, m_CursorName("ArrowFarCursor")
	, m_bIsValidPartSelected(false)
{
}

void PartSelectionTool::onMouseHover(const shared_ptr<RBX::InputObject>& inputObject)
{
	bool isValid = false;
	RBX::PartInstance* pPartHoveredOver = getPart(workspace, inputObject);
	partHoveredOverSignal(pPartHoveredOver, isValid);

    setHighlightPart( isValid ? shared_from(pPartHoveredOver) : boost::shared_ptr<RBX::PartInstance>());
}

shared_ptr<RBX::MouseCommand> PartSelectionTool::onMouseDown(const shared_ptr<RBX::InputObject>& inputObject)
{
	// first emit signal
	partSelectedSignal(getPart(workspace, inputObject), m_bIsValidPartSelected);

	// check if we've a valid selection
	if (!m_bIsValidPartSelected)
		return shared_from(this);
	return shared_ptr<RBX::MouseCommand>();
}

shared_ptr<RBX::MouseCommand> PartSelectionTool::onMouseUp(const shared_ptr<RBX::InputObject>& inputObject)
{
	if (!m_bIsValidPartSelected)
		return shared_from(this);
	return shared_ptr<RBX::MouseCommand>();
}

void PartSelectionTool::render3dAdorn(RBX::Adorn* adorn)
{
	RBX::MouseCommand::render3dAdorn(adorn);
	if (m_pHighlightPart)
	{
	      RBX::Draw::selectionBox(m_pHighlightPart->getPart(), adorn, 
								  RBX::ModelInstance::primaryPartHoverOverColor(), 
								  RBX::ModelInstance::primaryPartLineThickness());
	}
}

//--------------------------------------------------------------------------------------------
// PrimaryPartSelectionEditor
//--------------------------------------------------------------------------------------------
PrimaryPartSelectionEditor::PrimaryPartSelectionEditor(QWidget* parent, PropertyItem* pPropertyItem, const QString& labelText, boost::shared_ptr<RBX::ModelInstance> instance)
: QWidget(parent)
, m_pPrimaryPartPropertyItem(pPropertyItem)
, m_pModelInstance(instance)
{
	setStatusTip(tr("Select Primary Part"));

	QHBoxLayout *pLayout = new QHBoxLayout(this);
	
	QLabel *pTextLabel  = new QLabel(labelText, this);
	pTextLabel->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
	pLayout->addWidget(pTextLabel);

	QToolButton *pRemoveButton = new QToolButton(this);
	pRemoveButton->setObjectName("removeButton");
	pRemoveButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
	pRemoveButton->setFixedWidth(20);
	pRemoveButton->setIcon(QIcon(":/16x16/images/Studio 2.0 icons/16x16/delete_x_16_h.png"));
	pRemoveButton->setToolTip(tr("Remove Primary Part"));
	
	pLayout->addWidget(pRemoveButton);
	pLayout->setMargin(0);
	pLayout->setSpacing(4);

	setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
	setFocusProxy(pRemoveButton);
	
	connect(pRemoveButton, SIGNAL(clicked()), this, SLOT(onButtonClicked()));

	RBX::Workspace* workspace = RobloxDocManager::Instance().getPlayDoc()->getDataModel()->getWorkspace();
	m_pSelectionTool = RBX::Creatable<RBX::MouseCommand>::create<PartSelectionTool>(workspace);
	workspace->setMouseCommand(m_pSelectionTool);

	m_cPartSelectedConnection = m_pSelectionTool->partSelectedSignal.connect( boost::bind(&PrimaryPartSelectionEditor::onPartSelected, this, _1) );
	m_cPartHoveredOverConnection = m_pSelectionTool->partHoveredOverSignal.connect( boost::bind(&PrimaryPartSelectionEditor::onPartHoveredOver, this, _1, _2) );
	
	RobloxTreeWidget* pTreeWidget = UpdateUIManager::Instance().getViewWidget<RobloxExplorerWidget>(eDW_OBJECT_EXPLORER).getTreeWidget();
	if (pTreeWidget)
		pTreeWidget->setInstanceSelectionHandler(this);

	updateCursor("textures/ArrowFarCursor.png");
	qApp->installEventFilter(this);
}

PrimaryPartSelectionEditor::~PrimaryPartSelectionEditor()
{
	QApplication::restoreOverrideCursor();

	if (RobloxDocManager::Instance().getPlayDoc())
	{
		RBX::Workspace* workspace = RobloxDocManager::Instance().getPlayDoc()->getDataModel()->getWorkspace();
		if (workspace->getCurrentMouseCommand() == m_pSelectionTool.get())
		{
			workspace->setMouseCommand(boost::shared_ptr<RBX::MouseCommand>());
			UpdateUIManager::Instance().updateToolBars();
		}

		RobloxTreeWidget* pTreeWidget = UpdateUIManager::Instance().getViewWidget<RobloxExplorerWidget>(eDW_OBJECT_EXPLORER).getTreeWidget();
		if (pTreeWidget)
		{
			pTreeWidget->selectionModel()->clearSelection();
			pTreeWidget->setInstanceSelectionHandler(NULL);
		}
	}
}

bool PrimaryPartSelectionEditor::eventFilter(QObject* watched,QEvent* evt)
{
	if ((evt->type() == QEvent::ActionChanged) && (RobloxDocManager::Instance().getPlayDoc()))
	{
		// if there's an action change and any mouse command related action is activated then close editing operation
		static QString mouseCommandActions("advArrowToolAction actionFillColor actionMaterial anchorAction glueSurfaceAction "
			"smoothSurfaceAction weldSurfaceAction studsAction inletAction universalsAction hingeAction smoothNoOutlinesAction "
			"motorRightAction dropperAction playSoloAction lockAction  advTranslateAction advRotateAction resizeAction");
		if (!watched->objectName().isEmpty() && mouseCommandActions.contains(watched->objectName()))
		{
			QAction* pAction = qobject_cast<QAction*>(watched);
			if (pAction && pAction->isChecked())
				deleteLater();
		}
	}
	else if ((evt->type() == QEvent::KeyRelease) && (static_cast<QKeyEvent *>(evt)->key() == Qt::Key_Escape))
	{
		deleteLater();
	}
	else if (evt->type() == PRIMARY_PART_SELECTED)
	{
		if (m_pTempPrimaryPart.lock())
			setPrimaryPart(m_pTempPrimaryPart.lock().get());
	}

	return false;
}

void PrimaryPartSelectionEditor::onPartHoveredOver(RBX::PartInstance* pPart, bool& isValid)
{
	if (pPart && pPart->isDescendantOf(m_pModelInstance.get()))
	{
		isValid = true;
		updateCursor("textures/ArrowCursor.png");
	}
	else
	{
		isValid = false;
		updateCursor("textures/ArrowFarCursor.png");
	}
}

void PrimaryPartSelectionEditor::onInstanceSelected(boost::shared_ptr<RBX::Instance> instance)
{  setPrimaryPart(RBX::Instance::fastDynamicCast<RBX::PartInstance>(instance.get())); }

void PrimaryPartSelectionEditor::onInstanceHovered(boost::shared_ptr<RBX::Instance> instance)
{
	bool isValid = false;
	onPartHoveredOver(RBX::Instance::fastDynamicCast<RBX::PartInstance>(instance.get()), isValid);
}

void PrimaryPartSelectionEditor::onPartSelected(RBX::PartInstance* pPart)
{
	m_pTempPrimaryPart = shared_from(pPart);
	QApplication::postEvent(this, new RobloxCustomEvent(PRIMARY_PART_SELECTED));
}

void PrimaryPartSelectionEditor::onButtonClicked()
{  m_pPrimaryPartPropertyItem->buttonClicked(sender()->objectName()); }

bool PrimaryPartSelectionEditor::setPrimaryPart(RBX::PartInstance* pPart)
{
	if (pPart && pPart->isDescendantOf(m_pModelInstance.get()))
	{
		if (pPart == m_pModelInstance->getPrimaryPartSetByUser())
		{
			RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "Selected part is already set as primary part.");
			return false;
		}

		RBXASSERT(RobloxDocManager::Instance().getPlayDoc() && RobloxDocManager::Instance().getPlayDoc()->getDataModel());
		RBX::DataModel::LegacyLock lock(RobloxDocManager::Instance().getPlayDoc()->getDataModel(), RBX::DataModelJob::Write);

		// set primary part
		m_pModelInstance->setPrimaryPartSetByUser(pPart);
		// update property item
		m_pPrimaryPartPropertyItem->setModelData(NULL);

		return true;
	}

	return false;
}

void PrimaryPartSelectionEditor::updateCursor(const char* cursorImage)
{
	QApplication::restoreOverrideCursor();
	QPixmap pixmap(RBX::ContentProvider::getAssetFile(cursorImage).c_str());
	if (!pixmap.isNull())
	{
		QPainter painter(&pixmap);
		QPixmap toAppend(":/images/cursor_PrimaryPart.png");
		painter.drawPixmap(40, 20, toAppend);
		QApplication::setOverrideCursor(pixmap);
	}
}
