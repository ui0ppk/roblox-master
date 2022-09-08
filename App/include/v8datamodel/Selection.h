#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"

#include <vector>

namespace RBX {

	class Selection;

	class SelectionChanged {
		friend class Selection;
	public:
		shared_ptr<Instance> const addedItem;
		shared_ptr<Instance> const removedItem;
	private:
		SelectionChanged(shared_ptr<Instance> added, shared_ptr<Instance> removed)
			:addedItem(added),removedItem(removed)
		{}
	};

	class RBXInterface ISelectionBase
	{
		friend class Selection;
	private:
		virtual void onSelectionChanged(const SelectionChanged& event) = 0;
	};

	extern const char *const sSelection;
	class Selection 
		: public DescribedNonCreatable<Selection, Instance, sSelection>
		, public Service
	{
	private:
		copy_on_write_ptr<Instances> selection;
		std::vector<ISelectionBase*> filteredSelections;
		typedef std::map<RBX::Instance*, rbx::signals::connection> Connections;
		Connections connections;

	public:
		rbx::signal<void(const SelectionChanged&)> selectionChanged;
		rbx::signal<void()> luaSelectionChanged;

		Selection();
		~Selection();
		inline size_t size() const { return selection->size(); }

		shared_ptr<Instance> front() const { return selection->size()>0 ? selection->front() : shared_ptr<Instance>(); }
		shared_ptr<Instance> back() const { return selection->size()>0 ? selection->back() : shared_ptr<Instance>(); }
		Instances::const_iterator begin() const { return selection->begin(); } 
		Instances::const_iterator end() const { return selection->end(); } 
		const copy_on_write_ptr<Instances>& getSelection() const { return selection; }

		void setSelection(Instance* instance);
		template<class _InIt>
		void setSelection(_InIt _First, _InIt _Last)
		{
			// This can cause more events to fire than needed. However, it may be more efficient
			// than an O:n^2 algorithm.
			clearSelection();
			for (; _First != _Last; ++_First)
				addToSelection(*_First);
		}
		void clearSelection();

		// Used for reflection. Note that it might return NULL (or an empty container)
		shared_ptr<const Instances> getSelection2() { return selection.read(); }		

		void setSelection(shared_ptr<const Instances> selection);

		void addToSelection(Instance* instance);
		void addToSelection(const shared_ptr<Instance>& instance) {
			addToSelection(instance.get());
		}
		template<class _InIt>
		void addToSelection(_InIt _First, _InIt _Last)
		{
			for (; _First != _Last; ++_First)
				addToSelection(*_First);
		}

		void toggleSelection(Instance* instance);
		void toggleSelection(shared_ptr<const Instances> instances);
		void toggleSelection(const shared_ptr<Instance>& instance) {
			toggleSelection(instance.get());
		}
		template<class _InIt>
		void toggleSelection(_InIt _First, _InIt _Last)
		{
			for (; _First != _Last; ++_First)
				toggleSelection(*_First);
		}

		void removeFromSelection(Instance* source);
		void removeFromSelection(const shared_ptr<Instance>& instance) {
			removeFromSelection(instance.get());
		}
		template<class _InIt>
		void removeFromSelection(_InIt _First, _InIt _Last)
		{
			for (; _First != _Last; ++_First)
				removeFromSelection(*_First);
		}

		inline bool isSelected(const Instance* instance) const {
			return std::find( selection->begin(), selection->end(), shared_from(instance) )!=selection->end();
		}

		// Utility class for adding Instances to the selection as a result of std algorithms
		//class AddIterator : public std::_Outit
		class AddIterator : public std::iterator<std::output_iterator_tag, void,void,void,void>
		{
			Selection* selection;
		public:
			AddIterator(Selection* selection):selection(selection) {}
			AddIterator& operator=(Instance* _Val)
			{
				selection->addToSelection(_Val);
				return (*this);
			}
			AddIterator& operator=(shared_ptr<Instance> _Val)
			{
				selection->addToSelection(_Val);
				return (*this);
			}

			AddIterator& operator*()
				{	// pretend to return designated value
				return (*this);
				}

			AddIterator& operator++()
				{	// pretend to preincrement
				return (*this);
				}

			AddIterator operator++(int)
				{	// pretend to postincrement
				return (*this);
				}
		};
		// Utility class for removing Instances from the selection as a result of std algorithms
		class RemoveIterator : public std::iterator<std::output_iterator_tag, void,void,void,void>
		{
			Selection* selection;
		public:
			RemoveIterator(Selection* selection):selection(selection) {}
			RemoveIterator& operator=(Instance* _Val)
			{
				selection->removeFromSelection(_Val);
				return (*this);
			}
			RemoveIterator& operator=(shared_ptr<Instance> _Val)
			{
				selection->removeFromSelection(_Val);
				return (*this);
			}

			RemoveIterator& operator*()
				{	// pretend to return designated value
				return (*this);
				}

			RemoveIterator& operator++()
				{	// pretend to preincrement
				return (*this);
				}

			RemoveIterator operator++(int)
				{	// pretend to postincrement
				return (*this);
				}
		};
		// Utility class for toggling selection as a result of std algorithms
		class ToggleIterator : public std::iterator<std::output_iterator_tag, void,void,void,void>
		{
			Selection* selection;
		public:
			ToggleIterator(Selection* selection):selection(selection) {}
			ToggleIterator& operator=(Instance* _Val)
			{
				selection->toggleSelection(_Val);
				return (*this);
			}
			ToggleIterator& operator=(shared_ptr<Instance> _Val)
			{
				selection->toggleSelection(_Val);
				return (*this);
			}

			ToggleIterator& operator*()
				{	// pretend to return designated value
				return (*this);
				}

			ToggleIterator& operator++()
				{	// pretend to preincrement
				return (*this);
				}

			ToggleIterator operator++(int)
				{	// pretend to postincrement
				return (*this);
				}
		};
		virtual void onAncestryChanged(Instance* source);

		void addFilteredSelection(ISelectionBase* fs);
		void removeFilteredSelection(ISelectionBase* fs);
	private:
		void raiseAdded(shared_ptr<Instance> instance);
		void raiseRemoved(shared_ptr<Instance> source);
		void connect(Instance* instance);
		void disconnect(Instance* instance);
		void propagateChangeSignalToLua(const RBX::SelectionChanged& event);
		bool instanceCanLiveInSelection(Instance* instance);

		rbx::signals::connection selectionChangedConnection;
	};
	
