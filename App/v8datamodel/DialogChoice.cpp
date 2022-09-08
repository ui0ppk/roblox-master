#include "stdafx.h"

#include "V8DataModel/DialogChoice.h"
#include "V8DataModel/DialogRoot.h"

DYNAMIC_FASTFLAGVARIABLE(FilteringEnabledDialogFix, false);

namespace RBX
{

const char* const sDialogChoice= "DialogChoice";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<DialogChoice, std::string> prop_UserDialog("UserDialog", category_Data, &DialogChoice::getUserDialog, &DialogChoice::setUserDialog);
static Reflection::PropDescriptor<DialogChoice, std::string> prop_ResponseDialog("ResponseDialog", category_Data, &DialogChoice::getResponseDialog, &DialogChoice::setResponseDialog);
static Reflection::PropDescriptor<DialogChoice, std::string> prop_GoodbyeDialog("GoodbyeDialog", category_Data, &DialogChoice::getGoodbyeDialog, &DialogChoice::setGoodbyeDialog);
REFLECTION_END();

DialogChoice::DialogChoice() 
	: DescribedCreatable<DialogChoice, Instance, sDialogChoice>()
{
	this->setName(sDialogChoice);
}

void DialogChoice::setUserDialog(std::string value)
{
	//16 character per line, max of 3 lines
	if(value.length() > 48)
		value = value.substr(0,48);

	if(userDialog != value)
	{
		userDialog = value;
		raisePropertyChanged(prop_UserDialog);
	}
}

void DialogChoice::setResponseDialog(std::string value)
{
	if(responseDialog != value)
	{
		responseDialog = value;
		raisePropertyChanged(prop_ResponseDialog);
	}
}

void DialogChoice::setGoodbyeDialog(std::string value)
{
	if(goodbyeDialog != value)
	{
		goodbyeDialog = value;
		raisePropertyChanged(prop_GoodbyeDialog);
	}
}


bool DialogChoice::askSetParent(const Instance* instance) const
{
	return Instance::fastDynamicCast<DialogChoice>(instance) != NULL || Instance::fastDynamicCast<DialogRoot>(instance) != NULL;
}


}