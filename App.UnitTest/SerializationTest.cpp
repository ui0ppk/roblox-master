#include <boost/test/unit_test.hpp>

#include <rbx/test/ScopedFastFlagSetting.h>

#include <v8xml/SerializerBinary.h>
#include <v8xml/Serializer.h>
#include <v8xml/XmlSerializer.h>

#include <util/ProtectedString.h>
#include <util/UDim.h>
#include <util/Faces.h>
#include <util/Axes.h>
#include <util/BrickColor.h>
#include <util/BinaryString.h>
#include <util/PhysicalProperties.h>

#include <v8tree/Instance.h>
#include <v8datamodel/PartInstance.h>

#include "rbx/test/DataModelFixture.h"

using namespace RBX;

extern const char* const sSerializationTestInstance = "SerializationTestInstance";

static std::string generateRandomString(G3D::Random& rng)
{
	int length = rng.integer(0, 64);

	std::string result;
	
	for (int i = 0; i < length; ++i)
		result += static_cast<char>(rng.bits());
		
	return result;
}

struct SerializationTestInstance
	: public DescribedCreatable<SerializationTestInstance, Instance, sSerializationTestInstance>
{
	SerializationTestInstance()
	{
	}
	
	SerializationTestInstance(G3D::Random* rngp)
		: DescribedCreatable<SerializationTestInstance, Instance, sSerializationTestInstance>(generateRandomString(*rngp).c_str())
	{
		G3D::Random& rng = *rngp;
		
		prop0 = ProtectedString::fromTestSource(generateRandomString(rng));
		prop1 = generateRandomString(rng);
		prop2 = rng.bits() % 2 == 0;
		prop3 = rng.bits();
		prop4 = rng.uniform(-1e5, 1e5);
		prop5 = rng.uniform(-1e5, 1e5);
		prop6 = UDim(rng.uniform(), rng.bits());
		prop7 = UDim2(UDim(rng.uniform(), rng.bits()), UDim(rng.uniform(), rng.bits()));
		prop8 = RbxRay(Vector3(rng.uniform(), rng.uniform(), rng.uniform()), Vector3(rng.uniform(), rng.uniform(), rng.uniform()));
		prop9 = Faces(rng.bits() & 0x3f);
		prop10 = Axes(rng.bits() & 0x3f);
		prop11 = BrickColor::colorPalette()[rng.integer(0, BrickColor::colorPalette().size()-1)];
		prop12 = Color3(rng.uniform(), rng.uniform(), rng.uniform());
		prop13 = Vector2(rng.uniform(), rng.uniform());
		prop14 = Vector3(rng.uniform(), rng.uniform(), rng.uniform());
		prop15 = Vector2int16(rng.bits(), rng.bits());
		prop16 = Vector3int16(rng.bits(), rng.bits(), rng.bits());
		
		static const int orientIds[] = {1, 2, 4, 5, 8};
		
		Math::idToMatrix3(orientIds[rng.integer(0, 4)], prop17.rotation);
		prop17.translation = Vector3(rng.uniform(), rng.uniform(), rng.uniform()) * rng.uniform(0, 1e5);
		
		prop18 = rng.bits() % 2 ? GRASS_MATERIAL : SLATE_MATERIAL;
		prop19 = NULL;
		prop20 = ContentId(generateRandomString(rng));
        prop21 = BinaryString(generateRandomString(rng));
		prop22 = Rect2D::xywh(Vector2(rng.bits(), rng.bits()), Vector2(rng.bits(), rng.bits()));
		prop23 = PhysicalProperties(rng.bits(), rng.bits(), rng.bits(), rng.bits(), rng.bits());
	}
	
	PartMaterial getProp18() const { return prop18; }
	void setProp18(PartMaterial value) { prop18 = value; }
	
	SerializationTestInstance* getProp19() const { return prop19; }
	void setProp19(SerializationTestInstance* value) { prop19 = value; }
		
    ProtectedString prop0;
    std::string prop1;
    bool prop2;
    int prop3;
    float prop4;
    double prop5;
    UDim prop6;
    UDim2 prop7;
    RbxRay prop8;
    Faces prop9;
    Axes prop10;
    BrickColor prop11;
    G3D::Color3 prop12;
    G3D::Vector2 prop13;
    G3D::Vector3 prop14;
    G3D::Vector2int16 prop15;
    G3D::Vector3int16 prop16;
    G3D::CoordinateFrame prop17;
    PartMaterial prop18;
    SerializationTestInstance* prop19;
    ContentId prop20;
    BinaryString prop21;
	Rect2D prop22;
	PhysicalProperties prop23;
};

