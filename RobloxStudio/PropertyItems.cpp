/**
 * PropertyItems.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "PropertyItems.h"

// Qt Headers
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QColorDialog>
#include <QFontDialog>
#include <QFileDialog>
#include <QMouseEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QPainter>
#include <QSettings>
#include <QPixmapCache>

// 3rd Party Headers
#include "G3D/Vector3.h"

// Roblox Headers
#include "v8datamodel/DataModel.h"
#include "v8datamodel/ChangeHistory.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/ModelInstance.h"
#include "v8datamodel/Selection.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/PartOperation.h"
#include "v8datamodel/Workspace.h"
#include "util/ScopedAssign.h"
#include "util/Faces.h"
#include "util/UDim.h"
#include "util/Sound.h"
#include "util/TextureId.h"
#include "util/AnimationId.h"
#include "util/MeshId.h"
#include "util/BrickColor.h"
#include "util/PhysicalProperties.h"
#include "reflection/reflection.h"
#include "tool/ToolsArrow.h"
#include "reflection/Type.h"
#include "v8datamodel/NumberSequence.h"
#include "v8datamodel/NumberRange.h"
#include "v8datamodel/ColorSequence.h"
#include "v8datamodel/Workspace.h"
#include "v8world/MaterialProperties.h"

// Roblox Studio Headers
#include "RobloxPropertyWidget.h"
#include "RobloxCustomWidgets.h"
#include "ReflectionMetadata.h"
#include "QtUtilities.h"
#include "RobloxMainWindow.h"
#include "UpdateUIManager.h"
#include "RobloxSettings.h"
#include "RobloxGameExplorer.h"
#include "SplineEditor.h"
#include "RobloxDocManager.h"
#include "RobloxIDEDoc.h"

//utility functions to create icons
static QIcon getCheckBox(int value, bool readOnly);
static QIcon getColorIcon(const QColor &color);

//formatting code from Client/Roblox/PropGrid.cpp
static void trimTrailing0s(QString& s);
static QString format(double f);

FASTFLAG(GameExplorerImagesEnabled)
FASTFLAG(StudioPropertyCompareOriginalVals)
FASTFLAGVARIABLE(StudioPropertyErrorOutput, false)
FASTFLAGVARIABLE(AnimationIdIsContentPropertyItem, false)
FASTFLAGVARIABLE(FixColorSettingsCancel, false)
FASTFLAGVARIABLE(TextFieldUTF8, false)
DYNAMIC_FASTFLAG(MaterialPropertiesEnabled)
FASTFLAG(CSGPhysicsLevelOfDetailEnabled)

static const char* kLoadStringEnabledDialogDisabled = "LoadStringEnabledDialogDisabled";

static const double m_SliderDivisor = 100.0f;

//--------------------------------------------------------------------------------------------
// StringPropertyItem
//--------------------------------------------------------------------------------------------
class StringPropertyItem: public PropertyItem
{
public:
	StringPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: PropertyItem(pPropertyDescriptor)
	{
	}

	QString getTextValue()
	{	
		std::string strVal = m_variantValue.get<std::string>();

        if (FFlag::TextFieldUTF8)
            return QString::fromUtf8(strVal.c_str());
        else
		    return strVal.c_str();
	}

	void pushInstanceValue(RBX::Instance *pInstance) 
	{
		if (!pInstance || !m_pPropertyDescriptor)
			return;
		
		setVariantValue(m_pPropertyDescriptor->getStringValue(pInstance));
	}

	void pullInstanceValue(RBX::Instance *pInstance) 
	{
		if (!pInstance || !m_pPropertyDescriptor)
			return;

		std::string strVal = m_variantValue.get<std::string>();
		m_pPropertyDescriptor->setStringValue(pInstance, strVal);
	}

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option)
	{
		QLineEdit *pLineEdit = new QLineEdit(parent);
		pLineEdit->setGeometry(option.rect);
		pLineEdit->setFrame(false);
        pLineEdit->setMaxLength(131072);    // 128k
		return pLineEdit; 
	}
	
	void setEditorData(QWidget *editor)
	{
		QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
		if (!pLineEdit)
			return;

		pLineEdit->setText(text(1));
	}

	void setModelData(QWidget *editor)
	{
		QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
		if (!pLineEdit)
			return;

		//apply values only if it has been changed
		if (pLineEdit->text() != text(1))
		{
            if (FFlag::TextFieldUTF8)
            {
                QByteArray ba = pLineEdit->text().toUtf8();
                setVariantValue(std::string(ba.constData(), ba.size()));
            }
            else
            {
                setVariantValue(pLineEdit->text().toStdString());
            }

			commitModification();
		}
	}
};

//--------------------------------------------------------------------------------------------
// BoolPropertyItem
//--------------------------------------------------------------------------------------------
class BoolPropertyItem:public PropertyItem
{
	int  m_isChecked;
public:
	BoolPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: PropertyItem(pPropertyDescriptor)
	, m_isChecked(0)
	{		
		setIcon(1, getCheckBox(m_isChecked, isReadOnly()));
	}

	QString getTextValue(const RBX::Instance*) 
	{	return QString("");}

	void toggleState()
	{
		m_isChecked = !m_isChecked;
		setIcon(1, getCheckBox(m_isChecked, isReadOnly()));

		setVariantValue((bool)m_isChecked);
		commitModification();
	}	

	void pushInstanceValue(RBX::Instance *pInstance)
	{
		PropertyItem::pushInstanceValue(pInstance);

		m_isChecked = m_variantValue.get<bool>();
		setIcon(1, getCheckBox(m_isChecked, isReadOnly()));
	}

	void pullInstanceValue(RBX::Instance *pInstance)
	{
		m_pPropertyDescriptor->setVariant(pInstance, m_variantValue.get<bool>());
	}

	int compareInstanceValues(RBX::Instance *pPreviousInstance, RBX::Instance *pCurrentInstance)
	{	
		int result = PropertyItem::compareInstanceValues(pPreviousInstance, pCurrentInstance);		
		if (!result)
		{
			m_isChecked = -1;
            setIcon(1, getCheckBox(m_isChecked, isReadOnly()));
		}

		return result;
	}

	bool editorEvent(QEvent * pEvent)
	{
		if (!m_pPropertyDescriptor->isReadOnly() && (pEvent->type() == QEvent::MouseButtonRelease))
		{
			QMouseEvent *pMouseEvent = static_cast<QMouseEvent*>(pEvent);
			if (pMouseEvent && (pMouseEvent->button() & Qt::LeftButton))
			{
				if (canModifyValue())
					toggleState();
			}
		}

		return false;
	}
    
    bool isReadOnly()
    { return m_pPropertyDescriptor ? m_pPropertyDescriptor->isReadOnly() : true; }

	bool canModifyValue()
	{
		// TODO: make this a generic function if we have to add some more properties with a message box
		if (!m_isChecked && (strcmp(m_pPropertyDescriptor->name.c_str(), "LoadStringEnabled") == 0))
		{
			if (!RobloxSettings().value(kLoadStringEnabledDialogDisabled, false).toBool())
			{
				QtUtilities::RBXConfirmationMessageBox msgBox("Activating 'LoadStringEnabled' property might make your game vulnerable to exploits \
													   (for more information\
													    <a href=\"http://wiki.roblox.com/index.php?title=Security#LoadStringEnabled\">click here</a>).\
														Are you sure you want to enable it?");

				msgBox.setMinimumWidth(410);
				msgBox.setMinimumHeight(90);
				//show message box
				msgBox.exec();
				if (msgBox.getCheckBoxState())
					RobloxSettings().setValue(kLoadStringEnabledDialogDisabled, true);
				return msgBox.getClickedYes();
			}
		}

		return true;
	}
};

//--------------------------------------------------------------------------------------------
// IntPropertyItem
//--------------------------------------------------------------------------------------------
class IntPropertyItem: public PropertyItem
{
public:
	IntPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: PropertyItem(pPropertyDescriptor)
	{
	}

	IntPropertyItem(PropertyItem *pParentItem, const QString &name)
	: PropertyItem(pParentItem, name)
	{
	}

	QString getTextValue()
	{
		int val = m_variantValue.get<int>();
		return QString::number(val);
	}

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option)
	{
		QSpinBox *pSpinBox = new QSpinBox(parent);
		pSpinBox->setGeometry(option.rect);		
		pSpinBox->setMinimum(INT_MIN);
        pSpinBox->setMaximum(INT_MAX);
		pSpinBox->setFrame(false);
		return pSpinBox; 
	}

	void setEditorData(QWidget *editor)
	{
		QSpinBox *pSpinBox = dynamic_cast<QSpinBox *>(editor);
		if (!pSpinBox)
			return;
	
		pSpinBox->setValue(text(1).toInt());
	}

	void setModelData(QWidget *editor)
	{
		QSpinBox *pSpinBox = dynamic_cast<QSpinBox *>(editor);
		if (!pSpinBox)
			return;

		//apply values only if it has been changed
		if (pSpinBox->cleanText() != text(1))
		{
			setVariantValue(pSpinBox->value());
			commitModification();
		}
	}	
};

//--------------------------------------------------------------------------------------------
// DoublePropertyItem
//--------------------------------------------------------------------------------------------

DoublePropertyItem::DoublePropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: PropertyItem(pPropertyDescriptor)
	, m_SpinBox(NULL)
	, m_Slider(NULL)
{
}

DoublePropertyItem::DoublePropertyItem(PropertyItem *pParentItem, const QString &name)
	: PropertyItem(pParentItem, name)
	, m_SpinBox(NULL)
	, m_Slider(NULL)
{
}

QString DoublePropertyItem::getTextValue()
{		
	double val = m_variantValue.get<double>();
	return format(val);
}

void DoublePropertyItem::onSliderChanged(int value)
{
	setVariantValue(value / m_SliderDivisor);
	commitModification(false);
	updateSpinBox();
}

void DoublePropertyItem::onSpinBoxChanged()
{
	setVariantValue(m_SpinBox->value());
	commitModification(false);
	updateSlider();
}

void DoublePropertyItem::onSliderReleased()
{
	commitModification();
}

void DoublePropertyItem::updateSpinBox()
{
	if (m_SpinBox)
	{
		m_SpinBox->blockSignals(true);
		m_SpinBox->setValue(m_variantValue.get<double>());
		m_SpinBox->blockSignals(false);
	}
}

void DoublePropertyItem::updateSlider()
{
	if (m_Slider)
	{
		m_Slider->blockSignals(true);
		m_Slider->setValue(m_variantValue.get<double>() * m_SliderDivisor);
		m_Slider->blockSignals(false);
	}
}

QWidget* DoublePropertyItem::createEditor(QWidget *parent, const QStyleOptionViewItem &option)
{
	double minimum = 0;
	double maximum = 0;

	if (m_pPropertyDescriptor)
	{
		if (RBX::Reflection::Metadata::Item* metadata = RBX::Reflection::Metadata::Reflection::singleton()->get(*m_pPropertyDescriptor))
		{
			minimum = metadata->minimum;
			maximum = metadata->maximum;
		}
	}

	if (minimum != maximum)
	{
		QWidget* widget = new QWidget(parent);
		QHBoxLayout* layout = new QHBoxLayout;

		layout->setContentsMargins(0,0,0,0);
		layout->setSpacing(0);

		m_Slider = new QSlider(Qt::Horizontal);
		m_Slider->setStyleSheet(STRINGIFY(QSlider{ background: white; } ));
		m_Slider->setRange(minimum * m_SliderDivisor, maximum * m_SliderDivisor);
		m_Slider->setFocusPolicy(Qt::WheelFocus);
		m_Slider->installEventFilter(this);
		m_Slider->setValue(m_variantValue.get<double>() * m_SliderDivisor);

		m_SpinBox = new QDoubleSpinBox(widget);
		m_SpinBox->setSingleStep(0.01f);
		m_SpinBox->setRange(minimum, maximum);
		m_SpinBox->setDecimals(3);
		m_SpinBox->setFocusPolicy(Qt::WheelFocus);
		m_SpinBox->installEventFilter(this);
		m_SpinBox->setValue(m_variantValue.get<double>());

		layout->addWidget(m_SpinBox);
		layout->addWidget(m_Slider);

		widget->setLayout(layout);
		widget->setGeometry(option.rect);

		connect(m_Slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged(int)));
		connect(m_Slider, SIGNAL(sliderReleased()), this, SLOT(onSliderReleased()));
		connect(m_SpinBox, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged()));

		widget->setFocusProxy(m_SpinBox);

		return widget;
		}

	DoubleSpinBoxWidget *pSpinBoxWidget = new DoubleSpinBoxWidget(parent);
	pSpinBoxWidget->setGeometry(option.rect);
	pSpinBoxWidget->setSingleStep(1);
	pSpinBoxWidget->setMinimum(-DBL_MAX);
	pSpinBoxWidget->setMaximum(DBL_MAX);
	pSpinBoxWidget->setFrame(false);		
	pSpinBoxWidget->setDecimals(FFlag::StudioPropertyErrorOutput ? 10 : 3);
	return pSpinBoxWidget; 
}

void DoublePropertyItem::setEditorData(QWidget *editor)
{
	DoubleSpinBoxWidget *pSpinBox = dynamic_cast<DoubleSpinBoxWidget *>(editor);
	if (!pSpinBox)
		return;

	pSpinBox->setValue(text(1).toDouble());
}

void DoublePropertyItem::setModelData(QWidget *editor)
{
	if (m_Slider)
	{
		if (m_SpinBox)
		{
			setVariantValue(m_SpinBox->value());
			commitModification();

			m_SpinBox->removeEventFilter(this);
			m_SpinBox = NULL;
		}

		m_Slider->removeEventFilter(this);
		m_Slider = NULL;	
	}
	else
	{
		DoubleSpinBoxWidget *pSpinBox = dynamic_cast<DoubleSpinBoxWidget *>(editor);
		if (!pSpinBox)
			return;

		//apply values only if it has been changed
		if (pSpinBox->cleanText() != text(1))
		{
			setVariantValue(pSpinBox->value());
			commitModification();
		}
	}
}

void DoublePropertyItem::setVariantValue(const RBX::Reflection::Variant &value)
{
	if (FFlag::StudioPropertyErrorOutput)
	{
		QString str1 = format(value.get<double>());
		QString str2 = QString("%1").arg(value.get<double>());
		trimTrailing0s(str2);

		if (str1 != str2)
		{
			m_variantValue = RBX::Reflection::Variant(str1.toDouble());
		}
		else
		{
			m_variantValue = value;
		}
	}
	else
	{
		m_variantValue = value;
	}

	//check if the value will be changed
	setTextValue(getTextValue());
}

bool DoublePropertyItem::eventFilter(QObject * obj, QEvent * evt)
{
	if (evt->type() == QEvent::FocusIn || evt->type() == QEvent::FocusOut)
	{
		QWidget* focus = QApplication::focusWidget();

		if (focus != m_Slider && focus != m_SpinBox)
		{
			QWidget* parentEditor = static_cast<QWidget*>(m_SpinBox->parent());
			setModelData(parentEditor);
			getTreeWidget()->closeEditor(parentEditor, QAbstractItemDelegate::NoHint);
		}
	}

	return false;
}

//--------------------------------------------------------------------------------------------
// EnumPropertyItem
//--------------------------------------------------------------------------------------------
class EnumPropertyItem: public PropertyItem
{
	typedef QMap<QString, int>     EnumItems;
	EnumItems                      m_enumItems;
	
	bool                           m_bUpdateInProgress;

	static bool shouldDisplayEnumItem(const RBX::Reflection::EnumDescriptor::Item* item)
	{
		RBX::Reflection::Metadata::EnumItem* m = RBX::Reflection::Metadata::Reflection::singleton()->get(*item);

		if (m && !m->isBrowsable())
			return false;

		return !RBX::Reflection::Metadata::Item::isDeprecated(m, *item);
	}

public:
	EnumPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: PropertyItem(pPropertyDescriptor)
	, m_bUpdateInProgress(false)
	{
		const RBX::Reflection::EnumPropertyDescriptor* pEnumDesc = dynamic_cast<const RBX::Reflection::EnumPropertyDescriptor*>(pPropertyDescriptor);
		if (pEnumDesc)
		{
			std::vector<const RBX::Reflection::EnumDescriptor::Item* >::const_iterator curIter, endIter;
			curIter = pEnumDesc->enumDescriptor.begin();
			endIter = pEnumDesc->enumDescriptor.end();
			while (curIter!=endIter)
			{
				if (shouldDisplayEnumItem(*curIter))
					m_enumItems[(*curIter)->name.c_str()] = (*curIter)->value;

				++curIter;
			}
		}
	}

	//somehow variant value got from m_pPropertyDescriptor throws an exception while casting (so setting it explicitly)
	void pushInstanceValue(RBX::Instance *pInstance)
	{
		const RBX::Reflection::EnumPropertyDescriptor* pEnumDesc = dynamic_cast<const RBX::Reflection::EnumPropertyDescriptor*>(m_pPropertyDescriptor);
		if (pEnumDesc)
			setVariantValue(pEnumDesc->getEnumValue(pInstance));
	}

	void pullInstanceValue(RBX::Instance *pInstance)
	{
		const RBX::Reflection::EnumPropertyDescriptor* pEnumDesc = dynamic_cast<const RBX::Reflection::EnumPropertyDescriptor*>(m_pPropertyDescriptor);
		if (pEnumDesc)
			pEnumDesc->setEnumValue(pInstance, m_variantValue.get<int>());
	}

	QString getTextValue()
	{
		int val = m_variantValue.get<int>();
        return m_enumItems.key(val);
	}

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option)
	{
		QComboBox *pComboBox = new QComboBox(parent);
		pComboBox->setGeometry(option.rect);
		pComboBox->setFrame(UpdateUIManager::Instance().getMainWindow().isRibbonStyle());
		
        pComboBox->addItems(m_enumItems.keys());

		return pComboBox; 
	}

	void setEditorData(QWidget *editor)
	{
		QComboBox *pComboBox = dynamic_cast<QComboBox *>(editor);
		if (!pComboBox)
			return;
	
		//set current value in the combo
		int index = pComboBox->findText(text(1));
		pComboBox->setCurrentIndex(index < 0 ? 0 : index);
		
		SignalConnector *pSignalConnector = new SignalConnector(this);
		pSignalConnector->setParent(pComboBox);
		
		//connect with the required signal
		pSignalConnector->connectSignalArg1(pComboBox, SIGNAL(currentIndexChanged(const QString &)));
	}
	
	void onEvent(const QObject *, const QString &arg) 
	{
		if (m_bUpdateInProgress)
			return;
		
		RBX::ScopedAssign<bool> ignoreCommitModif(m_bUpdateInProgress, true);

		if ( !m_enumItems.contains(arg) ) 
			return;
		setVariantValue(m_enumItems[arg]);
		commitModification();
	}
	
	bool update()
	{
		RBX::ScopedAssign<bool> ignoreCommitModif(m_bUpdateInProgress, true);
		return PropertyItem::update();
	}
};

//--------------------------------------------------------------------------------------------
// BrickColorPropertyItem
//--------------------------------------------------------------------------------------------

#define PROP_BUTTON_SIZE 20

BrickColorPropertyItem::BrickColorPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
: PropertyItem(pPropertyDescriptor)
, m_pPopupLaunchEditor(NULL)
{
}

QString BrickColorPropertyItem::getTextValue()
{
	RBX::BrickColor color = m_variantValue.get<RBX::BrickColor>();
	return color.name().c_str();
}

void BrickColorPropertyItem::updateIcon()
{
	if (m_isMultiValued)
	{
		setIcon(1, QIcon());
	}
	else
	{
		RBX::BrickColor rbxColor = m_variantValue.get<RBX::BrickColor>();
		G3D::Color4uint8 colorToConvert = rbxColor.color4uint8();
		QColor qtColor(colorToConvert.r, colorToConvert.g, colorToConvert.b, colorToConvert.a);
		setIcon(1, getColorIcon(qtColor));
	}
}

bool BrickColorPropertyItem::update()
{
	if (!PropertyItem::update())
		return false;	

	updateIcon();

	return true;		
}

QWidget* BrickColorPropertyItem::createEditor(QWidget *parent, const QStyleOptionViewItem &option)
{
	m_pPopupLaunchEditor = new PopupLaunchEditor(this, parent, icon(1), text(1), PROP_BUTTON_SIZE);
	m_pPopupLaunchEditor->setGeometry(option.rect);		
	m_pPopupLaunchEditor->setAutoFillBackground(true);

	return m_pPopupLaunchEditor;
}

bool BrickColorPropertyItem::customLaunchEditorHook(QMouseEvent* event)
{
	getTreeWidget()->storeOriginalValues(this);
	buttonClicked();
	return true;
}

void BrickColorPropertyItem::buttonClicked(const QString& buttonName)
{
	RBX::BrickColor color = m_variantValue.get<RBX::BrickColor>();

	ColorPickerFrame *pPickerFrame = new ColorPickerFrame(m_pPopupLaunchEditor);

	pPickerFrame->setSelectedItem(color.asInt());
	pPickerFrame->move(computePopupLocation(pPickerFrame->size()));
	pPickerFrame->show();

	QString previousColor = text(1);

	connect(pPickerFrame, SIGNAL(currentBrickColorChanged(int)), this, SLOT(updatePropertyValue(int)));

	QEventLoop* eventLoop = new QEventLoop(pPickerFrame);
	pPickerFrame->setEventLoop(eventLoop);		
	eventLoop->exec();

	getTreeWidget()->resetPropertyCallback();

	int selectedColor = pPickerFrame->getSelectedItem();
	if (selectedColor != -1 && ( (selectedColor != color.asInt()) || 
			                        (FFlag::StudioPropertyCompareOriginalVals && !getTreeWidget()->hasSameOriginalValues())))
	{
		setVariantValue(RBX::BrickColor(selectedColor));
		commitModification();

		m_isMultiValued = false;
		updateIcon();
	}
	else
	{
		getTreeWidget()->restoreOriginalValues(this);
		getTreeWidget()->resetOriginalValues();
	}

	m_pPopupLaunchEditor = NULL;
}

void BrickColorPropertyItem::updatePropertyValueQColor(QColor selectedColor)
{
	if (!selectedColor.isValid())
	{
		getTreeWidget()->restoreOriginalValues(this);
		return;
	}

	RBX::BrickColor brickColor = RBX::BrickColor::closest(G3D::Color3::fromARGB(selectedColor.rgba()));

	setVariantValue(brickColor);
	commitModification(false);

	m_isMultiValued = false;
	updateIcon();
}

void BrickColorPropertyItem::updatePropertyValue(int selectedColor)
{
	if (selectedColor == -1)
		return;

	RBX::BrickColor brickColor(selectedColor);

	setVariantValue(brickColor);
	commitModification(false);

	m_isMultiValued = false;
	updateIcon();
}

//--------------------------------------------------------------------------------------------
// ColorPropertyItem
//--------------------------------------------------------------------------------------------
ColorPropertyItem::ColorPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: PropertyItem(pPropertyDescriptor)
	, m_pPopupLaunchEditor(NULL)
	, m_pProxyLineEdit(NULL)
{
}

ColorPropertyItem::ColorPropertyItem(PropertyItem* parent, const QString& name)
    : PropertyItem(parent, name)
    , m_pPopupLaunchEditor(NULL)
    , m_pProxyLineEdit(NULL)
{
}

QString ColorPropertyItem::getTextValue()
{
	G3D::Color3 color = m_variantValue.get<G3D::Color3>();
	return QString("[%1, %2, %3]").arg((int)(color.r *255)).arg((int)(color.g *255)).arg((int)(color.b *255));
}

void ColorPropertyItem::updateIcon()
{
	if (m_isMultiValued)
	{
		setIcon(1, QIcon());
	}
	else
	{
		G3D::Color3 color = m_variantValue.get<G3D::Color3>();			
		QColor qtColor(color.r *255, color.g *255, color.b *255);
		setIcon(1, getColorIcon(qtColor));
	}
}

bool ColorPropertyItem::update()
{
	if (!PropertyItem::update())
		return false;	

	updateIcon();

	return true;		
}

QWidget* ColorPropertyItem::createEditor(QWidget *parent, const QStyleOptionViewItem &option)
{
	m_pProxyLineEdit = new QLineEdit();
	m_pProxyLineEdit->setFrame(false);

	m_pPopupLaunchEditor = new PopupLaunchEditor(this, parent, icon(1), text(1), PROP_BUTTON_SIZE, m_pProxyLineEdit);
	m_pPopupLaunchEditor->setGeometry(option.rect);		
	m_pPopupLaunchEditor->setAutoFillBackground(true);

	return m_pPopupLaunchEditor;
}
	
void ColorPropertyItem::setEditorData(QWidget *editor)
{
	m_pProxyLineEdit->setParent(m_pPopupLaunchEditor);
		
	G3D::Color3 color = m_variantValue.get<G3D::Color3>();
	m_pProxyLineEdit->setText(QString("%1, %2, %3").arg((int)(color.r *255)).arg((int)(color.g *255)).arg((int)(color.b *255)));
}

bool ColorPropertyItem::customLaunchEditorHook(QMouseEvent* event)
{
	getTreeWidget()->storeOriginalValues(this);
	int sectionPos = treeWidget()->header()->sectionViewportPosition(1);
	if (sectionPos <= event->pos().x() && sectionPos + popupLauncherButtonSize() >= event->pos().x())
	{
		buttonClicked();
		return true;
	}
	return false;
}

void ColorPropertyItem::buttonClicked(const QString& buttonName)
{
    // restore the custom colors the user has set
    QSettings settings;
    for ( int i = 0 ; i < QColorDialog::customCount() ; i++ )
    {
        QRgb c = settings.value("CustomColor/" + QString::number(i),-1).toInt();
        QColorDialog::setCustomColor(i,c);
    }

	// set the original color in the color dialog
    G3D::Color3 originalColor = m_variantValue.get<G3D::Color3>();
    QColor originalQColor(originalColor.r * 255,originalColor.g * 255,originalColor.b * 255);
    QColorDialog colorDialog(originalQColor,getTreeWidget());

	connect(&colorDialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(updatePropertyValue(QColor))); 		
		
	if (colorDialog.exec())
	{
		getTreeWidget()->resetPropertyCallback();

		QColor selectedColor = colorDialog.selectedColor();

        G3D::Color3 color = G3D::Color3::fromARGB(selectedColor.rgba());
		setVariantValue(color);
		commitModification();
		
		m_isMultiValued = false;
		updateIcon();

        // save the custom colors the user might have set
        for ( int i = 0 ; i < QColorDialog::customCount() ; i++ )
            settings.setValue("CustomColor/" + QString::number(i),QColorDialog::customColor(i));
	}
	else
	{
		getTreeWidget()->resetPropertyCallback();
		if (FFlag::FixColorSettingsCancel && !getTreeWidget()->hasOriginalValues()) 
		{
			setVariantValue(originalColor);
			commitModification();
			m_isMultiValued = false;
			updateIcon();
		}
		else 
		{
			getTreeWidget()->restoreOriginalValues(this);
			getTreeWidget()->resetOriginalValues();
		}
	}

	m_pPopupLaunchEditor = NULL; m_pProxyLineEdit = NULL;
}

void ColorPropertyItem::updatePropertyValue(QColor selectedColor)
{
	G3D::Color3 color = G3D::Color3::fromARGB(selectedColor.rgba());
	setVariantValue(color);

	commitModification(false);

	m_isMultiValued = false;
	updateIcon();
}
	
void ColorPropertyItem::setModelDataSafe(QWidget *editor)
{
	if (m_pPopupLaunchEditor && m_pProxyLineEdit && (m_pProxyLineEdit->text() != text(1)))
	{
		G3D::Color3 editedColor;
		if (RBX::StringConverter<G3D::Color3>::convertToValue(m_pProxyLineEdit->text().toStdString(), editedColor))
		{
			setVariantValue(G3D::Color3(editedColor.r/255.0f, editedColor.g/255.0f, editedColor.b/255.0f));
			commitModification();

			m_isMultiValued = false;
			updateIcon();
		}
	}
}
	
void ColorPropertyItem::setModelData(QWidget *editor)
{
	if (m_pPopupLaunchEditor && m_pProxyLineEdit && (m_pProxyLineEdit->text() != text(1)))
	{
		G3D::Color3 editedColor;
		if (RBX::StringConverter<G3D::Color3>::convertToValue(m_pProxyLineEdit->text().toStdString(), editedColor))
		{
			setVariantValue(G3D::Color3(editedColor.r/255.0f, editedColor.g/255.0f, editedColor.b/255.0f));
			commitModification();

			m_isMultiValued = false;
			updateIcon();
		}

		m_pPopupLaunchEditor = NULL; m_pProxyLineEdit = NULL;
	}

}

int ColorPropertyItem::popupLauncherButtonSize()
{
	return PROP_BUTTON_SIZE;
}

//--------------------------------------------------------------------------------------------
// FontPropertyItem
//--------------------------------------------------------------------------------------------
class FontPropertyItem: public PropertyItem
{
public:

	FontPropertyItem(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor)
	    : PropertyItem(pPropertyDescriptor)
	{
	}

	QString getTextValue()
	{
		QFont font = m_variantValue.get<QFont>();
        QString text = font.family() + " : " + QString::number(font.pointSize()) + "pt";

        if ( font.bold() )
            text += ", Bold";
        if ( font.underline() )
            text += ", Underline";
        if ( font.italic() )
            text += ", Italic";
        if ( font.strikeOut() )
            text += ", Strikeout";

        if ( !font.bold() && !font.underline() && !font.italic() && !font.strikeOut() )
            text += ", Normal";

		return text;
	}

	bool editorEvent(QEvent* event)
	{
        if ( !m_pPropertyDescriptor->isReadOnly() && (event->type() == QEvent::MouseButtonRelease) )
		{
            QFont originalFont = m_variantValue.get<QFont>();
            bool ok = false;
            QFont newFont = QFontDialog::getFont(
                &ok,
                originalFont,
                getTreeWidget(),
                m_pPropertyDescriptor->name.c_str() );

            if ( ok && originalFont != newFont )
		    {
			    setVariantValue(newFont);
			    commitModification();
		    }
        }

        return false;
	}	
};

//--------------------------------------------------------------------------------------------
// DirPropertyItem
//--------------------------------------------------------------------------------------------
class DirPropertyItem: public PropertyItem
{
public:

	DirPropertyItem(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor)
	    : PropertyItem(pPropertyDescriptor)
	{
	}

	QString getTextValue()
	{
		QDir dir = m_variantValue.get<QDir>();
        return dir.absolutePath();
	}

	bool editorEvent(QEvent* event)
	{
        if ( !m_pPropertyDescriptor->isReadOnly() && (event->type() == QEvent::MouseButtonRelease) )
		{
            QString originalDir = m_variantValue.get<QDir>().absolutePath();
            
            QString newDir;

			newDir = QFileDialog::getExistingDirectory(getTreeWidget(),"Path",originalDir);

            if ( !newDir.isEmpty() )
            {
			    setVariantValue(QDir(newDir));
			    commitModification();
            }
        }

        return false;
	}	
};

//--------------------------------------------------------------------------------------------
// RefPropertyItem
//--------------------------------------------------------------------------------------------
class RefPropertyItem: public PropertyItem
{
	QString    m_TextValue;
public:
	RefPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: PropertyItem(pPropertyDescriptor)
	{
		//read only
		setFlags(Qt::NoItemFlags);
	}

	QString getTextValue()
	{
		std::string val = m_variantValue.get<std::string>();
		return val.c_str();
	}

	void pushInstanceValue(RBX::Instance *pInstance)
	{
		const RBX::Reflection::RefPropertyDescriptor* pRefProp = dynamic_cast<const RBX::Reflection::RefPropertyDescriptor*>(m_pPropertyDescriptor);
		const RBX::Instance *pParentInstance = boost::polymorphic_downcast<const RBX::Instance*>(pRefProp->getRefValue(pInstance));
		setVariantValue(pParentInstance ? pParentInstance->getName() : std::string(""));
	}	

	void pullInstanceValue(RBX::Instance*)
	{
		//no value change!!!
	}
};

//--------------------------------------------------------------------------------------------
// PrimaryPartPropertyItem
//--------------------------------------------------------------------------------------------
class PrimaryPartPropertyItem: public PropertyItem
{
	QString                               m_TextValue;
	boost::shared_ptr<RBX::ModelInstance> m_pModelInstance;
	QPointer<PrimaryPartSelectionEditor>  m_pPrimaryPartSelEditor;
	
public:
	PrimaryPartPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: PropertyItem(pPropertyDescriptor)
	, m_pPrimaryPartSelEditor(NULL)
	{
		setFlags(Qt::NoItemFlags);
	}

	QString getTextValue()
	{
		std::string val = m_variantValue.get<std::string>();
		return QString::fromStdString(val);
	}

	void pushInstanceValue(RBX::Instance *pInstance)
	{
		// make sure we enable edit if the no. of selected parts are only 1
		RBX::Selection* selection = RBX::ServiceProvider::find<RBX::Selection>(pInstance);
		if (!selection || selection->size() != 1)
		{
			setVariantValue(std::string(""));
			setFlags(Qt::NoItemFlags);
			m_pModelInstance.reset();
			return;
		}
		
		m_pModelInstance = shared_from(RBX::Instance::fastDynamicCast<RBX::ModelInstance>(pInstance));
		if (!m_pModelInstance)
			throw std::runtime_error("Invalid object");

		setFlags(Qt::ItemIsEditable|Qt::ItemIsSelectable|Qt::ItemIsEnabled);

		std::string strVal;
		RBX::PartInstance* pPartInstance = m_pModelInstance->getPrimaryPartSetByUser();
		if (pPartInstance)
			strVal = pPartInstance->getName().c_str();

		setVariantValue(strVal);
	}	

	void pullInstanceValue(RBX::Instance* pInstance)
	{
		if (!pInstance || !m_pPropertyDescriptor)
			return;

		std::string strVal;
		RBX::ModelInstance* mInstance = RBX::Instance::fastDynamicCast<RBX::ModelInstance>(pInstance);
		if (mInstance)
		{
			RBX::PartInstance* pPartInstance = mInstance->getPrimaryPartSetByUser();
			if (pPartInstance)
				strVal = pPartInstance->getName().c_str();
		}

		setVariantValue(strVal);
		m_pModelInstance.reset();
	}

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option)
	{
		QString partName(text(1));
		if (partName.isEmpty())
			partName = "No primary part";

        if (!m_pPrimaryPartSelEditor.isNull())
            delete m_pPrimaryPartSelEditor;

		m_pPrimaryPartSelEditor = new PrimaryPartSelectionEditor(parent, this, partName, m_pModelInstance);
		m_pPrimaryPartSelEditor->setGeometry(option.rect);
		m_pPrimaryPartSelEditor->setAutoFillBackground(true);

		UpdateUIManager::Instance().updateToolBars();

		return m_pPrimaryPartSelEditor;
	}

	void buttonClicked(const QString& buttonName)
	{
		if (buttonName == "removeButton")
		{
			// remove primary part
			if (m_pModelInstance)
				m_pModelInstance->setPrimaryPartSetByUser(NULL);
			setVariantValue(std::string(""));
			// close editor
			if (!m_pPrimaryPartSelEditor.isNull())
				getTreeWidget()->closeEditor(m_pPrimaryPartSelEditor, QAbstractItemDelegate::NoHint);
		}
	}

	void setModelData(QWidget *editor)
	{
		if (!m_pModelInstance)
			return;

		if (!editor)
		{
			std::string strVal;
			RBX::PartInstance* pPartInstance = m_pModelInstance->getPrimaryPartSetByUser();
			if (pPartInstance)
				strVal = pPartInstance->getName().c_str();
			setVariantValue(strVal);
			commitModification();
		}

		if (!m_pPrimaryPartSelEditor.isNull())
			getTreeWidget()->closeEditor(m_pPrimaryPartSelEditor, QAbstractItemDelegate::NoHint);
	}
};

//--------------------------------------------------------------------------------------------
// PhysicalPropertiesItem
//--------------------------------------------------------------------------------------------
class PhysicalPropertiesItem: public PropertyItem
{
	DoublePropertyItem *m_pChild[5];
	bool                m_checkToggled;
	int					m_componentToUpdate;
	int					m_isChecked;

	enum PhysicalPropertiesVar
	{
		PhysProp_Density			= 0, 
		PhysProp_Friction			= 1,
		PhysProp_Elasticity			= 2,
		PhysProp_FrictionWeight		= 3,
		PhysProp_ElasticityWeight	= 4
	};

public:
	PhysicalPropertiesItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: PropertyItem(pPropertyDescriptor)
	, m_componentToUpdate(-1)
	, m_checkToggled(false)
	, m_isChecked(0)
	{
		setIcon(1, getCheckBox(m_isChecked, false));

		m_pChild[PhysProp_Density] = new DoublePropertyItem(this, "Density");
		addChild(m_pChild[PhysProp_Density]);

		m_pChild[PhysProp_Friction] = new DoublePropertyItem(this, "Friction");
		addChild(m_pChild[PhysProp_Friction]);

		m_pChild[PhysProp_Elasticity] = new DoublePropertyItem(this, "Elasticity");
		addChild(m_pChild[PhysProp_Elasticity]);

		m_pChild[PhysProp_FrictionWeight] = new DoublePropertyItem(this, "FrictionWeight");
		addChild(m_pChild[PhysProp_FrictionWeight]);

		m_pChild[PhysProp_ElasticityWeight] = new DoublePropertyItem(this, "ElasticityWeight");
		addChild(m_pChild[PhysProp_ElasticityWeight]);
	}

	int compareInstanceValues(RBX::Instance *pPreviousInstance, RBX::Instance *pCurrentInstance)
	{	
		int result = 0;
		RBX::Reflection::TypedPropertyDescriptor<RBX::PhysicalProperties>* pTypedDesc = (RBX::Reflection::TypedPropertyDescriptor<RBX::PhysicalProperties>*)(m_pPropertyDescriptor);
		if (pTypedDesc)
		{
			RBX::PhysicalProperties value1 = pTypedDesc->getValue(pPreviousInstance);
			RBX::PhysicalProperties value2 = pTypedDesc->getValue(pCurrentInstance);

			result = value1 == value2;

			if (value1.getCustomEnabled() != value2.getCustomEnabled())
			{
				setIcon(1, getCheckBox(-1, false));
				result = -1;
			}
			if (value1.getDensity() != value2.getDensity())
			{
				m_pChild[PhysProp_Density]->setTextValue("");
				result = -1;
			}
			if (value1.getFriction() != value2.getFriction())
			{
				m_pChild[PhysProp_Friction]->setTextValue("");
				result = -1;
			}
			if (value1.getElasticity() != value2.getElasticity())
			{
				m_pChild[PhysProp_Elasticity]->setTextValue("");
				result = -1;
			}
			if (value1.getFrictionWeight() != value2.getFrictionWeight())
			{
				m_pChild[PhysProp_FrictionWeight]->setTextValue("");
				result = -1;
			}
			if (value1.getElasticityWeight() != value2.getElasticityWeight())
			{
				m_pChild[PhysProp_ElasticityWeight]->setTextValue("");
				result = -1;
			}			
		}
        return result;
	}

	void syncChildren(PropertyItem *pChildItem)
	{
		RBX::PhysicalProperties modifiedProperties;
		if (m_isChecked == 1)
		{
			modifiedProperties = RBX::PhysicalProperties( 
								(float) m_pChild[PhysProp_Density]->getVariantValue().get<double>(),
								(float) m_pChild[PhysProp_Friction]->getVariantValue().get<double>(),
								(float) m_pChild[PhysProp_Elasticity]->getVariantValue().get<double>(),
								(float) m_pChild[PhysProp_FrictionWeight]->getVariantValue().get<double>(),
 								(float) m_pChild[PhysProp_ElasticityWeight]->getVariantValue().get<double>());
		}
		else
		{
			modifiedProperties = RBX::PhysicalProperties();
		}

		if (!m_isMultiValued)
		{
			setVariantValue(modifiedProperties);
			commitModification();

			updateChildren();
		}
		else
		{
			m_componentToUpdate = PhysProp_Density;
			if (m_pChild[PhysProp_Friction] == pChildItem)
				m_componentToUpdate = PhysProp_Friction;
			else if (m_pChild[PhysProp_Elasticity] == pChildItem)
				m_componentToUpdate = PhysProp_Elasticity;
			else if (m_pChild[PhysProp_FrictionWeight] == pChildItem)
				m_componentToUpdate = PhysProp_FrictionWeight;
			else if (m_pChild[PhysProp_ElasticityWeight] == pChildItem)
				m_componentToUpdate = PhysProp_ElasticityWeight;

			//commit modifications
			commitModification();
		
			//reset component
			m_componentToUpdate = -1;
		}
	}

	void pushInstanceValue(RBX::Instance *pInstance)
	{
		PropertyItem::pushInstanceValue(pInstance);
		updateChildren();
	}

	void pullInstanceValue(RBX::Instance *pInstance)
	{
		//nothing special required in case there's no component modified
		if (m_componentToUpdate < 0 && !m_checkToggled)
		{
			PropertyItem::pullInstanceValue(pInstance);
			return;
		}

		RBX::Reflection::TypedPropertyDescriptor<RBX::PhysicalProperties>* pTypedDesc = (RBX::Reflection::TypedPropertyDescriptor<RBX::PhysicalProperties>*)(m_pPropertyDescriptor);
		if (!pTypedDesc)
			return;

		//set only the modified component
		RBX::PhysicalProperties value = pTypedDesc->getValue(pInstance);
		float currentDensity = value.getDensity();
		float currentFriction = value.getFriction();
		float currentElasticity = value.getElasticity();
		float currentFrictionWeight = value.getFrictionWeight();
		float currentElasticityWeight = value.getElasticityWeight();
		if (m_checkToggled)
		{
			if (m_isChecked && !value.getCustomEnabled())
			{
				RBX::PartInstance* ourPart = RBX::Instance::fastDynamicCast<RBX::PartInstance>(pInstance);
				RBX::PhysicalProperties materialProp = RBX::MaterialProperties::generatePhysicalMaterialFromPartMaterial(ourPart->getRenderMaterial());
				value = materialProp;
			}
			else if (!m_isChecked && value.getCustomEnabled())
			{
				value = RBX::PhysicalProperties();
			}
		}
		else
		{
			value = RBX::PhysicalProperties( 
				(float) m_pChild[PhysProp_Density]->getVariantValue().get<double>(),
				(float) m_pChild[PhysProp_Friction]->getVariantValue().get<double>(),
				(float) m_pChild[PhysProp_Elasticity]->getVariantValue().get<double>(),
				(float) m_pChild[PhysProp_FrictionWeight]->getVariantValue().get<double>(),
				(float) m_pChild[PhysProp_ElasticityWeight]->getVariantValue().get<double>());
		}
		/*
		else if (m_componentToUpdate == PhysProp_Density)
		{
			value = RBX::PhysicalProperties(m_pChild[PhysProp_Density]->getVariantValue().get<double>(), currentFriction, currentElasticity, currentFrictionWeight, currentElasticityWeight);
		}
		else if (m_componentToUpdate == PhysProp_Friction)
		{
			value = RBX::PhysicalProperties(currentDensity, m_pChild[PhysProp_Friction]->getVariantValue().get<double>(), currentElasticity, currentFrictionWeight, currentElasticityWeight);
		}
		else if (m_componentToUpdate == PhysProp_Elasticity)
		{
			value = RBX::PhysicalProperties(currentDensity, currentFriction, m_pChild[PhysProp_Elasticity]->getVariantValue().get<double>(), currentFrictionWeight, currentElasticityWeight);
		}
		else if (m_componentToUpdate == PhysProp_FrictionWeight)
		{
			value = RBX::PhysicalProperties(currentDensity, currentFriction, currentElasticity, m_pChild[PhysProp_FrictionWeight]->getVariantValue().get<double>(), currentElasticityWeight);
		}
		else if (m_componentToUpdate == PhysProp_ElasticityWeight)
		{
			value = RBX::PhysicalProperties(currentDensity, currentFriction, currentElasticity, currentFrictionWeight, m_pChild[PhysProp_ElasticityWeight]->getVariantValue().get<double>());
		}*/

		m_variantValue = RBX::Reflection::Variant(value);
		pTypedDesc->setValue(pInstance, value);	
	}

	void updateChildren()
	{
		//set appropriate values in children
		RBX::PhysicalProperties propertyVal = m_variantValue.get<RBX::PhysicalProperties>();
		m_isChecked = propertyVal.getCustomEnabled();
		setIcon(1, getCheckBox(m_isChecked, false));
		m_pChild[PhysProp_Density]->setVariantValue(propertyVal.getDensity());
		m_pChild[PhysProp_Friction]->setVariantValue(propertyVal.getFriction());
		m_pChild[PhysProp_Elasticity]->setVariantValue(propertyVal.getElasticity());
		m_pChild[PhysProp_FrictionWeight]->setVariantValue(propertyVal.getFrictionWeight());
		m_pChild[PhysProp_ElasticityWeight]->setVariantValue(propertyVal.getElasticityWeight());

		for (size_t i = 0; i < 5; i ++)
		{
			m_pChild[i]->setHidden(!m_isChecked);
		}
	}

	QString getTextValue() 
	{	return QString("");}

	void toggleState()
	{
		m_isChecked = !m_isChecked;
		setIcon(1, getCheckBox(m_isChecked, false));
		m_checkToggled = true;
		commitModification();
		m_checkToggled = false;
	}

	bool editorEvent(QEvent * pEvent)
	{
		if (!m_pPropertyDescriptor->isReadOnly() && (pEvent->type() == QEvent::MouseButtonRelease))
		{
			QMouseEvent *pMouseEvent = static_cast<QMouseEvent*>(pEvent);
			if (pMouseEvent && (pMouseEvent->button() & Qt::LeftButton))
			{
				toggleState();
			}
		}

		return false;
	}

	virtual void setVariantValue(const RBX::Reflection::Variant &value)
	{
		m_variantValue = value;

		if (FFlag::StudioPropertyErrorOutput)
		{
			updateChildren();
			RBX::PhysicalProperties currentProperty = m_variantValue.get<RBX::PhysicalProperties>();
			RBX::PhysicalProperties vectorVal;
			if (currentProperty.getCustomEnabled())
			{
				vectorVal = RBX::PhysicalProperties(
					  m_pChild[PhysProp_Density]->getVariantValue().get<double>()
					, m_pChild[PhysProp_Friction]->getVariantValue().get<double>()
					, m_pChild[PhysProp_Elasticity]->getVariantValue().get<double>()
					, m_pChild[PhysProp_FrictionWeight]->getVariantValue().get<double>()
					, m_pChild[PhysProp_ElasticityWeight]->getVariantValue().get<double>());
			}
			else
			{
				vectorVal = RBX::PhysicalProperties();
			}

			m_variantValue = vectorVal;
		}

		//also set its text value
		setTextValue(getTextValue());
	}
};

