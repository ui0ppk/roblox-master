#pragma once

#include "rbx/Debug.h"

#include <vector>

namespace RBX {

/**
 * Wrapper around std::vector that allows for quick insert and pop from both
 * front and back. This container never attempts to reduce the amount of
 * memory it uses, so it may not be suitable for queues that do not have a
 * reasonable upper bound in size.
 */
template<class T>
struct DoubleEndedVector {
private:
	size_t head;
	size_t internalSize;
	std::vector<T> data;
	size_t dataSizeMask;

	void grow() {
		if (internalSize == data.size()) {
			std::vector<T> replacement(std::max((size_t)32, internalSize * 2));

			if (data.size() > 0) {
				size_t firstSegment = data.size() - head;
				size_t secondSegment = internalSize - firstSegment;

				RBXASSERT(firstSegment + secondSegment == data.size());
				std::copy(&data[head], &data[head] + firstSegment, &replacement[0]);
				std::copy(&data[0], &data[0] + secondSegment, &replacement[firstSegment]);
			}

			head = 0;
			data.swap(replacement);
			RBXASSERT((data.size() & (data.size() - 1)) == 0);
			dataSizeMask = data.size() - 1;
		}
	}

public:
	DoubleEndedVector() : head(0), internalSize(0), data(), dataSizeMask(0) {}

	size_t size() const {
		return internalSize;
	}

	bool push_back(const T& inputData) {
		grow();
		data[(head + internalSize) & dataSizeMask] = inputData;
		internalSize++;
		return true;
	}
	bool push_front(const T& inputData) {
		grow();

		size_t newHead = (head - 1) & dataSizeMask;
		
		data[newHead] = inputData;
		head = newHead;
		internalSize++;
		return true;
	}

	void pop_front(T* out) {
		RBXASSERT(internalSize > 0);
		(*out) = data[head];
		head = (head + 1) & dataSizeMask;
		internalSize--;
	}

	inline T& operator[](const unsigned int& idx) {
		return data[(head + idx) & dataSizeMask];
	}
	inline const T& operator[](const unsigned int& idx) const {
		return data[(head + idx) & dataSizeMask];
	}
};



}