static Reflection::BoundProp<ProtectedString> desc_prop0("prop0", category_Data, &SerializationTestInstance::prop0);
static Reflection::BoundProp<std::string> desc_prop1("prop1", category_Data, &SerializationTestInstance::prop1);
static Reflection::BoundProp<bool> desc_prop2("prop2", category_Data, &SerializationTestInstance::prop2);
static Reflection::BoundProp<int> desc_prop3("prop3", category_Data, &SerializationTestInstance::prop3);
static Reflection::BoundProp<float> desc_prop4("prop4", category_Data, &SerializationTestInstance::prop4);
static Reflection::BoundProp<double> desc_prop5("prop5", category_Data, &SerializationTestInstance::prop5);
static Reflection::BoundProp<UDim> desc_prop6("prop6", category_Data, &SerializationTestInstance::prop6);
static Reflection::BoundProp<UDim2> desc_prop7("prop7", category_Data, &SerializationTestInstance::prop7);
static Reflection::BoundProp<RbxRay> desc_prop8("prop8", category_Data, &SerializationTestInstance::prop8);
static Reflection::BoundProp<Faces> desc_prop9("prop9", category_Data, &SerializationTestInstance::prop9);
static Reflection::BoundProp<Axes> desc_prop10("prop10", category_Data, &SerializationTestInstance::prop10);
static Reflection::BoundProp<BrickColor> desc_prop11("prop11", category_Data, &SerializationTestInstance::prop11);
static Reflection::BoundProp<G3D::Color3> desc_prop12("prop12", category_Data, &SerializationTestInstance::prop12);
static Reflection::BoundProp<G3D::Vector2> desc_prop13("prop13", category_Data, &SerializationTestInstance::prop13);
static Reflection::BoundProp<G3D::Vector3> desc_prop14("prop14", category_Data, &SerializationTestInstance::prop14);
static Reflection::BoundProp<G3D::Vector2int16> desc_prop15("prop15", category_Data, &SerializationTestInstance::prop15);
static Reflection::BoundProp<G3D::Vector3int16> desc_prop16("prop16", category_Data, &SerializationTestInstance::prop16);
static Reflection::BoundProp<G3D::CoordinateFrame> desc_prop17("prop17", category_Data, &SerializationTestInstance::prop17);
static Reflection::EnumPropDescriptor<SerializationTestInstance, PartMaterial> desc_prop18("prop18", category_Data, &SerializationTestInstance::getProp18, &SerializationTestInstance::setProp18);
static Reflection::RefPropDescriptor<SerializationTestInstance, SerializationTestInstance> desc_prop19("prop19", category_Data, &SerializationTestInstance::getProp19, &SerializationTestInstance::setProp19);
static Reflection::BoundProp<ContentId> desc_prop20("prop20", category_Data, &SerializationTestInstance::prop20);
static Reflection::BoundProp<BinaryString> desc_prop21("prop21", category_Data, &SerializationTestInstance::prop21);
static Reflection::BoundProp<Rect2D> desc_prop22("prop22", category_Data, &SerializationTestInstance::prop22);
static Reflection::BoundProp<PhysicalProperties> desc_prop23("prop23", category_Data, &SerializationTestInstance::prop23);

RBX_REGISTER_CLASS(SerializationTestInstance);

static std::string getBinaryRepresentation(Instance* root)
{
	std::stringstream stream;

	SerializerBinary::serialize(stream, root);
	
	return stream.str();
}

static std::string getXmlRepresentation(Instance* root)
{
	std::stringstream stream;
	
	boost::scoped_ptr<XmlElement> xmlroot(Serializer::newRootElement());

	root->writeChildren(xmlroot.get(), SerializationCreator);

	TextXmlWriter machine(stream);
	machine.serialize(xmlroot.get());
	
	return stream.str();
}

BOOST_AUTO_TEST_SUITE(SerializationTest)

#ifdef RBX_PLATFORM_IOS
#define SLEEPTIME 5
#else
#define SLEEPTIME 2
#endif

BOOST_AUTO_TEST_CASE(SerializationReplicationTest)
{
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	// Create a part on the server
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		G3D::Random rng(42);
		shared_ptr<SerializationTestInstance> instance = SerializationTestInstance::createInstance(&rng);
		instance->setName("SerializationTest");
		instance->setParent(serverDm->getWorkspace());
	}

	// Wait for it to replicate
	G3D::System::sleep(SLEEPTIME);

	// Confirm the client got it
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName("SerializationTest") != 0);
	}
}

BOOST_AUTO_TEST_CASE(RoundtripBinary)
{
	G3D::Random rng(42);
	
	std::vector<shared_ptr<SerializationTestInstance> > instances;
	instances.push_back(SerializationTestInstance::createInstance(&rng));
	
	for (size_t i = 0; i < 1000; ++i)
	{
		shared_ptr<SerializationTestInstance> instance = SerializationTestInstance::createInstance(&rng);
		
		instance->setParent(instances[rng.integer(0, instances.size() - 1)].get());
		
		instances.push_back(instance);
	}
	
	for (size_t i = 0; i < instances.size(); ++i)
	{
		if (rng.bits() % 4 != 0)
		{
			instances[i]->prop19 = instances[rng.integer(1, instances.size() - 1)].get();
		}
	}
	
	std::string binaryDataOriginal = getBinaryRepresentation(instances[0].get());
	
	shared_ptr<Instance> newRoot = SerializationTestInstance::createInstance(&rng);
	
	std::stringstream stream(binaryDataOriginal);
	SerializerBinary::deserialize(stream, newRoot.get());
	
	std::string binaryDataNew = getBinaryRepresentation(newRoot.get());
	
	BOOST_CHECK_EQUAL(binaryDataOriginal, binaryDataNew);
}

BOOST_AUTO_TEST_SUITE_END()