//--------------------------------------------------------------------------------------------
// VectorPropertyItem
//--------------------------------------------------------------------------------------------
class VectorPropertyItem: public PropertyItem
{
	DoublePropertyItem *m_pChild[3];
	int                 m_componentToUpdate;
public:
	VectorPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: PropertyItem(pPropertyDescriptor)
	, m_componentToUpdate(-1)
	{
		m_pChild[0] = new DoublePropertyItem(this, "X");
		addChild(m_pChild[0]);

		m_pChild[1] = new DoublePropertyItem(this, "Y");
		addChild(m_pChild[1]);

		m_pChild[2] = new DoublePropertyItem(this, "Z");
		addChild(m_pChild[2]);
	}

	int compareInstanceValues(RBX::Instance *pPreviousInstance, RBX::Instance *pCurrentInstance)
	{	
		int result = 0;

		RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>* pTypedDesc = (RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>*)(m_pPropertyDescriptor);
		if (pTypedDesc)
		{			
			G3D::Vector3 value1 = pTypedDesc->getValue(pPreviousInstance);
			G3D::Vector3 value2 = pTypedDesc->getValue(pCurrentInstance);
			result = value1 == value2;

			//set appropriate values in children

			for (int ii=0; ii<3; ++ii)
			{				
				if (value1[ii] != value2[ii])
				{
					m_pChild[ii]->setTextValue("");
					result = -1;
				}				
			}
		}

		return result;
	}

