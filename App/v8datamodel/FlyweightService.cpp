/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/FlyweightService.h"
#include "v8datamodel/Value.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "Util/MD5Hasher.h"
#include "Network/Players.h"

FASTFLAG(StudioCSGAssets)
DYNAMIC_FASTFLAGVARIABLE(DoNotCleanCSGDictionaryOnPublishInCloudEdit, true)

namespace {

	const std::string localKeyTag("CSGK");
	const size_t minKeySize = 4;
}

namespace RBX
{
	const char *const sFlyweightService = "FlyweightService";

	FlyweightService::FlyweightService()
	{
		setName("FlyweightService");
	}

	void FlyweightService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
	{
		Super::onServiceProvider(oldProvider, newProvider);
		
		if (!stringChildAddedSignal.connected())
			stringChildAddedSignal = this->onDemandWrite()->childAddedSignal.connect(boost::bind(&FlyweightService::onChildAdded, shared_from(this), _1));
	}

	void FlyweightService::onChildAdded(shared_ptr<RBX::Instance> childInstance)
	{
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent, GA_CATEGORY_GAME, "UsingCSG", "none", 0, false), flag);

		if (shared_ptr<RBX::BinaryStringValue> strValue = RBX::Instance::fastSharedDynamicCast<RBX::BinaryStringValue>(childInstance))
		{
			std::string key = createHashKey(strValue->getValue().value());

			if (!instanceMap.count(key))
			{
				instanceMap.insert(std::make_pair(key, InstanceStringData(strValue, 0)));
			}
			else if (!instanceMap.at(key).ref.lock())
			{
				instanceMap.at(key).ref = strValue;
			}
		}
	}

	std::string FlyweightService::createHashKey(const std::string& data)
	{
  	    boost::scoped_ptr<RBX::MD5Hasher> hasher(RBX::MD5Hasher::create());
        hasher->addData(data);
        return hasher->c_str();
	}

	std::string FlyweightService::getLocalKeyHash(const std::string& str)
	{
		RBXASSERT(str.size() > localKeyTag.size());

		if (str.size() > localKeyTag.size())
		{
			std::string key;
			key.resize(str.size() - localKeyTag.size());
			memcpy(&key[0], &str[localKeyTag.size()], key.size());
			return key;
		}
		return "";
	}

	std::string FlyweightService::getLocalKeyHash(const BinaryString& str)
	{
		return getLocalKeyHash(str.value());
	}

	const BinaryString FlyweightService::peekAtData(const BinaryString& str)
	{
        bool hasHash = FFlag::StudioCSGAssets ? isHashKey(str.value()) : (str.value().size() > minKeySize && (strncmp(str.value().c_str(), localKeyTag.c_str(), minKeySize) == 0));
        if (hasHash)
		{
            std::string key = getLocalKeyHash(str);

            if (instanceMap.count(key))
                if (shared_ptr<RBX::BinaryStringValue> strChild = instanceMap.at(key).ref.lock())
                    return strChild->getValue();
        }
        else
        {
            return str;
        }
		return BinaryString("");
	}

	void FlyweightService::storeStringData(BinaryString& str, bool forceIncrement, const std::string& name)
	{
        bool hasHash = FFlag::StudioCSGAssets ? isHashKey(str.value()) : (str.value().size() > minKeySize && (strncmp(str.value().c_str(), localKeyTag.c_str(), minKeySize) == 0));

        if (!hasHash)
		{
            std::string key = createHashKey(str.value());

            bool addChild = true;

            if (instanceMap.count(key))
            {
                shared_ptr<RBX::BinaryStringValue> strChild = instanceMap.at(key).ref.lock();

                if (strChild && strChild->getParent() == this)
                {
                    instanceMap.at(key).count++;
                    addChild = false;
                }
            }

            if (addChild)
            {
                shared_ptr<BinaryStringValue> stringValue = Creatable<Instance>::create<BinaryStringValue>();
                stringValue->setValue(str);
                stringValue->setName( name );
                stringValue->setParent(this);

                if (instanceMap.count(key))
                {
                    instanceMap.at(key).count++;
                }
                else
                {
                    instanceMap.insert(std::make_pair(key, InstanceStringData(stringValue, 1)));
                }
            }

            std::string urlKey = localKeyTag;
            urlKey.resize(localKeyTag.size() + key.size());
            memcpy(&urlKey[localKeyTag.size()], &key[0], key.size());
            str.set(urlKey.c_str(), urlKey.size());
        }
        else if (forceIncrement)
        {
            std::string key = getLocalKeyHash(str);
            if (instanceMap.count(key))
            {
                instanceMap.at(key).count++;
            }
            else
            {
                instanceMap.insert(std::make_pair(key, InstanceStringData(shared_ptr<BinaryStringValue>(), 1)));
            }
        }
    }

	void FlyweightService::retrieveStringData(BinaryString& str)
	{
        bool hasHash = FFlag::StudioCSGAssets ? isHashKey(str.value()) : (str.value().size() > minKeySize && (strncmp(str.value().c_str(), localKeyTag.c_str(), minKeySize) == 0));

        if (hasHash)
		{
            std::string key = getLocalKeyHash(str);

            if (instanceMap.count(key))
            {
                if (shared_ptr<RBX::BinaryStringValue> strChild = instanceMap.at(key).ref.lock())
                {
                    const BinaryString& dataValue = strChild->getValue();
                    str.set(dataValue.value().c_str(), dataValue.value().size());

                    instanceMap.at(key).count--;
                }
            }
        }
    }

    void FlyweightService::removeStringData(const BinaryString& str)
    {
		if (DFFlag::DoNotCleanCSGDictionaryOnPublishInCloudEdit && Network::Players::isCloudEdit(this))
		{
			return;
		}

        if (isHashKey(str.value()))
        {
            std::string key = getLocalKeyHash(str);

            FlyweightInstanceMap::iterator iter = instanceMap.find(key);
            if (iter != instanceMap.end())
            {
                if (shared_ptr<RBX::BinaryStringValue> strChild = iter->second.ref.lock())
                {
                    strChild->setParent(NULL);
                }
                instanceMap.erase(iter);
            }
        }
	}

	void FlyweightService::cleanChildren()
	{
		if (Network::Players::isCloudEdit(this))
		{
			return;
		}

		if (numChildren())
		{
			std::vector<shared_ptr<BinaryStringValue> > toDelete;
			for (RBX::Instances::const_iterator iter = getChildren()->begin(); iter != getChildren()->end(); ++iter)
			{
				if (shared_ptr<BinaryStringValue> stringValue = RBX::Instance::fastSharedDynamicCast<BinaryStringValue>(*iter))
				{
					std::string key = createHashKey(stringValue->getValue().value());

					if (instanceMap.count(key) && !instanceMap.at(key).ref.lock())
						instanceMap.at(key).ref = stringValue;
					else
						toDelete.push_back(stringValue);
				}
			}

			for (std::vector<shared_ptr<BinaryStringValue> >::const_iterator iter = toDelete.begin(); iter != toDelete.end(); ++iter)
			{
				(*iter)->setParent(NULL);
			}
		}
	}

	void FlyweightService::clean()
	{
		if (Network::Players::isCloudEdit(this))
		{
			return;
		}

		std::vector<FlyweightInstanceMap::const_iterator> toDelete;
		for (FlyweightInstanceMap::const_iterator iter = instanceMap.begin(); iter != instanceMap.end(); ++iter)
		{
			if (iter->second.count <= 0 || !iter->second.ref.lock())
				toDelete.push_back(iter);
			else
				instanceMap.at(iter->first).ref = shared_ptr<BinaryStringValue>();
		}
		if (toDelete.size() > 0)
		{
			for (std::vector<FlyweightInstanceMap::const_iterator>::iterator iter = toDelete.begin(); iter != toDelete.end(); ++iter)
			{
				shared_ptr<BinaryStringValue> stringValue = (*iter)->second.ref.lock();

				instanceMap.erase((*iter));
				if (stringValue)
				{
					stringValue->setParent(NULL);
				}
			}
		}
		cleanChildren();
	}

	void FlyweightService::refreshRefCount()
	{
		instanceMap.clear();

		refreshRefCountUnderInstance(DataModel::get(this));

		cleanChildren();
	}

	std::string FlyweightService::dataType(std::string key)
	{
		if (instanceMap.count(key))
		{
			if (shared_ptr<RBX::BinaryStringValue> strChild = instanceMap.at(key).ref.lock())
			{
				if (isChildData(strChild))
				{
					return "C";
				}
				else if (strncmp(strChild->getValue().value().c_str(), "CSGPH", 5) == 0)
				{
					return "P";
				}
				else
				{
					return "M";
				}
			}
		}

		return "-";
	}

	void FlyweightService::printMapSizes()
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_OUTPUT, "Instance map size:%d ,Child count:%d", (int)instanceMap.size(), (int)numChildren());
		for(FlyweightInstanceMap::const_iterator iter = instanceMap.begin(); iter != instanceMap.end(); ++iter)
		{
			if (iter->first.c_str())
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_OUTPUT, "Count: %d, %s Value: %s", iter->second.count, dataType(iter->first).c_str(), iter->first.c_str());
		}
	}

	bool FlyweightService::isChildData(shared_ptr<RBX::Instance> childData)
	{
		if (shared_ptr<RBX::BinaryStringValue> bStrValue = RBX::Instance::fastSharedDynamicCast<RBX::BinaryStringValue>(childData))
			if (strncmp(bStrValue->getValue().value().c_str(), "<roblox", 6) == 0)
				return true;
		return false;
	}

	bool FlyweightService::isHashKey(const std::string& str)
	{
        if (!FFlag::StudioCSGAssets)
            return strncmp(str.c_str(), localKeyTag.c_str(), minKeySize) == 0;

        if (str.empty())
            return false;

        if (str.size() < minKeySize)
            return false;

		if (strncmp(str.c_str(), localKeyTag.c_str(), minKeySize) == 0)
            return true;

        static const std::string httpStr("http://");
        static const std::string httpsStr("https://");

        if (strncmp(str.c_str(), httpStr.c_str(), httpStr.size()) == 0 ||
            strncmp(str.c_str(), httpsStr.c_str(), httpsStr.size()) == 0)
            return true;

        return false;
	}

	std::string FlyweightService::getHashKey(const std::string& str)
	{
		if (str.size() < minKeySize)
			return "";

		if (strncmp(str.c_str(), localKeyTag.c_str(), minKeySize) != 0)
			return createHashKey(str);
		
		return str;
	}

}
