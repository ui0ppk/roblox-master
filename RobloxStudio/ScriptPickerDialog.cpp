#include "stdafx.h"

#include "ScriptPickerDialog.h"

#include "RobloxGameExplorer.h"
#include "RobloxMainWindow.h"
#include "UpdateUIManager.h"

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QPushbutton>

void ScriptPickerDialog::runModal(QWidget* parent, QString suggestedName, CompletedState* state, QString* newName)
{
	picked = state;
	this->newName = newName;

	*picked = Abandoned;

	UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER)
		.getListOfScripts(&usedNames);

	dialog = new QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
	dialog->setWindowTitle("Create new LinkedSource");
	QGridLayout* topLayout = new QGridLayout(dialog);

	QLabel* otherLabel = new QLabel(dialog);
	otherLabel->setText("Name:");
	topLayout->addWidget(otherLabel, 0, 0);
	nameEdit = new QLineEdit(dialog);
	nameEdit->setText(suggestedName);
	topLayout->addWidget(nameEdit, 0, 1);

	nameEdit->setFocus();

	QPushButton* pb = new QPushButton(dialog);
	pb->setText("Create");
	topLayout->addWidget(pb, 1, 0, 1, 2);
	
	QLabel* descriptionLabel = new QLabel(dialog);
	descriptionLabel->setText("LinkedSources can be referenced by scripts in your game, "
		"allowing you to update all of them at the same time.");
	descriptionLabel->setWordWrap(true);
	topLayout->addWidget(descriptionLabel, 2, 0, 1, 2);

	QObject::connect(nameEdit, SIGNAL(returnPressed()),
			         this,     SLOT  (acceptLineEditValue()));
	QObject::connect(pb,   SIGNAL(clicked()),
			         this, SLOT  (acceptLineEditValue()));

	dialog->setLayout(topLayout);
	dialog->move(UpdateUIManager::Instance().getMainWindow().geometry().center());
	dialog->exec();
}

void ScriptPickerDialog::acceptLineEditValue()
{
	bool valid = true;
	QString customName = nameEdit->text().trimmed();
	QString errorMessage;

	if (customName.isEmpty())
	{
		valid = false;
		errorMessage = "Name cannot be empty";
	}

	if (customName.contains(QChar('\\')) || customName.contains(QChar('/')))
	{
		valid = false;
		errorMessage = "Name cannot include '\\' or '/'";
	}

	customName = QString("Scripts/") + customName;
	for (std::vector<QString>::const_iterator itr = usedNames.begin(); itr != usedNames.end(); ++itr)
	{
		if (customName.compare(*itr, Qt::CaseInsensitive) == 0)
		{
			valid = false;
			errorMessage = "Name already taken (not case sensitive)";
			break;
		}
	}

	if (valid)
	{
		*picked = Completed;
		*newName = customName;
		dialog->accept();
	}
	else
	{
		QMessageBox mb(dialog);
		mb.setWindowTitle("Error");
		mb.setText(errorMessage);
		mb.exec();
	}
}
