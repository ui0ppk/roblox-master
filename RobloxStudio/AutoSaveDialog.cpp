/**
 * AutoSaveDialog.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "AutoSaveDialog.h"

// Qt Headers
#include <QSettings>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

// Roblox Studio Headers
#include "AuthoringSettings.h"
#include "RobloxDocManager.h"
#include "UpdateUIManager.h"
#include "RobloxMainWindow.h"
#include "RobloxIDEDoc.h"
#include "RobloxApplicationManager.h"

LOGVARIABLE(AutoSave, 0)

AutoSaveDialog::AutoSaveDialog(QWidget* Parent)
    : QDialog(Parent,Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
    setAttribute(Qt::WA_DeleteOnClose,false);

    m_UI.setupUi(this);

#ifdef Q_WS_WIN32

    QString text = tr(
        "<font size=4><b>An auto-save recovery file was detected!</b></font><br><br>"
        "This means that Roblox Studio was not shut down properly and <u><i>may not have saved your data</i></u>!<br><br>"
        "Do you want to load the auto-save recovery file?<br>"
        "<ul>"
        "    <li>"
        "        <font size=4><b><i>Open</i></b></font> will open the file.  You can choose which file to load if there is more than one "
        "            recovery file.  Once an auto-save is loaded, it will not be deleted until the file is re-saved "
        "            with a different filename.<br>"
        "    </li>"
        "    <li>"
        "        <font size=4><b><i>Ignore</i></b></font> will continue and leave the files alone.  However, the next time ROBLOX Studio "
        "            is run, this message box will pop up again.<br>"
        "    </li>"
        "    <li>"
        "        <font size=4><b><i>Delete</i></b></font> will ask to delete all auto-save files.<br>"
        "    </li>"
        "</ul>"
        "<br><br>" );

#else

    QString text = tr(
        "<font size=4><font size=6><b>An auto-save recovery file was detected!</b></font><br><br>"
        "This means that Roblox Studio was not shut down properly and <u><i>may not have saved your data</i></u>!<br><br>"
        "Do you want to load the auto-save recovery file?<br>"
        "<ul>"
        "    <li>"
        "        <font size=5><b><i>Open</i></b></font> will open the file.  You can choose which file to load if there is more than one "
        "            recovery file.  Once an auto-save is loaded, it will not be deleted until the file is re-saved "
        "            with a different filename.<br>"
        "    </li>"
        "    <li>"
        "        <font size=5><b><i>Ignore</i></b></font> will continue and leave the files alone.  However, the next time ROBLOX Studio "
        "            is run, this message box will pop up again.<br>"
        "    </li>"
        "    <li>"
        "        <font size=5><b><i>Delete</i></b></font> will ask to delete all auto-save files.<br>"
        "    </li>"
        "</ul>"
        "<br><br></font>" );

#endif

    m_UI.label->setText(text);

    connect(m_UI.openButton,SIGNAL(clicked()),this,SLOT(onOpen()));
    connect(m_UI.ignoreButton,SIGNAL(clicked()),this,SLOT(onIgnore()));
    connect(m_UI.deleteButton,SIGNAL(clicked()),this,SLOT(onDelete()));
}

AutoSaveDialog::~AutoSaveDialog()
{
}

/**
 * Checks to see if there are any auto-save files.
 */
bool AutoSaveDialog::checkForAutoSaveFiles()
{
    // don't show if we've loaded a place
    if ( RobloxDocManager::Instance().getPlayDoc() )
        return false;

    // don't check for auto-saves if there are other instances running
    if ( RobloxApplicationManager::instance().getApplicationCount() != 1 )
         return false;

    return !getFileList().empty();
}

/**
 * Gets the list of auto-save files in the auto-save directory.
 *  Filters out files that do not have "_autosave_" in their name.
 */
QStringList AutoSaveDialog::getFileList()
{
    QStringList list;

	FASTLOGS(FLog::AutoSave, "Getting list of AutoSave files in dir %s", AuthoringSettings::singleton().autoSaveDir.absolutePath().toStdString());

    const QDir dir(AuthoringSettings::singleton().autoSaveDir);
    const QStringList entries = dir.entryList(QStringList() << "*.rbxl");
    if ( entries.empty() )
        return list;

    for ( int i = 0 ; i < entries.size() ; i++ )
	{
		FASTLOGS(FLog::AutoSave, "Found file %s", entries[i].toStdString());
        if ( entries[i].contains("_AUTOSAVE_",Qt::CaseInsensitive) )
		{
			FASTLOG(FLog::AutoSave, "Pattern matches, add to list");
            list.append(entries[i]);
		}
	}

    return list;
}

/**
 * Callback for closing the dialog.
 *  Calls onIgnore().
 */
void AutoSaveDialog::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);
    onIgnore();
}

/**
 * Callback for user clicking the open button.
 *  Attempts to open the file if there is just one file or pops a file open
 *  dialog if there are more.  Failing to open the file will delete the file.
 */
void AutoSaveDialog::onOpen()
{
    const QDir dir(AuthoringSettings::singleton().autoSaveDir);
    const QStringList entries = getFileList();

    QString fileName;

    if ( entries.size() == 1 )
    {
        // if only one file, try to load it
        fileName = dir.filePath(entries[0]);
    }
    else
    {
		fileName = QFileDialog::getOpenFileName(
			this, 
			tr("Open Roblox File"), 
			dir.absolutePath(), 
			tr("Roblox Places Files (*.rbxl)"));
    }

	FASTLOGS(FLog::AutoSave, "AutoSave open: file %s", fileName.toStdString());

    if ( !fileName.isEmpty() )
    {
        accept();

        bool openResult = UpdateUIManager::Instance().getMainWindow().handleFileOpen(fileName,IRobloxDoc::IDE);
        if ( openResult )
        {
			FASTLOG(FLog::AutoSave, "File opened successfully");
            RobloxDocManager::Instance().getPlayDoc()->setAutoSaveLoad();
        }
        else
        {
			FASTLOG(FLog::AutoSave, "Bad file, remove");
            // kill the bad file, it's useless to us
            QFile::remove(fileName);
            if ( !getFileList().empty() )
                reject();
        }
    }
}

/**
 * Callback for user clicking the ignore button.
 *  Exits the dialog.
 */
void AutoSaveDialog::onIgnore()
{
	FASTLOG(FLog::AutoSave, "AutoSave ignore");
    accept();
}

/**
 * Callback for user clicking the delete button.
 *  Deletes all files after confirmation.
 */
void AutoSaveDialog::onDelete()
{
	FASTLOG(FLog::AutoSave, "AutoSave delete");
    const QDir dir(AuthoringSettings::singleton().autoSaveDir);
    const QStringList entries = getFileList();
    const QString confirmMessage = 
        QString(tr("Do you want to delete all auto-save files?  There are %1 file(s).")).
        arg(entries.size());
    const int confirmationResult = QMessageBox::question(
        this,
        tr("Confirm Auto-Save Delete"),
        confirmMessage,
        QMessageBox::Yes | QMessageBox::No );

    if ( confirmationResult == QMessageBox::Yes )
    {
        for ( int i = 0 ; i < entries.size() ; ++i )
		{
			FASTLOGS(FLog::AutoSave, "Removing %s", entries[i].toStdString());
            QFile::remove(dir.filePath(entries[i]));
		}
        accept();
    }
}
