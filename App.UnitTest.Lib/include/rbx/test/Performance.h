#pragma once

namespace RBX { namespace Test {
	class Memory
	{
		long initialBytes;
	public:
		Memory();
		long GetBytes();
	};
}}