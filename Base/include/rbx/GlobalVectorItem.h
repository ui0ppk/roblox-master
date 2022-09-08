#pragma once


namespace RBX
{

	struct GlobalVectorItemBase
	{
		bool valid;
		GlobalVectorItemBase() : valid(false) {};
	};

	// this class manages a sparse static array.
	// allocate one of these "smartptr-like" classes to get the next available entry in the list.
	// T should derive from GlobalVectorItemBase
	template<class T> 
	class GlobalVectorItemPtr
	{
		T* p;
		T* newp;
	public:
		GlobalVectorItemPtr(T* list, size_t count) : p(0), newp(0)
		{
			for(size_t i = 0; i< count; ++i, ++list)
			{
				if(!list->valid)
				{
					p = list;
					p->valid = true;
					return;
				}
			}
			// out of space. just allocate one.

			newp = new T();
			p = newp;
		}

		~GlobalVectorItemPtr()
		{
			if(newp)
			{
				delete newp;
				newp = 0;
				p = 0;
			}
			else
			{
				// free slot.
				p->valid = false;
				p = 0;
			}
		}
	

		T* operator->()
		{
			return this->p;
		}

		T* get()
		{
			return this->p;
		}
	};
}