	void syncChildren(PropertyItem *pChildItem)
	{
		//get modified values
		G3D::Vector3 modifiedValue;
		for (int ii=0; ii<3; ++ii)
			modifiedValue[ii] = m_pChild[ii]->getVariantValue().get<double>();

		if (!m_isMultiValued)
		{
			//set and commit modification
			setVariantValue(modifiedValue);		
			commitModification();

			//update other children also
			updateChildren();
		}
		else
		{
			m_componentToUpdate = 0;
			if (m_pChild[1] == pChildItem)
				m_componentToUpdate = 1;
			else if (m_pChild[2] == pChildItem)
				m_componentToUpdate = 2;

			//commit modifications
			commitModification();

			//remove parent's info
			setText(1, "");			
			//reset component
			m_componentToUpdate = -1;
		}
	}		

	void pushInstanceValue(RBX::Instance *pInstance)
	{
		PropertyItem::pushInstanceValue(pInstance);
		updateChildren();
	}

	void pullInstanceValue(RBX::Instance *pInstance)
	{
		//nothing special required in case there's no component modified
		if (m_componentToUpdate < 0)
		{
			PropertyItem::pullInstanceValue(pInstance);
			return;
		}
		
		RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>* pTypedDesc = (RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>*)(m_pPropertyDescriptor);
		if (!pTypedDesc)
			return;

		//set only the modified component
		G3D::Vector3 value = pTypedDesc->getValue(pInstance);
		value[m_componentToUpdate] = m_pChild[m_componentToUpdate]->getVariantValue().get<double>();

        m_variantValue = RBX::Reflection::Variant(value);

        if (!handleCollisionLocally(pInstance))
		    pTypedDesc->setValue(pInstance, value);		
	}

