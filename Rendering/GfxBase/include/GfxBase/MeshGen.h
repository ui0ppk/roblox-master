#pragma once
#include "util/G3DCore.h"

namespace RBX { 

	class I3DLinearFunc
	{
		public:
			virtual Vector3 eval(float t)=0;
			// first derivative.
			virtual Vector3 evalTangent(float t)=0; // (tangent, normal, binormal, in that order, should form a right handed space)
			virtual Vector3 evalNormal(float t)=0;
			virtual Vector3 evalBinormal(float t)=0;

			//string that encodes this function in a unique way.
			virtual std::string hashString()=0;
	};

}