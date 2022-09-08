/**
 * KeepSavedChangesDialog.cpp
 * Copyright (c) 2014 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "KeepSavedChangesDialog.h"

#include "AuthenticationHelper.h"
#include "RobloxDocManager.h"
#include "RobloxStudioVerbs.h"
#include "RobloxIDEDoc.h"
#include "V8DataModel/DataModel.h"
#include "RobloxMainWindow.h"
#include "Util/RobloxGoogleAnalytics.h"

// Qt Headers
#include <QMessageBox>

KeepSavedChangesDialog::KeepSavedChangesDialog(QWidget* parent)
    : QMessageBox(parent)
    , dialogResult(Cancel)
{
    setAttribute(Qt::WA_DeleteOnClose,false);
    setIcon(QMessageBox::Question);
    QPushButton* button = addButton("Save Online", QMessageBox::AcceptRole);
    button->setDefault(true);
    addButton("Discard", QMessageBox::DestructiveRole);
    addButton("Cancel", QMessageBox::RejectRole);
    setText("Would you like to save these changes?");

    connect(this, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(onClicked(QAbstractButton*)));
}

KeepSavedChangesDialog::~KeepSavedChangesDialog()
{
}

void KeepSavedChangesDialog::onClicked(QAbstractButton* button)
{
    if (buttonRole(button) == QMessageBox::AcceptRole)
        onSave();
    else if (buttonRole(button) == QMessageBox::DestructiveRole)
        onDiscard();
    else
        onCancel();
}

void KeepSavedChangesDialog::onSave()
{
    if (!AuthenticationHelper::Instance().verifyUserAndAuthenticate() ||
        !AuthenticationHelper::validateMachine())
    {
        dialogResult = Fail;
        close();
        return;
    }

    try
    {
        RobloxIDEDoc* pIDEDoc = RobloxDocManager::Instance().getPlayDoc();
        if (!pIDEDoc)
        {
            dialogResult = Fail;
            close();
            return;
        }

		// need to save with edit datamodel
		shared_ptr<RBX::DataModel> dataModel = pIDEDoc->getEditDataModel();
		PublishToRobloxVerb* pPublishVerb = dataModel ? dynamic_cast<PublishToRobloxVerb*>(dataModel->getVerb("PublishToRobloxVerb")) : NULL;

		if (!pPublishVerb)
		{
			dialogResult = Fail;
			close();
			return;
		}

		pPublishVerb->doIt(dataModel.get());
    }
    catch (std::exception&)
    {
        RobloxMainWindow::sendCounterEvent("QTStudioPublishSaveFailure");
        RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "StudioPublishSaveFailure");
        dialogResult = Fail;
    }
    dialogResult = Save;
    close();
}

void KeepSavedChangesDialog::onDiscard()
{
    dialogResult = Discard;
    close();
}

void KeepSavedChangesDialog::onCancel()
{
    dialogResult = Cancel;
    close();
}