	QString getTextValue()
	{		
		G3D::Vector3 vectorVal = m_variantValue.get<G3D::Vector3>();		
		return QString("%1, %2, %3").arg(format(vectorVal.x)).arg(format(vectorVal.y)).arg(format(vectorVal.z));
	}

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option)
	{
		QLineEdit *pLineEdit = new QLineEdit(parent);
		pLineEdit->setGeometry(option.rect);
		pLineEdit->setFrame(false);
		return pLineEdit; 
	}

	void setEditorData(QWidget *editor)
	{
		QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
		if (!pLineEdit)
			return;

		pLineEdit->setText(text(1));
	}

	void setModelData(QWidget *editor)
	{
		QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
		if (!pLineEdit)
			return;

		//apply values only if it has been changed
		if (pLineEdit->text() != text(1))
		{
			G3D::Vector3 vectorVal;
			if (RBX::StringConverter<G3D::Vector3>::convertToValue(pLineEdit->text().toStdString(), vectorVal))
			{
				setVariantValue(vectorVal);
				commitModification();

				if (!FFlag::StudioPropertyErrorOutput)
					updateChildren();
			}
		}
	}

	virtual void setVariantValue(const RBX::Reflection::Variant &value)
	{
		m_variantValue = value;

		if (FFlag::StudioPropertyErrorOutput)
		{
			updateChildren();

			G3D::Vector3 vectorVal = G3D::Vector3(
				m_pChild[0]->getVariantValue().get<double>()
				, m_pChild[1]->getVariantValue().get<double>()
				, m_pChild[2]->getVariantValue().get<double>());

			m_variantValue = vectorVal;
		}

		//also set its text value
		setTextValue(getTextValue());
	}

	void updateChildren()
	{
		//set appropriate values in children
		G3D::Vector3 vectorVal = m_variantValue.get<G3D::Vector3>();
		for (int ii=0; ii<3; ++ii)
			m_pChild[ii]->setVariantValue(vectorVal[ii]);
	}
};


//--------------------------------------------------------------------------------------------
// RectPropertyItem
//--------------------------------------------------------------------------------------------
class RectPropertyItem: public PropertyItem
{
	DoublePropertyItem *m_pChild[4];
	int                 m_componentToUpdate;
public:
	RectPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: PropertyItem(pPropertyDescriptor)
	, m_componentToUpdate(-1)
	{
		m_pChild[0] = new DoublePropertyItem(this, "X0");
		addChild(m_pChild[0]);
        
		m_pChild[1] = new DoublePropertyItem(this, "Y0");
		addChild(m_pChild[1]);
        
		m_pChild[2] = new DoublePropertyItem(this, "X1");
		addChild(m_pChild[2]);
        
        m_pChild[3] = new DoublePropertyItem(this, "Y1");
		addChild(m_pChild[3]);
	}
    
	int compareInstanceValues(RBX::Instance *pPreviousInstance, RBX::Instance *pCurrentInstance)
	{
		int result = 0;
        
		RBX::Reflection::TypedPropertyDescriptor<G3D::Rect2D>* pTypedDesc = (RBX::Reflection::TypedPropertyDescriptor<G3D::Rect2D>*)(m_pPropertyDescriptor);
		if (pTypedDesc)
		{
			G3D::Rect2D value1 = pTypedDesc->getValue(pPreviousInstance);
			G3D::Rect2D value2 = pTypedDesc->getValue(pCurrentInstance);
			result = value1 == value2;
            
            if (value1.x0() != value2.x0())
            {
                m_pChild[0]->setTextValue("");
                result = -1;
            }
            
            if (value1.y0() != value2.y0())
            {
                m_pChild[1]->setTextValue("");
                result = -1;
            }
            
            if (value1.x1() != value2.x1())
            {
                m_pChild[2]->setTextValue("");
                result = -1;
            }
            
            if (value1.y1() != value2.y1())
            {
                m_pChild[3]->setTextValue("");
                result = -1;
            }
		}
        
		return result;
	}
    
