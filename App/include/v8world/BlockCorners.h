#pragma once

#include "Util/Memory.h"


/*
	Utility class - holds Vector3 [8] so that all blocks of the same size use the same geometry to improve cache / ram size for collisions
*/

namespace RBX {

	namespace POLY {

		class BlockCorners : public Allocator<BlockCorners>
		{
		private:
			Vector3 vertices[8];
		public:
			BlockCorners(const Vector3& _corner) 
			{
				Vector3 corner;
				corner.x = - std::abs(_corner.x);
				corner.y = - std::abs(_corner.y);
				corner.z = - std::abs(_corner.z);

				for (int i = 0; i < 2; i++) {
					corner.x *= -1.0;						// positive for i = 0, negative for i = -1
					for (int j = 0; j < 2; j++) {			
						corner.y *= -1.0;					// positive for j = 0...
						for (int k = 0; k < 2; k++) {
							corner.z *= -1.0;				
							vertices[i*4 + j*2 + k] = corner;
						}
					}
				}
			}
			const Vector3* getVertices() const {return vertices;}
		};

	} // namespace POLY
} // namespace RBX
