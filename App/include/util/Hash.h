#pragma once

#include <string>

// A simple hash function from Robert Sedgwicks Algorithms in C book. 

namespace RBX {

	class Hash {
	public:
		static unsigned int hash(const void* data, size_t bytes);
		static unsigned int hash(const std::string& str);

		static void hashAppend(unsigned int& currentHash, const void* data, size_t bytes);
		static void hashAppend(unsigned int& currentHash, unsigned int append);
	};

}	// namespace