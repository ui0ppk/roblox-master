/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Kernel/KernelIndex.h"
#include "Util/G3DCore.h"

namespace RBX {

	class Body;


	class Point 
		: public KernelIndex
	{
		friend class KernelData;
		friend class Kernel;
	private:
		int&					getKernelIndex() {return kernelIndex;}
		int						numOwners;

	protected:
		Body*					body;

		// constant
		Vector3					localPos;





		// auxillary variables, computed on every frame
		Vector3					worldPos;

		// accumulated quantities;
		Vector3					force;

		// This is private - only created by the kernel
		Point(Body* _body = NULL);

		virtual	~Point() 
		{}

	public:		// all points from same allocator, size of AttachPoint

		static bool sameBodyAndOffset(const Point& p0, const Point& p1) {
			return ((p0.body == p1.body) && (p0.localPos == p1.localPos));
		}

		//////////// called by kernel every step
		//
		// Updates World Position, Clears Accumulator

		void step();

		// force accumulation
		void accumulateForce(const Vector3& _force)	{
			force += _force;
		}

		// corresponds to "for each Point, accumulate forces to Body"
		void forceToBody();

		void setLocalPos(const Vector3& _localPos);
		
		void setWorldPos(const Vector3& _worldPos);
		
		void setBody(Body* _body) {
			body = _body;
		}

		//////////// inquiry
		Body* getBody() {
			return body;
		}

		const Vector3& getWorldPos() {
			return worldPos;
		}
	};

} // namespace RBX

