#pragma once

namespace RBX
{
	// TODO: This probably exists in some other library somewhere...
	template <typename V>
	class ScopedAssign
	{
		V* value;
		V oldValue;
	public:
		ScopedAssign() : value(NULL) {}
		ScopedAssign(V& value, const V& newValue)
			:value(&value)
			,oldValue(value)
		{
			*(this->value) = newValue;
		}
		~ScopedAssign() 
		{
			if (value)
				*value = oldValue;
		}

		void assign (V& value, const V& newValue)
		{
			this->value = &value;
			oldValue = value;
			*(this->value) = newValue;
		}
	};
}

