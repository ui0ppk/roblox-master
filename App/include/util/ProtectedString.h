#pragma once

#include <string>
#include <boost/functional/hash.hpp>
#include <boost/scoped_ptr.hpp>

struct lua_State;

namespace RBX {
    
	class ProtectedString
	{
	public:
		static const ProtectedString emptyString;

		static ProtectedString fromTrustedSource(const std::string& stringRef);
		static ProtectedString fromBytecode(const std::string& stringRef);

        // Only use in unit tests!
		static ProtectedString fromTestSource(const std::string& stringRef);

		ProtectedString();
		ProtectedString(const ProtectedString& other);

		const std::string& getSource() const { return source; }
		const std::string& getBytecode() const { return bytecode; }

        bool empty() const { return source.empty() && bytecode.empty(); }

		const std::string& getOriginalHash() const;
		void calculateHash(std::string* out) const;

		bool operator==(const ProtectedString& other) const;
		bool operator!=(const ProtectedString& other) const;
		
		ProtectedString& operator=(const ProtectedString& other);

	private:
		std::string source;
		std::string bytecode;

		// Need to keep a pointer to hash to keep the size of this object in
		// line with other lua-bridged types.
		boost::scoped_ptr<std::string> hash;

		// Hide this to force all changes in string to go through
		// fromTrustedSource.
		void setString(const std::string& newSource, const std::string& newBytecode);
	};

	size_t hash_value(const ProtectedString& str);

}
