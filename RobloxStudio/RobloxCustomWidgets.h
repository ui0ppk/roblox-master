/**
 * RobloxCustomWidgets.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QFrame>
#include <QToolButton>
#include <QEvent>
#include <QEventLoop>
#include <QPushButton>

// 3rd Party Headers
#include "boost/function.hpp"

namespace RBX {
    class DataModel;
}
#include "v8datamodel/MouseCommand.h"
#include "tool/ToolsArrow.h"

#include "RobloxTreeWidget.h"

class QWidget;
class QLineEdit;
class QListWidgetItem;
class PropertyItem;

#define PROPERTY_LABEL_UPDATE		QEvent::User+5
#define PROPERTY_WIDGET_UPDATE		QEvent::User+6
#define TREE_WIDGET_UPDATE			QEvent::User+7
#define OGRE_VIEW_UPDATE			QEvent::User+8
#define SCRIPT_REVIEW_UPDATE		QEvent::User+9
#define TREE_SCROLL_TO_INSTANCE     QEvent::User+10
#define TREE_WIDGET_VIEWPORT_UPDATE QEvent::User+11
#define PRIMARY_PART_SELECTED       QEvent::User+12
#define EXTRASELECTION_UPDATE		QEvent::User+13
#define TREE_WIDGET_FILTER_UPDATE	QEvent::User+14

#define PLUGIN_LOAD					QEvent::User+20

class RobloxCustomEvent: public QEvent
{
public:
	RobloxCustomEvent(int eventType) 
	: QEvent(QEvent::Type(eventType)) 
	{}
};

class RobloxCustomEventWithArg: public QEvent
{
public:
	RobloxCustomEventWithArg(int eventType, boost::function<void()> eventArg) 
	: QEvent(QEvent::Type(eventType)) 
	, m_pEventArg(NULL)
	{
		//make a copy
		m_pEventArg = new boost::function<void()>(eventArg);
	}

	~RobloxCustomEventWithArg()
	{
		delete m_pEventArg; m_pEventArg = NULL;
	}

	boost::function<void()>* m_pEventArg;
};

class PickerFrame : public QFrame
{
	Q_OBJECT

public:
	PickerFrame(QWidget *parent)
	: QFrame(parent, Qt::Popup)
	, m_pEventLoop(NULL)
	{ 
		setWindowFlags(Qt::Popup);
		setFrameShape(QFrame::StyledPanel);
		setMouseTracking(true);
		setStyleSheet( "QFrame{background-color: rgb(255, 255, 255);border-color: rgb(0, 0, 0);}" );
	}

    virtual ~PickerFrame() {}

	void setEventLoop(QEventLoop *eventLoop)
	{ m_pEventLoop = eventLoop; }

	virtual void setSelectedItem(int) {}
	virtual int getSelectedItem() {return -1; }

	virtual void setSelectedItemQString(const QString&) {}
	virtual QString getSelectedItemQString() { return QString(""); }

	virtual void setSelectedIndex(int index) {}
	virtual int getSelectedIndex() { return -1; }

Q_SIGNALS:
    void changed(const QString &selectedItem);
	
protected:
	void closeEvent(QCloseEvent *)
	{ if (m_pEventLoop) m_pEventLoop->exit(); m_pEventLoop = NULL; }

	void mouseMoveEvent(QMouseEvent *)
	{ repaint(); }

private:
	QEventLoop *m_pEventLoop;
};

class ColorPickerFrame : public PickerFrame
{
   Q_OBJECT
public:	
   ColorPickerFrame(QWidget *parent);

   void setSelectedItem(int selectedItem);
   int getSelectedItem();

   void setSelectedItemQString(QString selectedItem);
   QString getSelectedItemQString();

   void setSelectedIndex(int index);
   int getSelectedIndex();

Q_SIGNALS:
   void currentBrickColorChanged(int color);
   void currentColorChanged(QColor color);
   
private:
	void paintEvent(QPaintEvent *);
	void mousePressEvent(QMouseEvent *evt);
    void mouseReleaseEvent(QMouseEvent *event);
	bool event(QEvent *evt);

	int getColorIndex(const QPoint& pos);

	void paintPolygonGrid(QPainter& painter);

	enum PolygonSize
	{
		PolygonSize_Small = 0,
		PolygonSize_Normal
	};
	QPolygonF getPolygon(PolygonSize polySize);
	int getMaxPolygonsInARow();

	struct ColorKeyInfo
	{
		QColor color;
		QRectF rect;
		QPolygonF polygon;
		QString colorName;
		int colorValue;

		ColorKeyInfo(QColor color, QRectF rect , QString colorName, int colorValue)
		{
			this->color = color;
			this->rect = rect;
			this->colorName = colorName;
			this->colorValue = colorValue;
		}

		ColorKeyInfo(const QColor& color, const QPolygonF& poly, QString colorName, int colorValue)
		{
			this->color = color;
			this->polygon = poly;
			this->colorName = colorName;
			this->colorValue = colorValue;
		}
	};

   std::vector<ColorKeyInfo>         m_colorsInfo;
   QRectF                      m_buttonKeyRect;
   int                         m_selectedColorIndex;
   int						   m_hoveredColorIndex;

   static const QRectF		   m_selectionRectangleAdjustment;
   static const QRectF		   m_secondaryRectangleAdjustment;
   bool                        m_closeOnMouseRelease;
};

class MaterialPickerFrame : public PickerFrame
{
	 Q_OBJECT
public:
	MaterialPickerFrame( QWidget *parent );

	void setSelectedItem(int selectedItem);
    int getSelectedItem();

	void setSelectedItemQString(const QString& selectedItem);
	QString getSelectedItemQString();

Q_SIGNALS:
	void currentMaterialChanged(int material);
    
private:
   void paintEvent(QPaintEvent * evt); 
   void mousePressEvent(QMouseEvent *event );
   bool event(QEvent *evt);

   int getMaterialIndex(const QPoint& pos);

   QList<QString>	m_materialNames;
   QList<int>		m_materialValues;
   QList<QImage>	m_materialImages;

   int            m_selectedMaterial;
};

class CustomToolButton : public QToolButton
{
	Q_OBJECT
public:
    CustomToolButton(QWidget *parent = 0);	
    virtual ~CustomToolButton();

	virtual PickerFrame *getPickerFrame() { return NULL; }

    void setCustomToolButtonStyle(Qt::ToolButtonStyle style) { m_toolButtonStyle = style; }
    Qt::ToolButtonStyle getCustomToolButtonStyle() const { return m_toolButtonStyle; }

Q_SIGNALS:
    void changed(const QString &selectedItem);
	void frameShown();
	void frameHidden();

protected:
	void paintEvent(QPaintEvent * evt);
	void mousePressEvent(QMouseEvent *evt);

	void initStyleOptions(QStyleOptionToolButton *pOption);
	void showPopupFrame();

	QPoint getPopupLocation(const QSize &pickerFrameSize);

    PickerFrame*            m_pPickerFrame;
	bool                    m_bMenuButtonPressed;
    Qt::ToolButtonStyle     m_toolButtonStyle;
};

class ColorPickerToolButton : public CustomToolButton
{
public:
    ColorPickerToolButton(QWidget *parent)
	: CustomToolButton(parent)
	{ }

	PickerFrame *getPickerFrame();
};

class FillColorPickerToolButton : public CustomToolButton
{
	Q_OBJECT
public:
    FillColorPickerToolButton(QWidget *parent)
	: CustomToolButton(parent)
	, m_selectedIndex(-1)
	{ }

	PickerFrame *getPickerFrame();

private Q_SLOTS:
	void onPickerFrameChanged(const QString & selectedColor);
	void onPickerFrameDestroyed();

private:
	int      m_selectedIndex;
};

class MaterialPickerToolButton : public CustomToolButton
{
public:
    MaterialPickerToolButton(QWidget *parent)
	: CustomToolButton(parent)
	{ }

	PickerFrame *getPickerFrame();
};

class PropertyItem;
class PopupLaunchEditor: public QWidget
{
	Q_OBJECT
public:
	PopupLaunchEditor(PropertyItem *pParentItem, QWidget *pParent, const QIcon &itemIcon, const QString &labelText, int buttonWidth, QWidget *proxyWidget=NULL);	

private Q_SLOTS:
	void onButtonClicked();
	void onEditFinished();

private:
	PropertyItem *m_pParentItem;
};

class SignalConnector:public QObject
{
	Q_OBJECT
public:
	SignalConnector(PropertyItem *pPropertyItem);
	
	//add signal/slot as per requirement
	bool connectSignalArg1(QObject *pObjToConnect, const char * signal);
	
private Q_SLOTS:
	void genericSlotArg1(const QString &);
	
private:
	PropertyItem *m_pPropertyItem;
};

class ImagePickerWidget : public PickerFrame
{
	Q_OBJECT
public:
	ImagePickerWidget(QWidget* parent);
	QString getImageUrl(const QString& lastValue, const QPoint& location, boost::shared_ptr<RBX::DataModel> dm);

private:
	QLineEdit* lineEdit;
	QString resultString;

	QPoint moveLocationIntoMonitor(const QPoint& location);

	Q_INVOKABLE void acceptLineEditValue();
	Q_INVOKABLE void handleListItemClicked(QListWidgetItem* item);
};



// used exclusively by NumberSequencePropertyItem
// has a text box and a [...] button to call the editor
class NumberSequencePropertyWidget : public QWidget
{
    Q_OBJECT
public:
    struct Callbacks
    {
        virtual void onTextAccepted() = 0;
        virtual void onCurveClicked() = 0;
    };

    QLineEdit* textEditor;
    QPushButton* editButton;
    Callbacks* cb;

    NumberSequencePropertyWidget( QWidget* parent, Callbacks* cb );

private Q_SLOTS:
    void onText();
    void onButton(bool);

};
extern const char* const sPartSelectionTool;
class PartSelectionTool : public RBX::Named<RBX::MouseCommand, sPartSelectionTool>
{
public:
	PartSelectionTool(RBX::Workspace* workspace);
	void setHighlightPart(boost::shared_ptr<RBX::PartInstance> pPart) { m_pHighlightPart = pPart; }

	rbx::signal<void(RBX::PartInstance*, bool&)> partSelectedSignal;
	rbx::signal<void(RBX::PartInstance*, bool&)> partHoveredOverSignal;

private:
	/*override*/ const std::string getCursorName() const	{return m_CursorName;}
	/*override*/ void onMouseHover(const shared_ptr<RBX::InputObject>& inputObject);
	/*override*/ shared_ptr<RBX::MouseCommand> onMouseDown(const shared_ptr<RBX::InputObject>& inputObject);
	/*override*/ shared_ptr<RBX::MouseCommand> onMouseUp(const shared_ptr<RBX::InputObject>& inputObject);
	/*override*/ void render3dAdorn(RBX::Adorn* adorn);

	boost::shared_ptr<RBX::PartInstance> m_pHighlightPart;
	std::string                          m_CursorName;
	bool                                 m_bIsValidPartSelected;
};

