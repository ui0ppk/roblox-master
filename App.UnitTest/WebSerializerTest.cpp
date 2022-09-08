#include <boost/test/unit_test.hpp>
#include "V8Xml/WebSerializer.h"
#include "V8Xml/WebParser.h"
#include "V8DataModel/BasicPartInstance.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/JointInstance.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE(WebSerializer)

	BOOST_AUTO_TEST_CASE(TestWebSerializerList)
	{	
		shared_ptr<Reflection::ValueArray> valueCollection(new Reflection::ValueArray());
		{
			Reflection::Variant val = 2.25f;
			valueCollection->push_back(val);
		}
		{
			Reflection::Variant val = std::string("foo bar");
			valueCollection->push_back(val);
		}
		{
			Reflection::Variant val = true;
			valueCollection->push_back(val);
		}

		{
			shared_ptr<Instance> part = Creatable<Instance>::create<BasicPartInstance>();
			part->setName("CustomPart");
			Reflection::Variant val = part;
			valueCollection->push_back(val);
		}

		scoped_ptr<XmlElement> root(RBX::WebSerializer::writeList(*valueCollection));

		Reflection::Variant value;
		BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(root.get(), value));
		BOOST_CHECK(value.isType<shared_ptr<const Reflection::ValueArray> >());

		shared_ptr<const Reflection::ValueArray> newValueCollection = value.get<shared_ptr<const Reflection::ValueArray> >();
		BOOST_CHECK(newValueCollection);
        BOOST_CHECK(newValueCollection->size() == valueCollection->size());
		BOOST_CHECK(newValueCollection->at(0).isType<double>());
		BOOST_CHECK(newValueCollection->at(0).get<double>() == 2.25);
		BOOST_CHECK(newValueCollection->at(1).isType<std::string>());
		BOOST_CHECK(newValueCollection->at(1).get<std::string>() == "foo bar");
		BOOST_CHECK(newValueCollection->at(2).isType<bool>());
		BOOST_CHECK(newValueCollection->at(2).get<bool>() == true);

        {
            BOOST_CHECK(newValueCollection->at(3).isType<shared_ptr<Instance> >());
            BOOST_CHECK(newValueCollection->at(3).get<shared_ptr<Instance> >()->getName() == "CustomPart");
        }
	}

	BOOST_AUTO_TEST_CASE(TestWebSerializerTable)
	{	
		shared_ptr<Reflection::ValueMap> valueMap(new Reflection::ValueMap());
		{
			Reflection::Variant val = 2.25;
			(*valueMap)["number"] = val;
		}
		{
			Reflection::Variant val = std::string("foo bar");
			(*valueMap)["string"] = val;
		}
		{
			Reflection::Variant val = true;
			(*valueMap)["bool"] = val;
		}

		{
			shared_ptr<Instance> part = Creatable<Instance>::create<BasicPartInstance>();
			part->setName("CustomPart");
			Reflection::Variant val = part;
			(*valueMap)["instance"] = val;
		}

		{
			double val = 2.25;
			(*valueMap)["double"] = val;
		}
		{
			float val = 2.25f;
			(*valueMap)["float"] = val;
		}
		scoped_ptr<XmlElement> root(RBX::WebSerializer::writeTable(*valueMap));

		Reflection::Variant value;
		BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(root.get(), value));
		BOOST_CHECK(value.isType<shared_ptr<const Reflection::ValueMap> >());

		shared_ptr<const Reflection::ValueMap> newValueMap = value.get<shared_ptr<const Reflection::ValueMap> >();
		BOOST_CHECK(newValueMap);
		BOOST_CHECK(newValueMap->size() == valueMap->size());
		BOOST_CHECK(newValueMap->find("number")->second.isType<double>());
		BOOST_CHECK(newValueMap->find("number")->second.get<double>() == 2.25);
		BOOST_CHECK(newValueMap->find("string")->second.isType<std::string>());
		BOOST_CHECK(newValueMap->find("string")->second.get<std::string>() == "foo bar");
		BOOST_CHECK(newValueMap->find("bool")->second.isType<bool>());
		BOOST_CHECK(newValueMap->find("bool")->second.get<bool>() == true);

        {
            BOOST_CHECK(newValueMap->find("instance")->second.isType<shared_ptr<Instance> >());
            BOOST_CHECK(newValueMap->find("instance")->second.get<shared_ptr<Instance> >()->getName() == "CustomPart");
        }

		BOOST_CHECK(newValueMap->find("double")->second.isType<double>());
		BOOST_CHECK(newValueMap->find("double")->second.get<double>() == 2.25);
		BOOST_CHECK(newValueMap->find("float")->second.isType<double>());
		BOOST_CHECK(newValueMap->find("float")->second.get<double>() == 2.25);
	}

	BOOST_AUTO_TEST_CASE(TestWebSerializerInstance)
	{	
		Reflection::Variant value;
		{
			shared_ptr<Instance> part = Creatable<Instance>::create<BasicPartInstance>();
			part->setName("CustomPart");
			Creatable<Instance>::create<BasicPartInstance>()->setParent(part.get());
			Creatable<Instance>::create<BasicPartInstance>()->setParent(part.get());
			Creatable<Instance>::create<BasicPartInstance>()->setParent(part.get());
			value = part;
		}
		scoped_ptr<XmlElement> root(RBX::WebSerializer::writeValue(value));

		Reflection::Variant newValue;
		BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(root.get(), newValue));
		BOOST_CHECK(newValue.isType<shared_ptr<Instance> >());

		BOOST_CHECK(newValue.get<shared_ptr<Instance> >()->getName() == "CustomPart");
		BOOST_CHECK(newValue.get<shared_ptr<Instance> >()->getChildren()->size() == 3);
	}
	BOOST_AUTO_TEST_CASE(TestInstanceClone)
	{
		shared_ptr<Instance> model = Creatable<Instance>::create<ModelInstance>();
		shared_ptr<PartInstance> part0 = Creatable<Instance>::create<BasicPartInstance>();
		part0->setName("Part0");
		part0->setParent(model.get());
		shared_ptr<PartInstance> part1 = Creatable<Instance>::create<BasicPartInstance>();
		part1->setName("Part1");
		part1->setParent(model.get());
		shared_ptr<ManualWeld> weld = Creatable<Instance>::create<ManualWeld>();
		weld->setName("Weld");
		weld->setParent(part0.get());
		weld->setPart0(part0.get());
		weld->setPart1(part1.get());

		shared_ptr<Instance> cloneModel = model->clone(EngineCreator);
		BOOST_CHECK(cloneModel);
		Instance* clonePart0 = cloneModel->findFirstChildByName("Part0");
		BOOST_CHECK(clonePart0);
		Instance* clonePart1 = cloneModel->findFirstChildByName("Part1");
		BOOST_CHECK(clonePart1);
		ManualWeld* cloneWeld = Instance::fastDynamicCast<ManualWeld>(clonePart0->findFirstChildByName("Weld"));
		BOOST_CHECK(cloneWeld);
		BOOST_CHECK(cloneWeld->getPart0());
		BOOST_CHECK(cloneWeld->getPart0() == clonePart0);
		BOOST_CHECK(cloneWeld->getPart1());
		BOOST_CHECK(cloneWeld->getPart1() == clonePart1);
	}

	BOOST_AUTO_TEST_CASE(TestInstanceCloneExternal)
	{
		shared_ptr<Instance> model = Creatable<Instance>::create<ModelInstance>();
		shared_ptr<PartInstance> part0 = Creatable<Instance>::create<BasicPartInstance>();
		part0->setName("Part0");
		part0->setParent(model.get());
		shared_ptr<PartInstance> part1 = Creatable<Instance>::create<BasicPartInstance>();
		part1->setName("Part1");
		part1->setParent(model.get());
		shared_ptr<ManualWeld> weld = Creatable<Instance>::create<ManualWeld>();
		weld->setName("Weld");
		weld->setParent(part0.get());
		weld->setPart0(part0.get());
		weld->setPart1(part1.get());

		shared_ptr<ManualWeld> cloneWeld = Instance::fastSharedDynamicCast<ManualWeld>(weld->clone(EngineCreator));
		BOOST_CHECK(cloneWeld);
		BOOST_CHECK(cloneWeld->getPart0());
		BOOST_CHECK(cloneWeld->getPart0() == part0.get());
		BOOST_CHECK(cloneWeld->getPart1());
		BOOST_CHECK(cloneWeld->getPart1() == part1.get());
	}

BOOST_AUTO_TEST_SUITE_END()