	extern const char* const sFilteredSelection;

	template<class C>
	class FilteredSelection
		: public NonFactoryProduct<Instance,sFilteredSelection>
		// TODO: Add a notifier?
		, public Service
		, public ISelectionBase
	{
	private:
		typedef NonFactoryProduct<Instance,sFilteredSelection> Super;

	public:
		typedef std::vector<C*> CollectionType;
	private:
		shared_ptr<Selection> rootSelection;
		CollectionType filteredSelection;
	public:
		FilteredSelection() {
			setName("FilteredSelection");
		}
		~FilteredSelection() {
			if (rootSelection!=NULL)
				rootSelection->removeFilteredSelection(this);
		}

		// Returns the generic Selection service
		Selection* getSelection() {
			RBXASSERT(rootSelection!=NULL);
			return rootSelection.get();
		}

		const CollectionType& items() const { return filteredSelection; }
		inline size_t size() const { return filteredSelection.size(); }
		inline C* front() { return size()>0 ? filteredSelection.front() : NULL; }
		inline const C* front() const { return size()>0 ? filteredSelection.front() : NULL; }
		inline C* back() { return size()>0 ? filteredSelection.back() : NULL; }
		inline const C* back() const { return size()>0 ? filteredSelection.back() : NULL; }

		typename CollectionType::const_iterator begin() const { return filteredSelection.begin(); }
		typename CollectionType::const_iterator end() const { return filteredSelection.end(); }

		template<class _Fn1>
		inline void for_each(_Fn1 _Func) {
			std::for_each(filteredSelection.begin(), filteredSelection.end(), _Func);
		}

		template<class _Pr> 
		inline C* find_if(_Pr _Pred) {
			typename CollectionType::iterator iter = std::find_if(filteredSelection.begin(), filteredSelection.end(), _Pred);
			if (iter!=end())
				return *iter;
			else
				return NULL;
		}

		
		// These convenience functions pass through to the generic Selection. This means
		// that functions like "clearSelection" clears EVERYTHING, even selected
		// items that are not of type C
		void clearSelection()
		{
			getSelection()->clearSelection();
		}
		template<class _InIt>
		void addToSelection(_InIt _First, _InIt _Last)
		{
			getSelection()->addToSelection(_First, _Last);
		}
		template<class _InIt>
		void removeFromSelection(_InIt _First, _InIt _Last)
		{
			getSelection()->removeFromSelection(_First, _Last);
		}
		void addToSelection(C* c)
		{
			getSelection()->addToSelection(c);
		}
		void setSelection(C* c)
		{
			getSelection()->setSelection(c);
		}

		template<class _InIt>
		void setSelection(_InIt _First, _InIt _Last)
		{
			getSelection()->setSelection(_First, _Last);
		}

		
	protected:
		/*override*/ virtual void onAncestorChanged(const AncestorChanged& event);
		/*implement*/ virtual void onSelectionChanged(const SelectionChanged& event)
		{
			// If an item was added to the selection, add it to ours if it is of the right type
			C* c = dynamic_cast<C*>(event.addedItem.get());
			if (c!=NULL)
				filteredSelection.push_back(c);

			// If an item was removed from the selection, remove it from ours
			if (event.removedItem!=NULL)
			{
				typename CollectionType::iterator it = std::find( filteredSelection.begin(), filteredSelection.end(), event.removedItem.get() );
				if (it != filteredSelection.end())
					filteredSelection.erase(it);
			}

		}

	};
	
	template<class C>
	/*override*/ void FilteredSelection<C>::onAncestorChanged(const AncestorChanged& event)
	{
		Super::onAncestorChanged(event);
		if (rootSelection==NULL)
		{
			rootSelection = shared_from(ServiceProvider::create<Selection>(event.newParent));
			if (rootSelection!=NULL)
			{
				// Filter the current items
				for (Instances::const_iterator iter = rootSelection->begin(); iter!=rootSelection->end(); ++iter)
				{
					C* c = dynamic_cast<C*>(iter->get());
					if (c!=NULL)
						filteredSelection.push_back(c);
				}

				// Listen for changes
				rootSelection->addFilteredSelection(this);
			}
		}
	}

} // namespace
