#pragma once

#include <boost/scoped_ptr.hpp>
#include "ObscureValue.h"
#include "FastLog.h"


namespace RBX {

// Wrapper around boost::scoped_ptr that allows it to be used like
// a normal reference to the type (i.e. T& instead of T*).
// Also mildly obscures stored values so that they are harder to find
// with a memory scan.
template<typename T> class HeapValue {
	boost::scoped_ptr<ObscureValue<T> > storage;
public:
	explicit HeapValue(const T& value) : storage(new ObscureValue<T>(value)) {
	}

	operator const T() const {
		return *storage;
	}

	HeapValue& operator=(const T& other) {
        *storage = other;
		return *this;
	}
private:
	// Disable no-arg construction, copy, and regular assign.
	// Some of these may be safe, but they are not needed yet,
	// and the safety of this class is easier to understand without
	// them.
	HeapValue();
	HeapValue(const HeapValue&);
	HeapValue& operator=(const HeapValue&);
};

}