	void syncChildren(PropertyItem *pChildItem)
	{
		//get modified values
		G3D::Rect2D modifiedValue;
        modifiedValue = modifiedValue.xyxy(m_pChild[0]->getVariantValue().get<double>(),
                           m_pChild[1]->getVariantValue().get<double>(),
                           m_pChild[2]->getVariantValue().get<double>(),
                           m_pChild[3]->getVariantValue().get<double>());
        
		if (!m_isMultiValued)
		{
			//set and commit modification
			setVariantValue(modifiedValue);
			commitModification();
            
			//update other children also
			updateChildren();
		}
		else
		{
			m_componentToUpdate = 0;
			if (m_pChild[1] == pChildItem)
				m_componentToUpdate = 1;
			else if (m_pChild[2] == pChildItem)
				m_componentToUpdate = 2;
            
			//commit modifications
			commitModification();
            
			//remove parent's info
			setText(1, "");
			//reset component
			m_componentToUpdate = -1;
		}
	}
    
	void pushInstanceValue(RBX::Instance *pInstance)
	{
		PropertyItem::pushInstanceValue(pInstance);
		updateChildren();
	}
    
	void pullInstanceValue(RBX::Instance *pInstance)
	{
		//nothing special required in case there's no component modified
		if (m_componentToUpdate < 0)
		{
			PropertyItem::pullInstanceValue(pInstance);
			return;
		}
		
		RBX::Reflection::TypedPropertyDescriptor<G3D::Rect2D>* pTypedDesc = (RBX::Reflection::TypedPropertyDescriptor<G3D::Rect2D>*)(m_pPropertyDescriptor);
		if (!pTypedDesc)
			return;
        
        // TODO: remove because this is WILL CAUSE MEMORY LEAK
        //*pTypedDesc = *((RBX::Reflection::TypedPropertyDescriptor<G3D::Rect2D>*) new G3D::Rect2D());
        

        
        //if (FFlag::StudioPropertiesRespectCollisionToggle)
        //    m_variantValue = RBX::Reflection::Variant(value);
        
        //if (!handleCollisionLocally(pInstance))
        //    pTypedDesc->setValue(pInstance, value);
	}
    
	QString getTextValue()
	{
		G3D::Rect2D vectorVal = m_variantValue.get<G3D::Rect2D>();
		return QString("%1, %2, %3, %4").arg(format(vectorVal.x0())).arg(format(vectorVal.y0())).arg(format(vectorVal.x1())).arg(format(vectorVal.y1()));
	}
    
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option)
	{
		QLineEdit *pLineEdit = new QLineEdit(parent);
		pLineEdit->setGeometry(option.rect);
		pLineEdit->setFrame(false);
		return pLineEdit;
	}
    
	void setEditorData(QWidget *editor)
	{
		QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
		if (!pLineEdit)
			return;
        
		pLineEdit->setText(text(1));
	}
    
	void setModelData(QWidget *editor)
	{
		QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
		if (!pLineEdit)
			return;
        
		//apply values only if it has been changed
		if (pLineEdit->text() != text(1))
		{
			G3D::Rect2D rectVal;
			if (RBX::StringConverter<G3D::Rect2D>::convertToValue(pLineEdit->text().toStdString(), rectVal))
			{
				setVariantValue(rectVal);
				commitModification();
                
				updateChildren();
			}
		}
	}
    
	void updateChildren()
	{
		//set appropriate values in children
		G3D::Rect2D vectorVal = m_variantValue.get<G3D::Rect2D>();
        m_pChild[0]->setVariantValue(vectorVal.x0());
        m_pChild[1]->setVariantValue(vectorVal.y0());
        m_pChild[2]->setVariantValue(vectorVal.x1());
        m_pChild[3]->setVariantValue(vectorVal.y1());
	}
};


//--------------------------------------------------------------------------------------------
// UDimComponentItem
//--------------------------------------------------------------------------------------------
class UDim2ComponentItem: public PropertyItem
{
	DoublePropertyItem *m_pScale;
	IntPropertyItem    *m_pOffset;
	int                 m_pComponentID;
	int                 m_componentToUpdate;
public:
	UDim2ComponentItem(PropertyItem *pParentItem, const QString &name, int componentID)
	: PropertyItem(pParentItem, name)	
	, m_pComponentID(componentID)
	{		
		m_pScale = new DoublePropertyItem(this, "Scale");
		addChild(m_pScale);

		m_pOffset = new IntPropertyItem(this, "Offset");
		addChild(m_pOffset);		
	}

	int compareInstanceValues(const RBX::UDim &previousVal, const RBX::UDim &currentVal)
	{
		int result = 1;

		if (previousVal.scale != currentVal.scale)
		{
			m_pScale->setText(1, "");
			result = -1;
		}

		if (previousVal.offset != currentVal.offset)
		{
			m_pOffset->setText(1, "");
			result = -1;
		}		
				
		return result; 
	}	

	void setVariantValue(const RBX::Reflection::Variant &value)
	{
		PropertyItem::setVariantValue(value);
		updateChildren();
	}

	QString getTextValue()
	{
		RBX::UDim uDim = m_variantValue.get<RBX::UDim>();
		return QString("%1, %2").arg(format(uDim.scale)).arg(uDim.offset);
	}

	void syncChildren(PropertyItem *pChildItem)
	{			
		//get modified values
		RBX::UDim modifiedValue;		
		modifiedValue.scale  = m_pScale->getVariantValue().get<double>();
		modifiedValue.offset = m_pOffset->getVariantValue().get<int>();

		if (!m_isMultiValued)
		{
			//set and commit modification
			setVariantValue(modifiedValue);		
			commitModification();

			//update other children also
			updateChildren();
		}
		else
		{
			m_componentToUpdate = 0;
			if (m_pOffset == pChildItem)
				m_componentToUpdate = 1;			

			//commit modifications
			commitModification();

			//remove parent's info
			setText(1, "");			
			//reset component
			m_componentToUpdate = -1;
		}
	}

	void pullInstanceValue(RBX::Instance *pInstance)
	{
		RBX::Reflection::TypedPropertyDescriptor<RBX::UDim2>* pTypedDesc = (RBX::Reflection::TypedPropertyDescriptor<RBX::UDim2>*)(m_pParentItem->propertyDescriptor());
		if (!pTypedDesc)
			return;

		//set only the modified component
		RBX::UDim2 value = pTypedDesc->getValue(pInstance);

		//nothing special required in case there's no component modified
		if (m_componentToUpdate < 0)
		{				
			value[m_pComponentID] = m_variantValue.get<RBX::UDim>();
		}
		else
		{
			if (m_componentToUpdate > 0)
				value[m_pComponentID].offset = m_pOffset->getVariantValue().get<int>();
			else
				value[m_pComponentID].scale = m_pScale->getVariantValue().get<double>();
		}

		pTypedDesc->setValue(pInstance, value);		
	}

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option)
	{
		QLineEdit *pLineEdit = new QLineEdit(parent);
		pLineEdit->setGeometry(option.rect);
		pLineEdit->setFrame(false);
		return pLineEdit; 
	}

	void setEditorData(QWidget *editor)
	{
		QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
		if (!pLineEdit)
			return;

		pLineEdit->setText(text(1));
	}

	void setModelData(QWidget *editor)
	{
		QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
		if (!pLineEdit)
			return;

		//apply values only if it has been changed
		if (pLineEdit->text() != text(1))
		{
			RBX::UDim udimVal;
			if (RBX::StringConverter<RBX::UDim>::convertToValue(pLineEdit->text().toStdString(), udimVal))
			{
				setVariantValue(udimVal);
				commitModification();

				updateChildren();
			}
		}
	}	

	void updateChildren()
	{
		//set appropriate values in children
		RBX::UDim udimVal = m_variantValue.get<RBX::UDim>();		
		m_pScale->setVariantValue((double)udimVal.scale);
		m_pOffset->setVariantValue((int)udimVal.offset);
	}
};

//--------------------------------------------------------------------------------------------
// UDim2PropertyItem
//--------------------------------------------------------------------------------------------
class UDim2PropertyItem: public PropertyItem
{	
	UDim2ComponentItem  *m_pChild[2];
	int                  m_componentToUpdate;
public:
	UDim2PropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: PropertyItem(pPropertyDescriptor)
	, m_componentToUpdate(-1)
	{
		m_pChild[0] = new UDim2ComponentItem(this, "X", 0);
		addChild(m_pChild[0]);

		m_pChild[1] = new UDim2ComponentItem(this, "Y", 1);
		addChild(m_pChild[1]);
	}

	int compareInstanceValues(RBX::Instance *pPreviousInstance, RBX::Instance *pCurrentInstance)
	{	
		int result = 0;

		RBX::Reflection::TypedPropertyDescriptor<RBX::UDim2>* pTypedDesc = (RBX::Reflection::TypedPropertyDescriptor<RBX::UDim2>*)(m_pPropertyDescriptor);
		if (pTypedDesc)
		{			
			RBX::UDim2 value1 = pTypedDesc->getValue(pPreviousInstance);
			RBX::UDim2 value2 = pTypedDesc->getValue(pCurrentInstance);
			result = value1 == value2;

			//set appropriate values in children			
			for (int ii=0; ii < 2; ++ii)
			{
				if (m_pChild[ii]->compareInstanceValues(value1[ii], value2[ii]) <= 0)
				{
					m_pChild[ii]->setText(1, "");
					result = -1;
				}
			}
		}

		return result;
	}

	void syncChildren(PropertyItem *pChildItem)
	{			
		//get modified values
		RBX::UDim2 modifiedValue;
		for (int ii=0; ii<2; ++ii)
			modifiedValue[ii] = m_pChild[ii]->getVariantValue().get<RBX::UDim>();

		if (!m_isMultiValued)
		{
			//set and commit modification
			setVariantValue(modifiedValue);		
			commitModification();

			//update other children also
			updateChildren();
		}
		else
		{
			m_componentToUpdate = 0;
			if (m_pChild[1] == pChildItem)
				m_componentToUpdate = 1;			

			//commit modifications
			commitModification();

			//remove parent's info
			setText(1, "");			
			//reset component
			m_componentToUpdate = -1;
		}
	}		

	void pushInstanceValue(RBX::Instance *pInstance)
	{
		PropertyItem::pushInstanceValue(pInstance);
		updateChildren();
	}	

	void pullInstanceValue(RBX::Instance *pInstance)
	{
		//nothing special required in case there's no component modified
		if (m_componentToUpdate < 0)
		{
			PropertyItem::pullInstanceValue(pInstance);
			return;
		}
		
		RBX::Reflection::TypedPropertyDescriptor<RBX::UDim2>* pTypedDesc = (RBX::Reflection::TypedPropertyDescriptor<RBX::UDim2>*)(m_pPropertyDescriptor);
		if (!pTypedDesc)
			return;

		//set only the modified component
		RBX::UDim2 value = pTypedDesc->getValue(pInstance);
		value[m_componentToUpdate] = m_pChild[m_componentToUpdate]->getVariantValue().get<RBX::UDim>();
		pTypedDesc->setValue(pInstance, value);		
	}

	QString getTextValue()
	{		
		RBX::UDim2 uDim2 = m_variantValue.get<RBX::UDim2>();
		return QString("{%1, %2},{%3, %4}").arg(format(uDim2.x.scale)).arg(uDim2.x.offset).arg(format(uDim2.y.scale)).arg(uDim2.y.offset);
	}

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option)
	{
		QLineEdit *pLineEdit = new QLineEdit(parent);
		pLineEdit->setGeometry(option.rect);
		pLineEdit->setFrame(false);
		return pLineEdit; 
	}

	void setEditorData(QWidget *editor)
	{
		QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
		if (!pLineEdit)
			return;

		pLineEdit->setText(text(1));
	}

	void setModelData(QWidget *editor)
	{
		QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
		if (!pLineEdit)
			return;

		//apply values only if it has been changed
		if (pLineEdit->text() != text(1))
		{
			RBX::UDim2 udimVal;
			if (RBX::StringConverter<RBX::UDim2>::convertToValue(pLineEdit->text().toStdString(), udimVal))
			{
				setVariantValue(udimVal);
				commitModification();

				updateChildren();
			}
		}
	}	

	void updateChildren()
	{
		//set appropriate values in children
		RBX::UDim2 udimVal = m_variantValue.get<RBX::UDim2>();
		for (int ii=0; ii<2; ++ii)
			m_pChild[ii]->setVariantValue(udimVal[ii]);
	}
};




//--------------------------------------------------------------------------------------------
// FaceComponentItem
//--------------------------------------------------------------------------------------------
class FaceComponentItem: public PropertyItem
{
	RBX::NormalId    m_componentId;
public:
	FaceComponentItem(PropertyItem *pParentItem, const QString &name, RBX::NormalId component)
	: PropertyItem(pParentItem, name)
	, m_componentId(component)
	{
	}

	int compareInstanceValues(const RBX::Faces &previousFaces, const RBX::Faces &currentFaces)
	{	return previousFaces.getNormalId(m_componentId) == currentFaces.getNormalId(m_componentId); }	

	QString getTextValue()
	{
		RBX::Faces faceVal = m_variantValue.get<RBX::Faces>();		
		return RBX::StringConverter<bool>::convertToString(faceVal.getNormalId(m_componentId)).c_str();
	}

	void getValue(RBX::Faces& value)
	{		
		RBX::Faces faceVal = m_variantValue.get<RBX::Faces>();			
		value.setNormalId(m_componentId, faceVal.getNormalId(m_componentId));		
	}

