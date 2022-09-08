#pragma once

#include "FastLog.h"


namespace RBX {

	// Wrapper around values that allows it to be used like
	// a normal reference to the type (i.e. T& instead of T*).
	// Also mildly obscures stored values so that they are harder to find
	// with a memory scan.
	template<typename T> class ObscureValue {
        static const int kArraySize = (sizeof(T)/sizeof(long) < 1) ? 1 : sizeof(T)/sizeof(long);
		union InternalStorage
		{
			long asRaw[kArraySize];
			T asBase;
		};
		InternalStorage storage;
	public:
		explicit ObscureValue(const T& value){
            InternalStorage tmp;
            tmp.asBase = value;
            for (int i = 0; i < kArraySize; ++i)
            {
                storage.asRaw[i] = tmp.asRaw[i] ^ reinterpret_cast<uintptr_t>(this) ;
            }
        }

		operator const T() const {
			InternalStorage tmp;
			for (int i = 0; i < kArraySize; ++i)
			{
				tmp.asRaw[i] = storage.asRaw[i] ^ reinterpret_cast<uintptr_t>(this) ;
			}
			return tmp.asBase;
		}

		ObscureValue& operator=(const T& other) {
			InternalStorage tmp;
			tmp.asBase = other;
			for (int i = 0; i < kArraySize; ++i)
			{
				storage.asRaw[i] = tmp.asRaw[i] ^ reinterpret_cast<uintptr_t>(this) ;
			}
			return *this;
		}
	private:
		// Disable no-arg construction, copy, and regular assign.
		// Some of these may be safe, but they are not needed yet,
		// and the safety of this class is easier to understand without
		// them.
		ObscureValue();
		ObscureValue(const ObscureValue&);
		ObscureValue& operator=(const ObscureValue&);
	};

}