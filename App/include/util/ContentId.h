/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#ifndef _4B0F5828DADB441bA2D2FDCBCB5538A6
#define _4B0F5828DADB441bA2D2FDCBCB5538A6

#include "stdio.h"
#include "util/name.h"
#include <string>
#include <istream>
#include <memory>
#include <vector>
#include "rbx/boost.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

using boost::shared_ptr;

namespace RBX {

	class ContentId
	{
	public:
		static ContentId fromUrl(const std::string& url);
		static ContentId fromAssets(const char* filePath);	// filePath is a relative pathname within the "Content" directory
		static ContentId fromGameAssetName(const std::string& gameAssetName);

		// Constructors are explicit to encourage you to use static constructors above if the string isn't a fully qualified URL
		explicit ContentId(const char* id)
			:id(id) { CorrectBackslash(this->id); }
		explicit ContentId(const std::string& id)
			:id(id) { CorrectBackslash(this->id); }
		ContentId() {}

		void clear()
		{
			id.clear();
		}

		const char* c_str() const {
			return id.c_str();
		}
		const std::string& toString() const {
			return id;
		}
		
		void convertToLegacyContent(const std::string& baseUrl);
		void convertAssetId(const std::string& baseUrl, int universeId);
		bool reconstructUrl(const std::string& baseUrl, const char* const paths[], const int pathCount);
		bool reconstructAssetUrl(const std::string& baseUrl);

		std::string getAssetId() const;
		std::string getAssetName() const;
		std::string getUnConvertedAssetName() const;

		bool isNull() const { return id.size()==0; }
		bool isAsset() const { return id.compare(0, 11, "rbxasset://") == 0; }
		bool isAssetId() const { return id.compare(0, 13, "rbxassetid://") == 0; }
		bool isHttp() const { return id.compare(0, 4, "http") == 0; }
		bool isFile() const { return id.compare(0, 7, "file://") == 0; }
		bool isRbxHttp() const { return id.compare(0, 10, "rbxhttp://") == 0; }
		bool isAppContent() const { return id.compare(0, 9, "rbxapp://") == 0; }
		bool isNamedAsset() const;
		bool isConvertedNamedAsset() const;

		friend bool operator<(const ContentId& a, const ContentId& b);
		friend bool operator==(const ContentId& a, const ContentId& b);
		friend bool operator!=(const ContentId& a, const ContentId& b);
        
    private:
		static void CorrectBackslash(std::string& id);

		std::string id;
	};

	std::size_t hash_value(const ContentId& id);

}// namespace


#endif