	//TODO: add face item editing!
};

//--------------------------------------------------------------------------------------------
// FacesPropertyItem
//--------------------------------------------------------------------------------------------
class FacesPropertyItem: public PropertyItem
{
	FaceComponentItem  *m_pChild[6];
	int                 m_componentToUpdate;
public:
	FacesPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: PropertyItem(pPropertyDescriptor)
	, m_componentToUpdate(-1)
	{
		//create children
		m_pChild[0] = new FaceComponentItem(this, "Top",    RBX::NORM_Y);
		m_pChild[1] = new FaceComponentItem(this, "Bottom", RBX::NORM_Y_NEG);
		m_pChild[2] = new FaceComponentItem(this, "Back",   RBX::NORM_Z);
		m_pChild[3] = new FaceComponentItem(this, "Front",  RBX::NORM_Z_NEG);
		m_pChild[4] = new FaceComponentItem(this, "Right",  RBX::NORM_X);
		m_pChild[5] = new FaceComponentItem(this, "Left",   RBX::NORM_X_NEG);

		//add children
		for (int ii=0; ii<6; ++ii)
			addChild(m_pChild[ii]);
	}

	int compareInstanceValues(RBX::Instance *pPreviousInstance, RBX::Instance *pCurrentInstance)
	{	
		int result = 0;

		const RBX::Reflection::TypedPropertyDescriptor<RBX::Faces>* pFacesPropDesc = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<RBX::Faces>*>(m_pPropertyDescriptor);
		if (pFacesPropDesc)
		{			
			RBX::Faces value1 = pFacesPropDesc->getValue(pPreviousInstance);
			RBX::Faces value2 = pFacesPropDesc->getValue(pCurrentInstance);
			result = value1 == value2;

			//set appropriate values in children
			for (int ii=0; ii<6; ++ii)
			{				
				if (!m_pChild[ii]->compareInstanceValues(value1, value2))
				{
					m_pChild[ii]->setTextValue("");
					result = -1;
				}				
			}
		}

		return result;
	}

	void syncChildren(PropertyItem *pChildItem)
	{
		//get modified values
		RBX::Faces modifiedValue;
		for (int ii=0; ii<6; ++ii)
			m_pChild[ii]->getValue(modifiedValue);

		if (!m_isMultiValued)
		{
			//set and commit modification
			setVariantValue(modifiedValue);		
			commitModification();

			//update other children also
			updateChildren();
		}
		else
		{
			m_componentToUpdate = 0;
			for (int ii=0; ii<6; ++ii)
			{
				if (m_pChild[ii] == pChildItem)
				{
					m_componentToUpdate = ii;
					break;
				}
			}

			//commit modifications
			commitModification();

			//remove parent's info(TODO: check if we can update parent's value?)
			setText(1, "");			
			//reset component
			m_componentToUpdate = -1;
		}
	}		

	void pushInstanceValue(RBX::Instance *pInstance)
	{
		PropertyItem::pushInstanceValue(pInstance);
		updateChildren();
	}

	void pullInstanceValue(RBX::Instance *pInstance)
	{
		//nothing special required in case there's no component modified
		if (m_componentToUpdate < 0)
		{
			PropertyItem::pullInstanceValue(pInstance);
			return;
		}
		
		RBX::Reflection::TypedPropertyDescriptor<RBX::Faces>* pTypedDesc = (RBX::Reflection::TypedPropertyDescriptor<RBX::Faces>*)(m_pPropertyDescriptor);
		if (!pTypedDesc)
			return;

		//set only the modified component
		RBX::Faces value = pTypedDesc->getValue(pInstance);
		m_pChild[m_componentToUpdate]->getValue(value);
		pTypedDesc->setValue(pInstance, value);		
	}

	QString getTextValue()
	{		
		std::string result = RBX::StringConverter<RBX::Faces>::convertToString(m_variantValue.get<RBX::Faces>());
		return result.empty() ? "" : result.c_str();
	}

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option)
	{
		QLineEdit *pLineEdit = new QLineEdit(parent);
		pLineEdit->setGeometry(option.rect);
		pLineEdit->setFrame(false);
		return pLineEdit; 
	}

	void setEditorData(QWidget *editor)
	{
		QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
		if (!pLineEdit)
			return;

		pLineEdit->setText(text(1));
	}

	void setModelData(QWidget *editor)
	{
		QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
		if (!pLineEdit)
			return;

		//apply values only if it has been changed
		if (pLineEdit->text() != text(1))
		{
			RBX::Faces faceVal;
			if (RBX::StringConverter<RBX::Faces>::convertToValue(pLineEdit->text().toStdString(), faceVal))
			{
				setVariantValue(faceVal);
				commitModification();

				updateChildren();
			}
		}
	}	

	void updateChildren()
	{
		//set appropriate values in children
		for (int ii=0; ii<6; ++ii)
			m_pChild[ii]->setVariantValue(m_variantValue);
	}
};

//--------------------------------------------------------------------------------------------
// ContentPropertyItem
//--------------------------------------------------------------------------------------------
class ContentPropertyItem: public StringPropertyItem
{
public:
	ContentPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: StringPropertyItem(pPropertyDescriptor)
	{
	}	

	void setModelData(QWidget *editor)
	{
		QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
		if (!pLineEdit)
			return;

		//apply values only if it has been changed
		if (pLineEdit->text() != text(1))
		{
			bool ok = true;
			pLineEdit->text().toInt(&ok);

			if (ok)
				pLineEdit->setText(QString("rbxassetid://%1").arg(pLineEdit->text()));
			
			if (RBX::ContentProvider::isUrl(pLineEdit->text().toStdString()) || pLineEdit->text().isEmpty())
			{
				setVariantValue(pLineEdit->text().toStdString());
				commitModification();
			}
			else
			{
				QtUtilities::RBXMessageBox msgBox;
				msgBox.setIcon(QMessageBox::Warning);
				msgBox.setText("\"" + pLineEdit->text() + "\" is not a valid content location.\nPlease enter an asset ID or a correctly formatted asset url.");
				msgBox.setStandardButtons(QMessageBox::Ok);
				msgBox.setDefaultButton(QMessageBox::Ok);
				msgBox.exec();
			}
		}
	}
};


//--------------------------------------------------------------------------------------------
// ImageAssetPropertyItem
//--------------------------------------------------------------------------------------------

class ImageAssetPropertyItem : public ContentPropertyItem
{
public:
	ImageAssetPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
		: ContentPropertyItem(pPropertyDescriptor)
	{
	}	

	virtual Qt::TextElideMode getTextElideMode() const
	{
		return Qt::ElideLeft;
	}

	bool customLaunchEditorHook(QMouseEvent* event)
	{
		if (UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER)
				.getCurrentGameId() > 0)
		{
			QRect rect = getTreeWidget()->visualItemRect(this);
			QPoint location = getTreeWidget()->mapToGlobal(rect.bottomLeft());
			boost::shared_ptr<RBX::DataModel> dm = getTreeWidget()->getDataModel();

			ImagePickerWidget pickerFrame(treeWidget());
			QString string = pickerFrame.getImageUrl(text(1), location, dm);
		
			{
				RBX::DataModel::LegacyLock lock(dm, RBX::DataModelJob::Write);

				setVariantValue(string.toStdString());
				commitModification();
			}

			return true;
		}
		return false;
	}
};


//--------------------------------------------------------------------------------------------
// ScriptAssetPropertyItem
//--------------------------------------------------------------------------------------------

class ScriptAssetPropertyItem : public PropertyItem
{
	const QString kUseEmbeddedSourceText;
	bool currentlyCommitting;

public:
	ScriptAssetPropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
	: PropertyItem(pPropertyDescriptor)
	, kUseEmbeddedSourceText("[Embedded]")
	, currentlyCommitting(false)
	{
	}

	virtual Qt::TextElideMode getTextElideMode() const
	{
		return Qt::ElideLeft;
	}

	QString getTextValue()
	{
		QString value = QString::fromStdString(getVariantValue().cast<RBX::ContentId>().toString());
		if (value.isEmpty())
		{
			value = kUseEmbeddedSourceText;
		}
		return value;
	}

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option)
	{
		QComboBox *pComboBox = new QComboBox(parent);
		pComboBox->setGeometry(option.rect);
		pComboBox->setFrame(UpdateUIManager::Instance().getMainWindow().isRibbonStyle());
		pComboBox->setEditable(true);

		pComboBox->addItem(kUseEmbeddedSourceText);

		RobloxGameExplorer& rge = UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER);

		QString currentValue = text(1);
		bool foundCurrentValue = currentValue == kUseEmbeddedSourceText;

		std::vector<QString> scriptNames;
		rge.getListOfScripts(&scriptNames);
		for (size_t i = 0; i < scriptNames.size(); ++i)
		{
			RBX::ContentId contentId = RBX::ContentId::fromGameAssetName(scriptNames[i].toStdString());
			QString contentIdString = QString::fromStdString(contentId.toString());
			pComboBox->addItem(contentIdString);
			foundCurrentValue = foundCurrentValue ||
				(QString::fromStdString(contentId.toString()) == currentValue);
		}
		
		if (!foundCurrentValue)
		{
			pComboBox->addItem(currentValue);
		}

		return pComboBox; 
	}

	void setEditorData(QWidget *editor)
	{
		if (currentlyCommitting)
			return;

		QComboBox *pComboBox = dynamic_cast<QComboBox *>(editor);
		if (!pComboBox)
			return;

		//set current value in the combo
		int index = pComboBox->findText(text(1));
		RBXASSERT(index >= 0);
		pComboBox->setCurrentIndex(std::max(0, index));
	}
	
	void setModelData(QWidget *editor)
	{
		QComboBox *comboBox = dynamic_cast<QComboBox *>(editor);
		if (!comboBox)
			return;

		//apply values only if it has been changed
		QString currentText = comboBox->currentText();
		if (comboBox->currentText() != text(1))
		{
			if (currentText == kUseEmbeddedSourceText)
			{
				currentText = "";
			}
			RBX::ScopedAssign<bool> ignoreCommitModif(currentlyCommitting, true);
			setVariantValue(RBX::ContentId(currentText.toStdString()));
			commitModification();
		}
	}
};


//--------------------------------------------------------------------------------------------
// NumberSequencePropertyItem
//--------------------------------------------------------------------------------------------
class NumberSequencePropertyItem : public PropertyItem, public NumberSequencePropertyWidget::Callbacks
{
public:
    NumberSequencePropertyItem(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor)
        : PropertyItem(pPropertyDescriptor)
    {
    }

    ~NumberSequencePropertyItem()
    {
    }


    QString getTextValue()
    {
        RBX::NumberSequence val = m_variantValue.get<RBX::NumberSequence>();
        const std::vector< RBX::NumberSequenceKeypoint >& keys = val.getPoints();
        if( keys.size() == 2 && keys[0].value == keys[1].value && keys[0].envelope == 0 && keys[1].envelope == 0 )
            return QString("%1").arg(keys[0].value);

        return "<NumberSequence>";
    }

    bool editorEvent(QEvent* event)
    {
        return false;
    }

    //////////////////////////////////////////////////////////////////////////

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option)
    {
        NumberSequencePropertyWidget* w = new NumberSequencePropertyWidget(parent, this);
        w->setGeometry(option.rect);
        return w; 
    }

    void setEditorData(QWidget *editor)
    {
        NumberSequencePropertyWidget* w = static_cast<NumberSequencePropertyWidget*>(editor);
        QString text = getTextValue();
        w->textEditor->setText( (text[0] != '<' ? text : "") );
        w->textEditor->setFocus();
    }

    void setModelData(QWidget *editor)
    {
        NumberSequencePropertyWidget* w  = static_cast<NumberSequencePropertyWidget*>(editor);

        float constval;
        if( 1 != sscanf( w->textEditor->text().toStdString().c_str(), "%f", &constval ) )
            return;

        setVariantValue( RBX::NumberSequence(constval) );
        commitModification();
    }

    // These come from the editor widget

    void onCurveClicked()
    {
        InstanceList selected;
        getSelectedInstances(selected);
        if( selected.size() != 1) 
            return; // no multi-edit

        RBX::Instance* inst = *selected.begin();
        SplineEditorAdapter* adapter = new SplineEditorAdapter(m_pPropertyDescriptor, getTreeWidget()->getDataModel(), inst);
        adapter->show();
    }

    void onTextAccepted()
    {
    }


};

//--------------------------------------------------------------------------------------------
// NumberRangePropertyItem
//--------------------------------------------------------------------------------------------
class NumberRangePropertyItem : public PropertyItem
{
    DoublePropertyItem* m_pMin;
    DoublePropertyItem* m_pMax;
    int m_componentToUpdate;
public:
    NumberRangePropertyItem(const RBX::Reflection::PropertyDescriptor* pd) : PropertyItem(pd)
    {
        m_pMin = new DoublePropertyItem(this, "Min");
        m_pMax = new DoublePropertyItem(this, "Max");
        addChild(m_pMin);
        addChild(m_pMax);
    }

    int compareInstanceValues(RBX::Instance* prev, RBX::Instance* cur)
    {
        int result = 0;

        if (const RBX::Reflection::TypedPropertyDescriptor<RBX::NumberRange>* propd = static_cast<const RBX::Reflection::TypedPropertyDescriptor<RBX::NumberRange>* > ( m_pPropertyDescriptor ))
        {
             RBX::NumberRange a = propd->getValue(prev);
             RBX::NumberRange b = propd->getValue(cur);
             if (a.min != b.min)
             {
                 m_pMin->setText(1, "");
                 result = -1;
             }

             if (a.max != b.max)
             {
                 m_pMax->setText(1, "");
                 result = -1;
             }
        }
        return result;
    }

