#include "stdafx.h"

#include "V8DataModel/Selection.h"
#include "V8DataModel/DataModel.h"

const char* const RBX::sSelection			= "Selection";
const char* const RBX::sFilteredSelection	= NULL;

using namespace RBX;


REFLECTION_BEGIN();
// TODO: add, remove, toggle, isSelected
static Reflection::BoundFuncDesc<Selection, shared_ptr<const Instances>()> func_getSelection(&Selection::getSelection2, "Get", Security::Plugin);
static Reflection::BoundFuncDesc<Selection, void(shared_ptr<const Instances>)> func_setSelection(&Selection::setSelection, "Set", "selection", Security::Plugin);
static Reflection::EventDesc<Selection, void() > desc_SelectionChanged(&Selection::luaSelectionChanged, "SelectionChanged");
REFLECTION_END();

Selection::Selection()
	:selection(Instances())	// Initialize with an empty container
{
	setName("Selection");
	selectionChangedConnection = selectionChanged.connect(boost::bind(&Selection::propagateChangeSignalToLua, this, _1));
}

Selection::~Selection() {
	RBXASSERT(size()==0);
	RBXASSERT(connections.empty());
}

void Selection::onAncestryChanged(Instance* source)
{
	if (!instanceCanLiveInSelection(source))
		removeFromSelection(source);
}

void Selection::connect(Instance* instance)
{
		RBXASSERT_SLOW(connections.find(instance)==connections.end());
		connections[instance] = instance->ancestryChangedSignal.connect(boost::bind(&Selection::onAncestryChanged, this, instance));
}

void Selection::disconnect(Instance* instance)
{
		RBXASSERT(connections.find(instance)!=connections.end());
		Connections::iterator iter = connections.find(instance);
		iter->second.disconnect();
		connections.erase(iter);
}

void Selection::propagateChangeSignalToLua(const RBX::SelectionChanged& event) {
	luaSelectionChanged();
}

bool Selection::instanceCanLiveInSelection(Instance* instance)
{
	Instance* rootAncestor = getRootAncestor();
	return instance && rootAncestor && instance->getRootAncestor() == rootAncestor;
}

void Selection::toggleSelection(Instance* instance)
{
	RBXASSERT(DataModel::get(this)->write_requested);
	shared_ptr<Instance> shared = shared_from(instance);

	shared_ptr<Instances> sel = selection.write();
	Instances::iterator it = std::find(sel->begin(), sel->end(), shared );
	if (it == sel->end())
	{
		if (instanceCanLiveInSelection(instance))
		{
			sel->push_back(shared);
			connect(instance);
			raiseAdded(shared);
		}
	}
	else
	{
		sel->erase(it);
		disconnect(instance);
		raiseRemoved(shared);
	}
}

void Selection::toggleSelection(shared_ptr<const Instances> instances)
{
	if (selection->size() == 0 && (!instances || instances->size() == 0))
		return;

	selectionChangedConnection.disconnect();
	
	if (instances)
	{
		shared_ptr<Instances> currentSelection = selection.write();

		for (Instances::const_iterator iter = instances->begin(); iter != instances->end(); ++iter)
		{
			shared_ptr<Instance> instance = *iter;

			Instances::iterator it = std::find(currentSelection->begin(), currentSelection->end(), instance);
			if (it == currentSelection->end())
			{
				if (instanceCanLiveInSelection(instance.get()))
				{
					currentSelection->push_back(instance);
					connect(instance.get());
					raiseAdded(instance);
				}
			}
			else
			{
				currentSelection->erase(it);
				disconnect(instance.get());
				raiseRemoved(instance);
			}
		}
	}

	selectionChangedConnection = selectionChanged.connect(boost::bind(&Selection::propagateChangeSignalToLua, this, _1));

	luaSelectionChanged();
}

void Selection::addToSelection(Instance* instance)
{
	if (!instanceCanLiveInSelection(instance))
	{
		return;
	}

	RBXASSERT(!DataModel::get(this) || DataModel::get(this)->write_requested);
	shared_ptr<Instance> shared = shared_from(instance);

	RBXASSERT(instance!=NULL);
	shared_ptr<Instances> sel = selection.write();
	Instances::const_iterator it = std::find( sel->begin(), sel->end(), shared );
	if (it == sel->end()) {
		sel->push_back(shared);
		connect(shared.get());
		raiseAdded(shared);
	}
}

