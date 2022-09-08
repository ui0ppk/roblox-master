/**
 * PropertyItems.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QObject>
#include <QTreeWidget>
#include <QDoubleSpinBox>
#include <QListWidgetItem>

// Roblox Headers
#include "reflection/Type.h"
#include "RobloxCustomWidgets.h"

namespace RBX {
	class Instance;
	namespace Reflection
	{
		class ClassDescriptor;
		class PropertyDescriptor;
		class Variant;
	}
}

class PropertyTreeWidget;

typedef std::list< RBX::Instance* >                          InstanceList;
typedef std::list<const RBX::Reflection::ClassDescriptor*>   Classes;

class PropertyItem: public QTreeWidgetItem
{
public:
	PropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor);
	PropertyItem(PropertyItem *pParentItem, const QString &name);
	virtual ~PropertyItem();

	//factory method
	static PropertyItem* createItem(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor);

	virtual Qt::TextElideMode getTextElideMode() const { return Qt::ElideRight; }

	//for editing of values, will be called from "PropertyItemDelegate" (default implementation)
	virtual QWidget* createEditor(QWidget*, const QStyleOptionViewItem&) { return NULL; }
	virtual void     setModelData(QWidget*)  {}
	virtual void     setModelDataSafe(QWidget*)  {}
	virtual void     setEditorData(QWidget*) {}
	virtual bool     editorEvent(QEvent*)      { return false; }

	//will set update values from selected instance(s) to the property widget
	virtual bool update();
	//search filter
	virtual bool applyFilter(const QString &filterString);
	//incase child update is required (this can be replaced with a signal)
	virtual void syncChildren(PropertyItem*) {} 

	virtual void setTextValue(const QString &value);
	virtual QString getTextValue();

	virtual int compareInstanceValues(RBX::Instance *pPreviousInstance, RBX::Instance *pCurrentInstance);

	virtual void pushInstanceValue(RBX::Instance *pInstance);
	virtual void pullInstanceValue(RBX::Instance *pInstance);

	virtual void setVariantValue(const RBX::Reflection::Variant &value);
	virtual RBX::Reflection::Variant getVariantValue();	

	// returns weather the hook handled the edit launch
	virtual bool customLaunchEditorHook(QMouseEvent* event) { return false; }

	virtual void buttonClicked(const QString& buttonName = QString()) {}
	//add functions with different arguments as and when required
	virtual void onEvent(const QObject *pSender, const QString &arg) {}

	// for hiding Properties dynamically
	virtual bool currentlyHidden(const InstanceList &instances);

	const RBX::Reflection::PropertyDescriptor* propertyDescriptor() const { return m_pPropertyDescriptor; }

	PropertyTreeWidget* getTreeWidget();
	bool hasError() const {  return m_hasExceptionError; }

protected:
	void addClassDescriptor(const RBX::Reflection::ClassDescriptor* pClassDescriptor);
	void getSelectedInstances(InstanceList &instances);
	bool canBeShown();
	QPoint computePopupLocation(const QSize &pickerFrameSize);

	void init(const QString &itemName);

	void commitModification(bool requestWaypoint = true);

    bool handleCollisionLocally(RBX::Instance* pInstance);	

	Classes                                         m_Classes;
	const RBX::Reflection::PropertyDescriptor      *m_pPropertyDescriptor;
	PropertyItem                                   *m_pParentItem;

	RBX::Reflection::Variant                        m_variantValue;

	bool                                            m_isMultiValued;
	bool                                            m_hasExceptionError;
};

class BrickColorPropertyItem: public QObject, public PropertyItem
{
	Q_OBJECT
private:
	PopupLaunchEditor * m_pPopupLaunchEditor;

public:

	BrickColorPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor);

	QString getTextValue();

	void updateIcon();
	bool update();

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option);

	void updatePropertyValueQColor(QColor selectedColor);
	
	bool customLaunchEditorHook(QMouseEvent* event);
	void buttonClicked(const QString& buttonName = QString());

private Q_SLOTS:
	void updatePropertyValue(int selectedColor);
};


class ColorPropertyItem: public QObject, public PropertyItem
{
	Q_OBJECT
private:
	PopupLaunchEditor *m_pPopupLaunchEditor;
	QLineEdit         *m_pProxyLineEdit;
public:

	ColorPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor);
    ColorPropertyItem(PropertyItem* parent, const QString& name);

	QString getTextValue();

	void updateIcon();
	bool update();

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option);

	void setEditorData(QWidget *editor);
	bool customLaunchEditorHook(QMouseEvent* event);
	void buttonClicked(const QString& buttonName = QString());
	void setModelData(QWidget *editor);	
	void setModelDataSafe(QWidget *editor);	

	int popupLauncherButtonSize();

private Q_SLOTS:
	void updatePropertyValue(QColor selectedColor);
};

class DoubleSpinBoxWidget : public QDoubleSpinBox
{
public:
	DoubleSpinBoxWidget(QWidget* parent)
		: QDoubleSpinBox(parent)
	{}

	virtual QString textFromValue(double value) const
	{
		std::stringstream stream;
		stream << value;
		return QString(stream.str().c_str());
	}
};

class DoublePropertyItem: public QObject, public PropertyItem
{
	Q_OBJECT

public:

	DoublePropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor);
	DoublePropertyItem(PropertyItem* parent, const QString& name);

	QString getTextValue();
	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem &option);
	void setEditorData(QWidget *editor);
	void setModelData(QWidget *editor);

	void updateSpinBox();
	void updateSlider();

	virtual void setVariantValue(const RBX::Reflection::Variant &value);

	virtual bool eventFilter(QObject * obj, QEvent * evt);
	
private Q_SLOTS:
	void onSpinBoxChanged();
	void onSliderChanged(int value);
	void onSliderReleased();

private:

	QDoubleSpinBox*		m_SpinBox;
	QSlider*			m_Slider;

	QMutex				m_updateMutex;
};

