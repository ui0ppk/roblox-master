#pragma once

// Class that can average 512 prior values using only 9 floats for storage

#include <vector>

namespace RBX {

	template<typename T>
	class Average {
	private:
		size_t samples;
		size_t tag;
		std::vector<T> history;

	public:
		Average(size_t samples, T initValue) : samples(samples), tag(0)
		{
			history.resize(samples, initValue);	
		}

		void sample(T value, bool advanceBuffer = true) 
		{
			history[tag] = value;
			if (advanceBuffer) {
				tag = (tag + 1) % samples;
			}
		}

		T getAverage() const
		{
			T answer = T();
			for (size_t i = 0; i < samples; ++i) {
				answer += history[i];
			}
			return answer / static_cast<float>(samples);
		}

		size_t size() const {
			return samples;
		}
		
		const T& getValue(size_t index) const {
			return history[index];
		}

		void resetValues(const T& value) {
			for (size_t i = 0; i < samples; ++i) {
				history[i] = value;
			}
		}

		void resetValues(size_t samplesNew, const T& value) {
			for (size_t i = 0; i < samples; ++i) {
				history[i] = value;
			}
			history.resize(samplesNew, value);
			samples = samplesNew;
		}
	};
} // namespace