#include <boost/test/unit_test.hpp>
#include "V8Xml/WebParser.h"
#include "v8tree/Instance.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE(WebParser)

	BOOST_AUTO_TEST_CASE(TestJSONEmpty)
	{
		Reflection::Variant v;
		BOOST_CHECK(RBX::WebParser::parseJSONObject("{}", v));
		BOOST_CHECK(v.isType<shared_ptr<const Reflection::ValueTable> >());
		BOOST_CHECK(v.get<shared_ptr<const Reflection::ValueTable> >()->size() == 0);

		BOOST_CHECK(RBX::WebParser::parseJSONObject("[]", v));
		BOOST_CHECK(v.isType<shared_ptr<const Reflection::ValueArray> >());
		BOOST_CHECK(v.get<shared_ptr<const Reflection::ValueArray> >()->size() == 0);
	}

	BOOST_AUTO_TEST_CASE(TestJSONBasic)
	{
		Reflection::Variant v;
		BOOST_CHECK(RBX::WebParser::parseJSONObject(
			"{ \"a\" : 0, \"b\": false, \"c\": \"True\", \"d\": {}, \"e\": [], \"f\": null}", v));
		BOOST_CHECK(v.isType<shared_ptr<const Reflection::ValueTable> >());
		shared_ptr<const Reflection::ValueTable> table = v.get<shared_ptr<const Reflection::ValueTable> >();
		BOOST_CHECK(table->size() == 6);
		Reflection::ValueTable::const_iterator it = table->find("a");
		BOOST_CHECK(it != table->end());
		BOOST_CHECK(it->second.isNumber());
		BOOST_CHECK(it->second.get<int>() == 0);

		it = table->find("b");
		BOOST_CHECK(it != table->end());
		BOOST_CHECK(it->second.isType<bool>());
		BOOST_CHECK(it->second.get<bool>() == FALSE);

		it = table->find("c");
		BOOST_CHECK(it != table->end());
		BOOST_CHECK(it->second.isType<std::string>());
		BOOST_CHECK(it->second.get<std::string>() == "True");

		it = table->find("d");
		BOOST_CHECK(it != table->end());
		BOOST_CHECK(it->second.get<shared_ptr<const Reflection::ValueTable> >()->size() == 0);

		it = table->find("e");
		BOOST_CHECK(it != table->end());
		BOOST_CHECK(it->second.isType<shared_ptr<const Reflection::ValueArray> >());
		BOOST_CHECK(it->second.get<shared_ptr<const Reflection::ValueArray> >()->size() == 0);

		it = table->find("f");
		BOOST_CHECK(it != table->end());
		BOOST_CHECK(it->second.isVoid());
	}

	BOOST_AUTO_TEST_CASE(TestJSONSub)
	{
		Reflection::Variant v;
		BOOST_CHECK(RBX::WebParser::parseJSONObject(
			"{ \"subdict\": { \"x\": true, \"y\": 0 }, \"subarray\": [0, 1, null]  }", v));

		shared_ptr<const Reflection::ValueTable> table = v.get<shared_ptr<const Reflection::ValueTable> >();
		BOOST_CHECK(table->size() == 2);
		Reflection::ValueTable::const_iterator it = table->find("subdict");
		BOOST_CHECK(it != table->end());
		BOOST_CHECK(it->second.isType<shared_ptr<const Reflection::ValueTable> >());

		shared_ptr<const Reflection::ValueTable> subTable = it->second.get<shared_ptr<const Reflection::ValueTable> >();
		BOOST_CHECK(subTable->size() == 2);

		it = subTable->find("x");
		BOOST_CHECK(it->second.isType<bool>());
		BOOST_CHECK(it->second.get<bool>() == TRUE);

		it = subTable->find("y");
		BOOST_CHECK(it->second.isType<int>());
		BOOST_CHECK(it->second.get<int>() == 0);

		it = table->find("subarray");
		BOOST_CHECK(it != table->end());
		shared_ptr<const Reflection::ValueArray> subArray = it->second.get<shared_ptr<const Reflection::ValueArray> >();
		BOOST_CHECK(subArray->size() == 3);

		BOOST_CHECK((*subArray)[0].isType<int>());
		BOOST_CHECK((*subArray)[0].get<int>() == 0);
		BOOST_CHECK((*subArray)[1].isType<int>());
		BOOST_CHECK((*subArray)[1].get<int>() == 1);
		BOOST_CHECK((*subArray)[2].isType<void>());
	}

	BOOST_AUTO_TEST_CASE(TestJSONWriteBasic)
	{
		double testValue = 4.5;
		shared_ptr<Reflection::ValueTable> table(rbx::make_shared<Reflection::ValueTable>());
		(*table)["a"] = 2;
		(*table)["b"] = testValue;
		(*table)["c"] = false;
		(*table)["d"] = shared_ptr<const Reflection::ValueTable>(rbx::make_shared<Reflection::ValueTable>());
		(*table)["e"] = shared_ptr<const Reflection::ValueArray>(rbx::make_shared<Reflection::ValueArray>());
		(*table)["f"] = RBX::Vector3();
		(*table)["g"] = std::string("test");

		Reflection::Variant v = shared_ptr<const Reflection::ValueTable>(table);

		std::string result;
		BOOST_CHECK_EQUAL(RBX::WebParser::writeJSON(v, result, RBX::WebParser::FailOnNonJSON), false);
		BOOST_CHECK(RBX::WebParser::writeJSON(v, result, RBX::WebParser::SkipNonJSON));
		// Warning: depends on implementation of particular JSON parser, might break if parser is changed
		BOOST_CHECK_EQUAL(result,"{\"g\":\"test\",\"f\":null,\"e\":[],\"d\":{},\"c\":false,\"b\":4.5,\"a\":2}"); 

		// Check roundtrip
		Reflection::Variant parsedV;
		BOOST_CHECK(RBX::WebParser::parseJSONObject(result, parsedV));
		BOOST_CHECK(parsedV.isType<shared_ptr<const Reflection::ValueTable> >());
		shared_ptr<const Reflection::ValueTable> parsedTable = parsedV.get<shared_ptr<const Reflection::ValueTable> >();
		BOOST_CHECK_EQUAL(parsedTable->find("b")->second.get<double>(),testValue);
		BOOST_CHECK(parsedTable->find("f")->second.isVoid());
	}

	BOOST_AUTO_TEST_CASE(TestJSONRountrip)
	{
		double floatValue = 4.6; // Can't be represented by double exactly
		int intValue = 123456789;
		shared_ptr<Reflection::ValueTable> table(rbx::make_shared<Reflection::ValueTable>());
		(*table)["bigInt"] = intValue;
		(*table)["float"] = floatValue;

		Reflection::Variant v = shared_ptr<const Reflection::ValueTable>(table);
		std::string result;
		BOOST_CHECK(RBX::WebParser::writeJSON(v, result));

		Reflection::Variant parsedV;
		BOOST_CHECK(RBX::WebParser::parseJSONObject(result, parsedV));
		BOOST_CHECK(parsedV.isType<shared_ptr<const Reflection::ValueTable> >());
		shared_ptr<const Reflection::ValueTable> parsedTable = parsedV.get<shared_ptr<const Reflection::ValueTable> >();
		BOOST_CHECK_EQUAL(parsedTable->find("float")->second.get<double>(),floatValue);
		BOOST_CHECK_EQUAL(parsedTable->find("bigInt")->second.get<int>(),intValue);
	}

	BOOST_AUTO_TEST_CASE(TestWebParserList)
	{	
		std::string response = "<List><Value>foo</Value><Value>bar</Value></List>";
		RBX::Reflection::Variant value;

		std::auto_ptr<std::istream> stream(new std::istringstream(response));
		BOOST_REQUIRE(RBX::WebParser::parseWebGenericResponse(*stream, value));
		BOOST_CHECK(value.isType<shared_ptr<const Reflection::ValueArray> >());
	}

	BOOST_AUTO_TEST_CASE(TestWebParserTable)
	{	
		std::string response = "<Table><Entity><Key>foo</Key><Value>foo2</Value></Entity><Entity><Key>bar</Key><Value>bar2</Value></Entity></Table>";
		RBX::Reflection::Variant value;

		std::auto_ptr<std::istream> stream(new std::istringstream(response));
		if(!RBX::WebParser::parseWebGenericResponse(*stream, value)){
			BOOST_CHECK(false);
		}
		BOOST_CHECK(value.isType<shared_ptr<const Reflection::ValueMap> >());
	}

	
	BOOST_AUTO_TEST_CASE(TestWebParserBool)
	{	
		std::string response;
		RBX::Reflection::Variant value;

		{
			response = "<Value>true</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			if(!RBX::WebParser::parseWebGenericResponse(*stream, value)){
				BOOST_CHECK(false);
			}
			BOOST_CHECK(value.isType<bool>());
		}
		{
			response = "<Value>false</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			if(!RBX::WebParser::parseWebGenericResponse(*stream, value)){
				BOOST_CHECK(false);
			}
			BOOST_CHECK(value.isType<bool>());
		}
	}

	BOOST_AUTO_TEST_CASE(TestWebParserBoolWithType)
	{	
		std::string response;
		RBX::Reflection::Variant value;

		{
			response = "<Value Type=\"boolean\">true</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			if(!RBX::WebParser::parseWebGenericResponse(*stream, value)){
				BOOST_CHECK(false);
			}
			BOOST_CHECK(value.isType<bool>());
		}
		{
			response = "<Value Type=\"boolean\">false</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(*stream, value));
			BOOST_CHECK(value.isType<bool>());
		}

		{
			response = "<Value Type=\"boolean\">False</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(*stream, value));
			BOOST_CHECK(value.isType<bool>());
		}
		{
			response = "<Value Type=\"boolean\">True</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(*stream, value));
			BOOST_CHECK(value.isType<bool>());
		}
		{
			response = "<Value Type=\"boolean\">TRUE</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(*stream, value));
			BOOST_CHECK(value.isType<bool>());
		}
		{
			response = "<Value Type=\"boolean\">FALSE</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(*stream, value));
			BOOST_CHECK(value.isType<bool>());
		}
		{
			response = "<Value Type=\"boolean\">foo</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(!RBX::WebParser::parseWebGenericResponse(*stream, value));
		}
		{
			response = "<Value Type=\"boolean\">bar</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(!RBX::WebParser::parseWebGenericResponse(*stream, value));
		}
	}

	BOOST_AUTO_TEST_CASE(TestWebParserNumberWithType)
	{	
		std::string response;
		RBX::Reflection::Variant value;

		{
			response = "<Value Type=\"number\">0.12321</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(*stream, value));
			BOOST_CHECK(value.isType<double>());
		}
		{
			response = "<Value Type=\"number\">1241241</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(*stream, value));
			BOOST_CHECK(value.isType<double>());
		}

		{
			response = "<Value Type=\"number\">1234aba</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(*stream, value));
			BOOST_CHECK(value.isType<double>());
		}
		{
			response = "<Value Type=\"number\">fdklajsd980f</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(*stream, value));
		}
	}

	BOOST_AUTO_TEST_CASE(TestWebParserString)
	{	
		std::string response;
		RBX::Reflection::Variant value;

		{
			response = "<Value>Foo</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			if(!RBX::WebParser::parseWebGenericResponse(*stream, value)){
				BOOST_CHECK(false);
			}
			BOOST_CHECK(value.isType<std::string>());
		}
		{
			response = "<Value>Bar</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			if(!RBX::WebParser::parseWebGenericResponse(*stream, value)){
				BOOST_CHECK(false);
			}
			BOOST_CHECK(value.isType<std::string>());
		}
	}

	BOOST_AUTO_TEST_CASE(TestWebParserStringWithType)
	{	
		std::string response;
		RBX::Reflection::Variant value;

		{
			response = "<Value Type=\"string\">Foo</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(*stream, value));
			BOOST_CHECK(value.isType<std::string>());
		}
		{
			response = "<Value Type=\"string\">Bar</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(*stream, value));
			BOOST_CHECK(value.isType<std::string>());
		}
	}

	BOOST_AUTO_TEST_CASE(TestWebParserInstanceWithType)
	{	
		std::string response;
		RBX::Reflection::Variant value;

		{
			response = "<Value Type=\"instance\">"
	"<roblox xmlns:xmime=\"http://www.w3.org/2005/05/xmlmime\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://www.roblox.com/roblox.xsd\" version=\"4\">"
	"<External>null</External>								"
	"<External>nil</External>								"
	"<Item class=\"Part\" referent=\"RBX0\">				"
	"	<Properties>										 "
	"		<bool name=\"Anchored\">false</bool>			 "
	"		<float name=\"BackParamA\">-0.5</float>			  "
	"		<float name=\"BackParamB\">0.5</float>			  "
	"		<token name=\"BackSurface\">0</token>			   "
	"		<token name=\"BackSurfaceInput\">0</token>		   "
	"		<float name=\"BottomParamA\">-0.5</float>		    "
	"		<float name=\"BottomParamB\">0.5</float>		    "
	"		<token name=\"BottomSurface\">4</token>			     "
	"		<token name=\"BottomSurfaceInput\">0</token>	     "
	"		<int name=\"BrickColor\">194</int>					  "
	"		<CoordinateFrame name=\"CFrame\">					  "
	"			<X>0</X>										   "
	"			<Y>0.600000024</Y>								   "
	"			<Z>0</Z>										    "
	"			<R00>1</R00>									    "
	"			<R01>0</R01>										 "
	"			<R02>0</R02>										 "
	"			<R10>0</R10>										  "
	"			<R11>1</R11>										  "
	"			<R12>0</R12>										   "
	"			<R20>0</R20>										   "
	"			<R21>0</R21>											"
	"			<R22>1</R22>											"
	"		</CoordinateFrame>											 "
	"		<bool name=\"CanCollide\">true</bool>						 "
	"		<bool name=\"DraggingV1\">false</bool>						  "
	"		<float name=\"Elasticity\">0.5</float>						  "
	"		<token name=\"FormFactor\">1</token>						   "
	"		<float name=\"Friction\">0.300000012</float>				   "
	"		<float name=\"FrontParamA\">-0.5</float>					    "
	"		<float name=\"FrontParamB\">0.5</float>						    "
	"		<token name=\"FrontSurface\">0</token>						     "
	"		<token name=\"FrontSurfaceInput\">0</token>					     "
	"		<float name=\"LeftParamA\">-0.5</float>							  "
	"		<float name=\"LeftParamB\">0.5</float>							  "
	"		<token name=\"LeftSurface\">0</token>							   "
	"		<token name=\"LeftSurfaceInput\">0</token>						   "
	"		<bool name=\"Locked\">false</bool>								    "
	"		<token name=\"Material\">256</token>							    "
	"		<string name=\"Name\">Part</string>									 "
	"		<float name=\"Reflectance\">0</float>								 "
	"		<float name=\"RightParamA\">-0.5</float>							  "
	"		<float name=\"RightParamB\">0.5</float>								  "
	"		<token name=\"RightSurface\">0</token>								   "
	"		<token name=\"RightSurfaceInput\">0</token>							   "
	"		<Vector3 name=\"RotVelocity\">											"
	"			<X>0</X>															"
	"			<Y>0</Y>															 "
	"			<Z>0</Z>															 "
	"		</Vector3>																  "
	"		<float name=\"TopParamA\">-0.5</float>									  "
	"		<float name=\"TopParamB\">0.5</float>									   "
	"		<token name=\"TopSurface\">3</token>									   "
	"		<token name=\"TopSurfaceInput\">0</token>								    "
	"		<float name=\"Transparency\">0.319999993</float>						    "
	"		<Vector3 name=\"Velocity\">												     "
	"			<X>0</X>															     "
	"			<Y>0</Y>																  "
	"			<Z>0</Z>																  "
	"		</Vector3>																	   "
	"		<bool name=\"archivable\">true</bool>										   "
	"		<token name=\"shape\">1</token>												    "
	"		<Vector3 name=\"size\">														    "
	"			<X>4</X>																	 "
	"			<Y>1.20000005</Y>															 "
	"			<Z>2</Z>																	  "
	"		</Vector3>																		  "
	"	</Properties>																		   "
	"</Item>																				   "
	"</roblox>"
	"</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(RBX::WebParser::parseWebGenericResponse(*stream, value));
			BOOST_CHECK(value.isType<shared_ptr<Instance> >());
		}

		{
			response = "<Value Type=\"instance\">"
	"<roblox xmlns:xmime=\"http://www.w3.org/2005/05/xmlmime\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://www.roblox.com/roblox.xsd\" version=\"4\">"
	"<External>null</External>								"
	"<External>nil</External>								"
	"<Item class=\"Part\" referent=\"RBX0\">				"
	"	<Properties>										 "
	"		<bool name=\"Anchored\">false</bool>			 "
	"		<float name=\"BackParamA\">-0.5</float>			  "
	"		<float name=\"BackParamB\">0.5</float>			  "
	"		<token name=\"BackSurface\">0</token>			   "
	"		<token name=\"BackSurfaceInput\">0</token>		   "
	"		<float name=\"BottomParamA\">-0.5</float>		    "
	"		<float name=\"BottomParamB\">0.5</float>		    "
	"		<token name=\"BottomSurface\">4</token>			     "
	"		<token name=\"BottomSurfaceInput\">0</token>	     "
	"		<int name=\"BrickColor\">194</int>					  "
	"		<CoordinateFrame name=\"CFrame\">					  "
	"			<X>0</X>										   "
	"			<Y>0.600000024</Y>								   "
	"			<Z>0</Z>										    "
	"			<R00>1</R00>									    "
	"			<R01>0</R01>										 "
	"			<R02>0</R02>										 "
	"			<R10>0</R10>										  "
	"			<R11>1</R11>										  "
	"			<R12>0</R12>										   "
	"			<R20>0</R20>										   "
	"			<R21>0</R21>											"
	"			<R22>1</R22>											"
	"		</CoordinateFrame>											 "
	"		<bool name=\"CanCollide\">true</bool>						 "
	"		<bool name=\"DraggingV1\">false</bool>						  "
	"		<float name=\"Elasticity\">0.5</float>						  "
	"		<token name=\"FormFactor\">1</token>						   "
	"		<float name=\"Friction\">0.300000012</float>				   "
	"		<float name=\"FrontParamA\">-0.5</float>					    "
	"		<float name=\"FrontParamB\">0.5</float>						    "
	"		<token name=\"FrontSurface\">0</token>						     "
	"		<token name=\"FrontSurfaceInput\">0</token>					     "
	"		<float name=\"LeftParamA\">-0.5</float>							  "
	"		<float name=\"LeftParamB\">0.5</float>							  "
	"		<token name=\"LeftSurface\">0</token>							   "
	"		<token name=\"LeftSurfaceInput\">0</token>						   "
	"		<bool name=\"Locked\">false</bool>								    "
	"		<token name=\"Material\">256</token>							    "
	"		<string name=\"Name\">Part</string>									 "
	"		<float name=\"Reflectance\">0</float>								 "
	"		<float name=\"RightParamA\">-0.5</float>							  "
	"		<float name=\"RightParamB\">0.5</float>								  "
	"		<token name=\"RightSurface\">0</token>								   "
	"		<token name=\"RightSurfaceInput\">0</token>							   "
	"		<Vector3 name=\"RotVelocity\">											"
	"			<X>0</X>															"
	"			<Y>0</Y>															 "
	"			<Z>0</Z>															 "
	"		</Vector3>																  "
	"		<float name=\"TopParamA\">-0.5</float>									  "
	"		<float name=\"TopParamB\">0.5</float>									   "
	"		<token name=\"TopSurface\">3</token>									   "
	"		<token name=\"TopSurfaceInput\">0</token>								    "
	"		<float name=\"Transparency\">0.319999993</float>						    "
	"		<Vector3 name=\"Velocity\">												     "
	"			<X>0</X>															     "
	"			<Y>0</Y>																  "
	"			<Z>0</Z>																  "
	"		</Vector3>																	   "
	"		<bool name=\"archivable\">true</bool>										   "
	"		<token name=\"shape\">1</token>												    "
	"		<Vector3 name=\"size\">														    "
	"			<X>4</X>																	 "
	"			<Y>1.20000005</Y>															 "
	"			<Z>2</Z>																	  "
	"		</Vector3>																		  "
	"	</Properties>																		   "
	"</Item>																				   "
	"<Item class=\"Part\" referent=\"RBX1\">				"
	"	<Properties>										 "
	"		<bool name=\"Anchored\">false</bool>			 "
	"		<float name=\"BackParamA\">-0.5</float>			  "
	"		<float name=\"BackParamB\">0.5</float>			  "
	"		<token name=\"BackSurface\">0</token>			   "
	"		<token name=\"BackSurfaceInput\">0</token>		   "
	"		<float name=\"BottomParamA\">-0.5</float>		    "
	"		<float name=\"BottomParamB\">0.5</float>		    "
	"		<token name=\"BottomSurface\">4</token>			     "
	"		<token name=\"BottomSurfaceInput\">0</token>	     "
	"		<int name=\"BrickColor\">194</int>					  "
	"		<CoordinateFrame name=\"CFrame\">					  "
	"			<X>0</X>										   "
	"			<Y>0.600000024</Y>								   "
	"			<Z>0</Z>										    "
	"			<R00>1</R00>									    "
	"			<R01>0</R01>										 "
	"			<R02>0</R02>										 "
	"			<R10>0</R10>										  "
	"			<R11>1</R11>										  "
	"			<R12>0</R12>										   "
	"			<R20>0</R20>										   "
	"			<R21>0</R21>											"
	"			<R22>1</R22>											"
	"		</CoordinateFrame>											 "
	"		<bool name=\"CanCollide\">true</bool>						 "
	"		<bool name=\"DraggingV1\">false</bool>						  "
	"		<float name=\"Elasticity\">0.5</float>						  "
	"		<token name=\"FormFactor\">1</token>						   "
	"		<float name=\"Friction\">0.300000012</float>				   "
	"		<float name=\"FrontParamA\">-0.5</float>					    "
	"		<float name=\"FrontParamB\">0.5</float>						    "
	"		<token name=\"FrontSurface\">0</token>						     "
	"		<token name=\"FrontSurfaceInput\">0</token>					     "
	"		<float name=\"LeftParamA\">-0.5</float>							  "
	"		<float name=\"LeftParamB\">0.5</float>							  "
	"		<token name=\"LeftSurface\">0</token>							   "
	"		<token name=\"LeftSurfaceInput\">0</token>						   "
	"		<bool name=\"Locked\">false</bool>								    "
	"		<token name=\"Material\">256</token>							    "
	"		<string name=\"Name\">Part</string>									 "
	"		<float name=\"Reflectance\">0</float>								 "
	"		<float name=\"RightParamA\">-0.5</float>							  "
	"		<float name=\"RightParamB\">0.5</float>								  "
	"		<token name=\"RightSurface\">0</token>								   "
	"		<token name=\"RightSurfaceInput\">0</token>							   "
	"		<Vector3 name=\"RotVelocity\">											"
	"			<X>0</X>															"
	"			<Y>0</Y>															 "
	"			<Z>0</Z>															 "
	"		</Vector3>																  "
	"		<float name=\"TopParamA\">-0.5</float>									  "
	"		<float name=\"TopParamB\">0.5</float>									   "
	"		<token name=\"TopSurface\">3</token>									   "
	"		<token name=\"TopSurfaceInput\">0</token>								    "
	"		<float name=\"Transparency\">0.319999993</float>						    "
	"		<Vector3 name=\"Velocity\">												     "
	"			<X>0</X>															     "
	"			<Y>0</Y>																  "
	"			<Z>0</Z>																  "
	"		</Vector3>																	   "
	"		<bool name=\"archivable\">true</bool>										   "
	"		<token name=\"shape\">1</token>												    "
	"		<Vector3 name=\"size\">														    "
	"			<X>4</X>																	 "
	"			<Y>1.20000005</Y>															 "
	"			<Z>2</Z>																	  "
	"		</Vector3>																		  "
	"	</Properties>																		   "
	"</Item>																				   "
	"</roblox>"
	"</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(!RBX::WebParser::parseWebGenericResponse(*stream, value));
		}
		{
			response = "<Value Type=\"instance\">Bar</Value>";
			std::auto_ptr<std::istream> stream(new std::istringstream(response));
			BOOST_CHECK(!RBX::WebParser::parseWebGenericResponse(*stream, value));
		}
	}

BOOST_AUTO_TEST_SUITE_END()