    void setVariantValue(const RBX::Reflection::Variant& val)
    {
        PropertyItem::setVariantValue(val);
        updateChildren();
    }

    QString getTextValue()
    {
        RBX::NumberRange r = m_variantValue.get<RBX::NumberRange>();
        if(r.min == r.max)
        {
            return format(r.min);
        }
        return QString("%1, %2").arg(format(r.min), format(r.max));
    }

    void syncChildren(PropertyItem *pChildItem)
    {			
        //get modified values
        float a = m_pMin->getVariantValue().get<double>();
        float b = m_pMax->getVariantValue().get<double>();
        RBX::NumberRange modifiedValue(std::min(a,b), std::max(a,b));

        if (!m_isMultiValued)
        {
            //set and commit modification
            setVariantValue(modifiedValue);		
            commitModification();

            //update other children also
            updateChildren();
        }
        else
        {
            //commit modifications
            commitModification();

            //remove parent's info
            setText(1, "");			
        }
    }

    void pushInstanceValue(RBX::Instance *pInstance)
    {
        PropertyItem::pushInstanceValue(pInstance);
        updateChildren();
    }

    void pullInstanceValue(RBX::Instance *pInstance)
    {
        //nothing special required in case there's no component modified
        PropertyItem::pullInstanceValue(pInstance);
        return;
    }

    void updateChildren()
    {
        //set appropriate values in children
        RBX::NumberRange val = m_variantValue.get<RBX::NumberRange>();
        m_pMin->setVariantValue( val.min );
        m_pMax->setVariantValue( val.max );
    }

    //////////////////////////////////////////////////////////////////////////

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option)
    {
        QLineEdit *pLineEdit = new QLineEdit(parent);
        pLineEdit->setGeometry(option.rect);
        pLineEdit->setFrame(false);
        return pLineEdit; 
    }

    void setEditorData(QWidget *editor)
    {
        QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
        if (!pLineEdit)
            return;

        pLineEdit->setText(text(1));
    }

    void setModelData(QWidget *editor)
    {
        QLineEdit *pLineEdit = dynamic_cast<QLineEdit *>(editor);
        if (!pLineEdit)
            return;

        //apply values only if it has been changed
        if (pLineEdit->text() != text(1))
        {
            std::string text = pLineEdit->text().toStdString();
            float min, max;
            if (2 != sscanf(text.c_str(), " %f , %f ", &min, &max))
            {
                if (1 != sscanf(text.c_str(), "%f", &min))
                    return;
                
                max = min;
            }
            setVariantValue(RBX::NumberRange(min, max));
            commitModification();

            if (!FFlag::StudioPropertyErrorOutput)
                updateChildren();
        }
        return;
    }


};

class ColorSequencePropertyItem : public PropertyItem
{
    ColorPropertyItem* m_start;
    ColorPropertyItem* m_end;
public:

    ColorSequencePropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor) : PropertyItem(pPropertyDescriptor)
    {
        m_start = new ColorPropertyItem(this, "Start");
        m_end   = new ColorPropertyItem(this, "End");
        addChild(m_start);
        addChild(m_end);

        this->setText(1, "<ColorSequence>");
    }

    QString getTextValue() { return "<ColorSequence>"; }

    virtual void setVariantValue(const RBX::Reflection::Variant &value)
    {
        m_variantValue = value;
        RBX::ColorSequence cs = value.get<RBX::ColorSequence>();

        m_start->setVariantValue(cs.start().value);
        m_end->setVariantValue(cs.end().value);

        m_start->updateIcon();
        m_end->updateIcon();
        this->updateIcon();

        RBX::Color3 a = m_start->getVariantValue().get<RBX::Color3>();
        RBX::Color3 b = m_end->getVariantValue().get<RBX::Color3>();

        if (a == b)
            this->setText(1, QString("[%1, %2, %3]").arg(int(a.r*255)).arg(int(a.g*255)).arg(int(a.b*255)) );
        else
            this->setText(1, QString("[%1, %2, %3] - [%4, %5, %6]").arg(int(a.r*255)).arg(int(a.g*255)).arg(int(a.b*255)).arg(int(b.r*255)).arg(int(b.g*255)).arg(int(b.b*255)) ); // hell yeah
    }

    void syncChildren(PropertyItem *pChildItem)
    {
        RBX::Color3 a = m_start->getVariantValue().get<RBX::Color3>();
        RBX::Color3 b = m_end->getVariantValue().get<RBX::Color3>();

        setVariantValue( RBX::ColorSequence(a,b) );
        commitModification(true);
    }

    void updateIcon()
    {
        RBX::ColorSequence cs = getVariantValue().get<RBX::ColorSequence>();
        RBX::Color3 a = cs.start().value;
        RBX::Color3 b = cs.end().value;

        const int H = 12;
        const int W = 12;

        QImage img(W, H, QImage::Format_ARGB32_Premultiplied);
        img.fill(0);

        QPainter painter(&img);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        for (int x=0; x<W; ++x) // pretty lame, but this is surely temporary-ish
        {
            RBX::Color3 c = a.lerp(b, x/float(W-1) );
            QColor qc(c.r * 255, c.g*255, c.b*255, 255);
            painter.setPen(qc);
            painter.drawLine(QPoint(x,0), QPoint(x,H));
        }
        painter.end();

        this->setIcon(1, QPixmap::fromImage(img));
    }
};

//--------------------------------------------------------------------------------------------
// PropertyItem
//--------------------------------------------------------------------------------------------
PropertyItem::PropertyItem(const RBX::Reflection::PropertyDescriptor *pPropertyDescriptor)
: m_pPropertyDescriptor(pPropertyDescriptor)
, m_pParentItem(NULL)
, m_isMultiValued(false)
, m_hasExceptionError(false)
{
	init(m_pPropertyDescriptor->name.c_str());
	
	//add descriptor and it's derived classes
	addClassDescriptor(&m_pPropertyDescriptor->owner);
}

PropertyItem::PropertyItem(PropertyItem *pParentItem, const QString &itemName)
: m_pPropertyDescriptor(NULL)
, m_pParentItem(pParentItem)
, m_isMultiValued(false)
, m_hasExceptionError(false)
{
	init(itemName);
}

PropertyItem::~PropertyItem()
{
}

void PropertyItem::init(const QString &itemName)
{
	// by default items are hidden
	setHidden(true);
	setText(0, itemName);
    setToolTip(0, itemName);    // in case the name is cut off
	
	Qt::ItemFlags flag = Qt::NoItemFlags;
	if (m_pPropertyDescriptor) 
	{
		if (!m_pPropertyDescriptor->isReadOnly())
			flag = flags()|Qt::ItemIsEditable;

        RBX::Reflection::Metadata::Item* metadata = RBX::Reflection::Metadata::Reflection::singleton()->get(*m_pPropertyDescriptor);
        if ( metadata && !metadata->description.empty() )
			setToolTip(0,QtUtilities::wrapText(QString::fromStdString(metadata->description).replace(QRegExp("<a href.*(\\/a>|\\/>)"), ""),80));
	}
	else if (m_pParentItem)
	{
		flag = m_pParentItem->flags();
	}
		
	setFlags(flag);
}

PropertyItem* PropertyItem::createItem(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor)
{
	const RBX::Reflection::TypedPropertyDescriptor<bool>* pBoolProp = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<bool>*>(pPropertyDescriptor);
	if (pBoolProp)
		return new BoolPropertyItem(pPropertyDescriptor);

	const RBX::Reflection::TypedPropertyDescriptor<int>* pIntProp = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<int>*>(pPropertyDescriptor);
	if (pIntProp)
		return new IntPropertyItem(pPropertyDescriptor);
			
	const RBX::Reflection::TypedPropertyDescriptor<float>* pFloatProp = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<float>*>(pPropertyDescriptor);
	if (pFloatProp)
		return new DoublePropertyItem(pPropertyDescriptor);

	if (const RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>* pBrickColorProp = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>*>(pPropertyDescriptor))
	if (pBrickColorProp)
		return new BrickColorPropertyItem(pPropertyDescriptor);

	const RBX::Reflection::TypedPropertyDescriptor<G3D::Color3>* pColorProp = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<G3D::Color3>*>(pPropertyDescriptor);
	if (pColorProp)
		return new ColorPropertyItem(pPropertyDescriptor);
			
	const RBX::Reflection::EnumPropertyDescriptor* pEnumProp = dynamic_cast<const RBX::Reflection::EnumPropertyDescriptor*>(pPropertyDescriptor);
	if (pEnumProp)
		return new EnumPropertyItem(pPropertyDescriptor);

	const RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>* pVector3Prop = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>*>(pPropertyDescriptor);
	if (pVector3Prop)
		return new VectorPropertyItem(pPropertyDescriptor);	

	const RBX::Reflection::TypedPropertyDescriptor<RBX::PhysicalProperties>* pPhysicalPropertiesProp = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<RBX::PhysicalProperties>*>(pPropertyDescriptor);
	if (pPhysicalPropertiesProp)
		return new PhysicalPropertiesItem(pPropertyDescriptor);

	const RBX::Reflection::RefPropertyDescriptor* pRefProp = dynamic_cast<const RBX::Reflection::RefPropertyDescriptor*>(pPropertyDescriptor);
	if (pRefProp)
	{
		if (strcmp(pPropertyDescriptor->name.c_str(), "PrimaryPart") == 0)
			return new PrimaryPartPropertyItem(pPropertyDescriptor);
		return new RefPropertyItem(pPropertyDescriptor);
	}

	const RBX::Reflection::TypedPropertyDescriptor<RBX::Faces>* pFacesProp = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<RBX::Faces>*>(pPropertyDescriptor);
	if (pFacesProp)
		return new FacesPropertyItem(pPropertyDescriptor);	

	const RBX::Reflection::TypedPropertyDescriptor<RBX::UDim2>* pUdim2Prop = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<RBX::UDim2>*>(pPropertyDescriptor);
	if (pUdim2Prop)	
		return new UDim2PropertyItem(pPropertyDescriptor);

	const RBX::Reflection::TypedPropertyDescriptor<RBX::MeshId>* pMeshProp = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<RBX::MeshId>*>(pPropertyDescriptor);
	if (pMeshProp)
		return new ContentPropertyItem(pPropertyDescriptor);

	const RBX::Reflection::TypedPropertyDescriptor<RBX::TextureId>* pTextureProp = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<RBX::TextureId>*>(pPropertyDescriptor);
	if (pTextureProp)
		return new ImageAssetPropertyItem(pPropertyDescriptor);
		
	const RBX::Reflection::TypedPropertyDescriptor<RBX::Soundscape::SoundId>* pSoundProp = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<RBX::Soundscape::SoundId>*>(pPropertyDescriptor);
	if (pSoundProp)		
		return new ContentPropertyItem(pPropertyDescriptor);
	
	if (FFlag::AnimationIdIsContentPropertyItem)
	{
		const RBX::Reflection::TypedPropertyDescriptor<RBX::AnimationId>* pAnimationProp = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<RBX::AnimationId>*>(pPropertyDescriptor);
		if (pAnimationProp)		
			return new ContentPropertyItem(pPropertyDescriptor);
	}
	
	const RBX::Reflection::TypedPropertyDescriptor<QFont>* pFontProp = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<QFont>*>(pPropertyDescriptor);
	if ( pFontProp )
		return new FontPropertyItem(pPropertyDescriptor);

    const RBX::Reflection::TypedPropertyDescriptor<QDir>* pDirProp = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<QDir>*>(pPropertyDescriptor);
 	if ( pDirProp )
		return new DirPropertyItem(pPropertyDescriptor);

    const RBX::Reflection::TypedPropertyDescriptor<RBX::NumberSequence>* pNSProp = dynamic_cast< const RBX::Reflection::TypedPropertyDescriptor<RBX::NumberSequence>*>(pPropertyDescriptor);
    if( pNSProp )
        return new NumberSequencePropertyItem(pPropertyDescriptor);

    const RBX::Reflection::TypedPropertyDescriptor<RBX::NumberRange>* pNRProp = dynamic_cast< const RBX::Reflection::TypedPropertyDescriptor<RBX::NumberRange>*>(pPropertyDescriptor);
    if( pNRProp )
        return new NumberRangePropertyItem(pPropertyDescriptor);

    const RBX::Reflection::TypedPropertyDescriptor<RBX::ColorSequence>* pCSProp = dynamic_cast< const RBX::Reflection::TypedPropertyDescriptor<RBX::ColorSequence>*>(pPropertyDescriptor);
    if( pCSProp )
        return new ColorSequencePropertyItem(pPropertyDescriptor);

	const RBX::Reflection::TypedPropertyDescriptor<RBX::ContentId>* pContentProp = dynamic_cast<const RBX::Reflection::TypedPropertyDescriptor<RBX::ContentId>*>(pPropertyDescriptor);
	if (pContentProp)
		return new ScriptAssetPropertyItem(pPropertyDescriptor);

	const RBX::Reflection::TypedPropertyDescriptor<RBX::Rect2D>* pRectProp = dynamic_cast< const RBX::Reflection::TypedPropertyDescriptor<RBX::Rect2D>*>(pPropertyDescriptor);
	if( pRectProp )
		return new RectPropertyItem(pPropertyDescriptor);

	if (pPropertyDescriptor->hasStringValue())
		return new StringPropertyItem(pPropertyDescriptor);

	return NULL;
}

