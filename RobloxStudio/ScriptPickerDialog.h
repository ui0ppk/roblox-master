#pragma once

#include <QObject>
#include <QString>

#include <vector>

class QDialog;
class QLineEdit;
class QListWidgetItem;
class QLabel;
class QPushButton;

class ScriptPickerDialog : public QObject
{
	Q_OBJECT
public:
	enum CompletedState
	{
		Abandoned,
		Completed
	};

	void runModal(QWidget* parent, QString suggestedName, CompletedState* state, QString* newName);

private:
	QDialog* dialog;
	QLineEdit* nameEdit;
	
	CompletedState* picked;
	QString* newName;
	std::vector<QString> usedNames;

private Q_SLOTS:
	void acceptLineEditValue();
};
