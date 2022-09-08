#pragma once

namespace RBX {

	class Primitive;

	// Fail:	Stop the hit test - don't bore any further down
	// Ignore:	Keep testing
	// Hit:		Found something

	class HitTestFilter {
	public:
		typedef enum Result	{	STOP_TEST, 
								IGNORE_PRIM, 
								INCLUDE_PRIM} Result;

		virtual Result filterResult(const Primitive* testMe) const = 0;
		virtual ~HitTestFilter()
		{}
	};

} // namespace