class PrimaryPartSelectionEditor: public QWidget, public InstanceSelectionHandler
{
	Q_OBJECT
public:
	PrimaryPartSelectionEditor(QWidget* parent, PropertyItem* pItem, const QString& labelText, boost::shared_ptr<RBX::ModelInstance> instance);
	~PrimaryPartSelectionEditor();

private Q_SLOTS:
	void onButtonClicked();

private:
	/*override*/ bool eventFilter(QObject* watched,QEvent* evt);
	/*override*/ void onInstanceSelected(boost::shared_ptr<RBX::Instance> instance);
	/*override*/ void onInstanceHovered(boost::shared_ptr<RBX::Instance> instance);

	void onPartSelected(RBX::PartInstance* pPart);
	void onPartHoveredOver(RBX::PartInstance* pPart, bool& isValid);

	bool setPrimaryPart(RBX::PartInstance* pPart);
	void updateCursor(const char* cursorImage);

	boost::shared_ptr<PartSelectionTool>       m_pSelectionTool;
	boost::shared_ptr<RBX::ModelInstance>      m_pModelInstance;

	boost::weak_ptr<RBX::PartInstance>         m_pTempPrimaryPart;

	PropertyItem                              *m_pPrimaryPartPropertyItem;

	rbx::signals::scoped_connection		       m_cPartSelectedConnection;
	rbx::signals::scoped_connection		       m_cPartHoveredOverConnection;
};