bool materialPropertiesIsEnabled(const InstanceList &instances)
{
	for (InstanceList::const_iterator curIter = instances.begin(); curIter != instances.end(); curIter ++)
	{
		if (RBX::PartInstance* part = (*curIter)->fastDynamicCast<RBX::PartInstance>())
		{
			if (RBX::Workspace* currentWorkspace = RBX::Workspace::findWorkspace(part))
			{
				return currentWorkspace->getUsingNewPhysicalProperties();
			}

		}
	}
	return false;
}

bool PropertyItem::currentlyHidden(const InstanceList &instances)
{
	// Use this function to hide properties Dynamically

	// CSG PHYSICS QUALITY - FFlagCSGPhysicsLevelOfDetailEnabled
	if (!FFlag::CSGPhysicsLevelOfDetailEnabled && (*propertyDescriptor() == RBX::PartOperation::prop_CollisionFidelity))
	{
		// Hide property if flag is disabled
		return true;
	}

	// MATERIAL PROPERTIES - DFFlagMaterialPropertiesEnabled
	// Property Hiding Logic
	if (!DFFlag::MaterialPropertiesEnabled && (*propertyDescriptor() == RBX::Workspace::prop_physicalPropertiesMode))
	{
		// Hide workspace flag if main flag is disabled.
		return true;
	}

	if (DFFlag::MaterialPropertiesEnabled && materialPropertiesIsEnabled(instances))
	{
		if (*propertyDescriptor() == RBX::PartInstance::prop_Friction ||
			*propertyDescriptor() == RBX::PartInstance::prop_Elasticity)
		{
			// Hide old properties if Workspace Flag is true.
			return true;
		}
	}
	else
	{
		if (*propertyDescriptor() == RBX::PartInstance::prop_CustomPhysicalProperties)
		{
			// Hide new property if Workspace Flag is false.
			return true;
		}
	}
	// MATERIAL PROPERTIES done

	return false;
}

bool PropertyItem::update()
{
	if (!m_pPropertyDescriptor)
		return false;

	m_isMultiValued = false;
	if (m_hasExceptionError)	
		setToolTip(1, "");
	m_hasExceptionError = false;
	bool hasValue = false;
	
	ClassDescriptorInstanceMap& selectionData = static_cast<PropertyTreeWidget*>(treeWidget())->getSelectionData();
	if (!selectionData.empty())
	{
		Classes::const_iterator curClassIter = m_Classes.begin();
		Classes::const_iterator endClassIter = m_Classes.end();

		InstanceList::const_iterator previousIter;

		while (curClassIter != endClassIter)
		{
			ClassDescriptorInstanceMap::const_iterator classDescIter = selectionData.find(*curClassIter);
			if (classDescIter != selectionData.end())
			{
				if (currentlyHidden(classDescIter->second))
				{
					setHidden(true);
					return true;
				}
				InstanceList::const_iterator curIter = classDescIter->second.begin(), endIter = classDescIter->second.end();
				while (curIter != endIter)
				{
					if (!hasValue)
					{
						previousIter = curIter;

						try 
						{
							pushInstanceValue(*curIter);
						}
						catch (std::exception& e)
						{
							setTextValue(""); // remove any previously set value
							m_hasExceptionError = true;
							setToolTip(1, QString(e.what()));
						}
						
						hasValue = true;
					}
					else 
					{
						int result = 0;

						try
						{
							result = compareInstanceValues(*previousIter, *curIter);
						}
						catch (std::exception& e)
						{
							m_hasExceptionError = true;
							setToolTip(1, QString(e.what()));
						}

						if (result <= 0)
						{
							//remove the value shown in the item
							setTextValue("");
							m_isMultiValued = true;

							if (!result)
							{
								setHidden(false);
								return true;
							}
						}	
						previousIter = curIter;
					}
					++curIter;
				}//while (curIter != endIter)
			}//if (classDescIter != selectionData.end())
			++curClassIter;
		}//while (curClassIter != endClassIter)
	}

	setHidden(!hasValue);

	return hasValue;
}

bool PropertyItem::applyFilter(const QString &filterString)
{
	bool filterStringFound = filterString.isEmpty() || text(0).contains(filterString, Qt::CaseInsensitive);
	bool itemShown = filterStringFound && canBeShown();
	setHidden(!itemShown);	
	return itemShown;
}

bool PropertyItem::canBeShown()
{	
	ClassDescriptorInstanceMap& selectionData = static_cast<PropertyTreeWidget*>(treeWidget())->getSelectionData();
	if (selectionData.empty())
		return false;
	
	bool isDescriptorAvailable = false;

	Classes::const_iterator curIter = m_Classes.begin();
	Classes::const_iterator endIter = m_Classes.end();

	while (curIter != endIter)
	{
		//if one of the supported classes exists in the selected instances then we can show this item
		ClassDescriptorInstanceMap::const_iterator classDescIter = selectionData.find(*curIter);
		if (classDescIter != selectionData.end())
		{
			isDescriptorAvailable = true;
			break;
		}
		++curIter;
	}
	
	return isDescriptorAvailable;
}

void PropertyItem::setTextValue(const QString &value)
{	setText(1, value); }

QString PropertyItem::getTextValue()
{	return QString(""); }

int PropertyItem::compareInstanceValues(RBX::Instance *pPreviousInstance, RBX::Instance *pCurrentInstance)
{
	if (!m_pPropertyDescriptor)
		return 0;

	return m_pPropertyDescriptor->equalValues(pPreviousInstance, pCurrentInstance);
}

void PropertyItem::pushInstanceValue(RBX::Instance *pInstance) 
{ 
	if (!pInstance || !m_pPropertyDescriptor)
		return;

	//set variant value
	RBX::Reflection::Variant value;
	m_pPropertyDescriptor->getVariant(pInstance, value);	
	setVariantValue(value);
}

void PropertyItem::pullInstanceValue(RBX::Instance *pInstance)
{
	if (!m_pPropertyDescriptor || !pInstance)
		return;

    if (!handleCollisionLocally(pInstance))
        m_pPropertyDescriptor->setVariant(pInstance, m_variantValue);
}

void PropertyItem::setVariantValue(const RBX::Reflection::Variant &value)
{
	m_variantValue = value;
	//also set its text value
	setTextValue(getTextValue());
}

RBX::Reflection::Variant PropertyItem::getVariantValue()
{ return m_variantValue; }

void PropertyItem::addClassDescriptor(const RBX::Reflection::ClassDescriptor* pClassDescriptor)
{
	if (!m_pPropertyDescriptor)
		return;

	m_Classes.push_back(pClassDescriptor);

	// Recursively add each slot of derived classes
	std::for_each(pClassDescriptor->derivedClasses_begin(), 
		          pClassDescriptor->derivedClasses_end(), 
				  boost::bind(&PropertyItem::addClassDescriptor, this, _1));
}

void PropertyItem::getSelectedInstances(InstanceList &instances)
{
	ClassDescriptorInstanceMap& selectionData = static_cast<PropertyTreeWidget*>(treeWidget())->getSelectionData();
	if (!selectionData.empty())
	{
		Classes::const_iterator curClassIter = m_Classes.begin();
		Classes::const_iterator endClassIter = m_Classes.end();

		while (curClassIter != endClassIter)
		{
			ClassDescriptorInstanceMap::const_iterator classDescIter = selectionData.find(*curClassIter);
			if (classDescIter != selectionData.end())
				instances.insert(instances.end(), classDescIter->second.begin(), classDescIter->second.end());
			++curClassIter;
		}
	}
}

void PropertyItem::commitModification(bool requestWaypoint)
{	
	try 
	{
		if (m_pPropertyDescriptor)
		{		
			//get selected instances and set value appropriately
			InstanceList instancesToModify;
			getSelectedInstances(instancesToModify);

			boost::shared_ptr<RBX::DataModel> pDataModel = getTreeWidget()->getDataModel();
			if ( pDataModel.get() )
			{
				RBX::DataModel::LegacyLock lock(pDataModel, RBX::DataModelJob::Write);

				//now modify values
				for (InstanceList::iterator iter = instancesToModify.begin(); iter != instancesToModify.end(); ++iter)
					pullInstanceValue(*iter);			
				
				//for undo/redo
				if (requestWaypoint)
					RBX::ChangeHistoryService::requestWaypoint(m_pPropertyDescriptor->name.c_str(), pDataModel.get());

				pDataModel->setDirty(true);
			}
			else
			{
				//now modify values
				for (InstanceList::iterator iter = instancesToModify.begin(); iter != instancesToModify.end(); ++iter)
					pullInstanceValue(*iter);			
			}
			
				/*
					some setters clamp the value within certain bounds
					if the clamped value is equal to the current value, the changed event won't fire
					this is necessary to update the value in property widget
				*/
			for (InstanceList::iterator iter = instancesToModify.begin(); iter != instancesToModify.end(); ++iter)
				pushInstanceValue(*iter);
			}

		if (m_pParentItem)
			m_pParentItem->syncChildren(this);	
	}
	
	catch (std::exception& e)
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
	}
}

QPoint PropertyItem::computePopupLocation(const QSize &pickerFrameSize)
{
	QPoint location;

	QRect rect = getTreeWidget()->visualItemRect(this);		
	QRect screen = QApplication::desktop()->availableGeometry(getTreeWidget());	

	if (getTreeWidget()->mapToGlobal(QPoint(0, rect.bottom())).y() + pickerFrameSize.height() <= screen.height()) 
		location = getTreeWidget()->mapToGlobal(rect.bottomLeft()); 
	else 
		location = getTreeWidget()->mapToGlobal(rect.topLeft() - QPoint(0, pickerFrameSize.height()));

	location.rx() = qMax(screen.left(), qMin(location.x(), screen.right() - pickerFrameSize.width())) - 18;
	location.ry() += 24;

	return location;
}

PropertyTreeWidget* PropertyItem::getTreeWidget()
{	return static_cast<PropertyTreeWidget*>(treeWidget()); }

bool PropertyItem::handleCollisionLocally(RBX::Instance* pInstance)
{
    if (!RBX::AdvArrowTool::advCollisionCheckMode && 
        m_variantValue.isType<RBX::Vector3>())
    {
        if (RBX::PartInstance* partInstance = pInstance->fastDynamicCast<RBX::PartInstance>())
        {
            QString propertyName = m_pPropertyDescriptor->name.c_str();
            if (propertyName == "Position")
            {
                RBX::Vector3 position = m_variantValue.get<RBX::Vector3>();
                G3D::CoordinateFrame cFrame(partInstance->getCoordinateFrame().rotation, position);
                partInstance->setCoordinateFrame(cFrame);
                return true;
            }
            else if (propertyName == "Rotation")
            {
                RBX::Vector3 rotationVector = m_variantValue.get<RBX::Vector3>();
                G3D::Matrix3 rotation = G3D::Matrix3::fromEulerAnglesXYZ(RBX::Math::degreesToRadians(rotationVector.x),RBX::Math::degreesToRadians(rotationVector.y),RBX::Math::degreesToRadians(rotationVector.z));
                G3D::CoordinateFrame cFrame(rotation, partInstance->getCoordinateFrame().translation);
                partInstance->setCoordinateFrame(cFrame);
                return true;
            }
        }
    }
    return false;
}

static QIcon getCheckBox(int value, bool readOnly)
{
    QPixmap pixmap;
    if (!QPixmapCache::find(QString("CheckBoxState%1-%2").arg(value).arg(readOnly),&pixmap))
    {
        QStyleOptionButton opt;
        if (value < 0)
            opt.state |= QStyle::State_NoChange;
        else
            opt.state |= value ? QStyle::State_On : QStyle::State_Off;
        opt.state |= readOnly ? QStyle::State_ReadOnly : QStyle::State_Enabled;
        const QStyle *style = QApplication::style();
        const int indicatorWidth = style->pixelMetric(QStyle::PM_IndicatorWidth, &opt);
        const int indicatorHeight = style->pixelMetric(QStyle::PM_IndicatorHeight, &opt);
        const int listViewIconSize = indicatorWidth;
        const int pixmapWidth = indicatorWidth;
        const int pixmapHeight = qMax(indicatorHeight, listViewIconSize);

        opt.rect = QRect(0, 0, indicatorWidth, indicatorHeight);
        pixmap = QPixmap(pixmapWidth, pixmapHeight);
        pixmap.fill(Qt::transparent);
        {
            const int xoff = (pixmapWidth  > indicatorWidth)  ? (pixmapWidth  - indicatorWidth)  / 2 : 0;
            const int yoff = (pixmapHeight > indicatorHeight) ? (pixmapHeight - indicatorHeight) / 2 : 0;
            QPainter painter(&pixmap);
            painter.translate(xoff, yoff);
            style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &opt, &painter);
        }
        
        QPixmapCache::insert(QString("CheckBoxState%1-%2").arg(value).arg(readOnly), pixmap);
    }

    return QIcon(pixmap);
}

static QIcon getColorIcon(const QColor &color)
{
    QImage img(12, 12, QImage::Format_ARGB32_Premultiplied);
    img.fill(0);

    QPainter painter(&img);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.setPen(QPen(Qt::black, 1));
    painter.drawRect(0, 0, img.width()-1, img.height()-1);
    painter.fillRect(1, 1, img.width()-2, img.height()-2, color);
    painter.end();

    return QPixmap::fromImage(img);
}

void trimTrailing0s(QString& s)
{
    int decimalIndex = s.indexOf(".");
    if ( decimalIndex == -1 )
        return;

    // remove trailing zeros
    while ( s.endsWith("0") )
        s.chop(1);

    // check if we don't have any decimal places at all
    if ( s.endsWith(".") )
        s.chop(1);
}

QString format(double f)
{
    QString s;
	s.sprintf("%.3f", f);
	trimTrailing0s(s);
	return s;
}
