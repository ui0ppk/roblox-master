#pragma once

namespace RBX {

template <typename Enum, typename Storage> class CompactEnum
{
public:
	CompactEnum()
	{
	}

	CompactEnum(Enum value): data(value)
	{
	}

	operator Enum() const
	{
		return static_cast<Enum>(data);
	}

private:
	Storage data;
};

}
