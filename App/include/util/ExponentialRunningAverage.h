#pragma once

#include "Util/G3DCore.h"

namespace RBX {

	// ToDo - templatize this
	class floatERA {
	private:
		float weight;
		float avg;
	public:
		floatERA()
			: weight(.5f)
		{
			reset();
		}

		floatERA(float weight) 
			: weight(weight)
		{
			reset();
		}

		void reset() {
			avg = 0.0f;
		}

		float pushAndGetAverage(float value) {
			avg = (weight * (value - avg)) + avg;
			return avg;
		}

		float getAverage() {return avg;}
	};



	class Vector3ERA {
	private:
		float weight;
		Vector3 avg;
	public:
		Vector3ERA()
			:weight(.5f) 
		{}					// Vector inits to zeros

		Vector3ERA(float weight)
			: weight(weight)
		{}					// Vector3 inits to zeros;

		void reset(const Vector3& value) {
			avg = value;
		}

		void reset() {
			reset(Vector3::zero());
		}
		
		void push(const Vector3& value) {
			avg += weight * (value - avg);
		}

		const Vector3& getAverage() {return avg;}
	};
}


