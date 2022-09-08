#pragma once

namespace RBX {

template<class ElementType, int size=8>
struct FixedSizeCircularBuffer {
private:
	ElementType data[size];
	unsigned int head;
	unsigned int pushed;
public:

	FixedSizeCircularBuffer() : head(0), pushed(0) {}

	void push(const ElementType& newData) {
		head = (head + size - 1) % size;
		data[head] = newData;
		if (pushed < size) { pushed++; }
	}

	bool find(const ElementType& key, unsigned int* outIndex) {
		for (unsigned int i = 0; i < pushed; ++i) {
			if (data[(head + i) % size] == key) {
				(*outIndex) = i;
				return true;
			}
		}
		return false;
	}

	const ElementType& operator[](const unsigned int& index) const {
		return data[(head + index) % size];
	}
};

}