void Selection::clearSelection()
{
	RBXASSERT(!DataModel::get(this) || DataModel::get(this)->write_requested);
	while (selection->size()>0)
	{
		shared_ptr<Instance> instance = selection->back();
		selection.write()->pop_back();
		disconnect(instance.get());
		raiseRemoved(instance);
	}
}
void Selection::addFilteredSelection(ISelectionBase* fs)
{
	this->filteredSelections.push_back(fs);
}
void Selection::removeFilteredSelection(ISelectionBase* fs)
{
	filteredSelections.erase(std::find(filteredSelections.begin(), filteredSelections.end(), fs));
}

void Selection::raiseAdded(shared_ptr<Instance> instance)
{
	SelectionChanged event(instance, shared_ptr<Instance>());
	// First notify the filtered selections
	for (std::vector<ISelectionBase*>::iterator iter = filteredSelections.begin(); iter!=filteredSelections.end(); ++iter)
		(*iter)->onSelectionChanged(event);
	// Now notify everybody else
	selectionChanged(boost::cref(event));
}

void Selection::raiseRemoved(shared_ptr<Instance> source)
{
	SelectionChanged event(shared_ptr<Instance>(), source);
	// First notify the filtered selections
	for (std::vector<ISelectionBase*>::iterator iter = filteredSelections.begin(); iter!=filteredSelections.end(); ++iter)
		(*iter)->onSelectionChanged(event);
	// Now notify everybody else
	selectionChanged(boost::cref(event));
}

void Selection::setSelection(shared_ptr<const Instances> instances)
{
	if (selection->size() == 0 && (!instances || instances->size() == 0))
		return;

	selectionChangedConnection.disconnect();

	clearSelection();

	if (instances)
	{
		shared_ptr<Instances> currentSelection = selection.write();

		RBXASSERT(!DataModel::get(this) || DataModel::get(this)->write_requested);

		for (Instances::const_iterator iter = instances->begin(); iter != instances->end(); ++iter)
		{
			shared_ptr<Instance> instance = *iter;
			if (!instanceCanLiveInSelection(iter->get()))
				break;

			if (connections.find(instance.get()) != connections.end())
				continue;

			currentSelection->push_back(instance);

			connect(instance.get());
			raiseAdded(instance);
		}
	}

	selectionChangedConnection = selectionChanged.connect(boost::bind(&Selection::propagateChangeSignalToLua, this, _1));

	luaSelectionChanged();
	
}

void Selection::setSelection(Instance* instance)
{
	RBXASSERT(DataModel::get(this)->write_requested);
	// Note that we call selection.write() multiple times.
	// It is slightly dangerous to use a single write() instance
	// throughout the algorithm because the raiseRemoved call
	// might cause the selection to be modified as well.
	// This is somewhat less efficient than using a single
	// write() instance.

	shared_ptr<Instance> inst = shared_from(instance);

	// Remove all items except -instance-
	while (selection->size()>0)
	{		
		shared_ptr<Instance> item = selection->back();
		if (item==inst)
		{
			if (selection->size()==1)
				return;	// Done!
			else
			{
				shared_ptr<Instances> sel = selection.write();

				// Move the instance to the front
				sel->back() = sel->front();
				sel->front() = item;
			}
		}
		else
		{
			// Issue a new "write" request for each iteration
			// just in case raiseRemoved has side-effects
			selection.write()->pop_back();
			disconnect(item.get());
			raiseRemoved(item);
		}
	}

	// Add instance, if necessary
	if (inst && instanceCanLiveInSelection(instance))
	{
		selection.write()->push_back(inst);
		connect(inst.get());
		raiseAdded(inst);
	}
}

void Selection::removeFromSelection(Instance* instance)
{
	RBXASSERT(!DataModel::get(this) || DataModel::get(this)->write_requested);
	RBXASSERT(instance!=NULL);

	shared_ptr<Instances> sel = selection.write();
	Instances::iterator iter = std::find(sel->begin(), sel->end(), shared_from(instance));
	if (iter!=sel->end())
	{
		shared_ptr<Instance> nonConst = *iter;
		sel->erase(iter);
		disconnect(instance);
		raiseRemoved(nonConst);
	}
}

