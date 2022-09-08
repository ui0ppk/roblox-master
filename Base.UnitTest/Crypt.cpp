#include <boost/test/unit_test.hpp>

#include "rbx/crypt.h"

using namespace boost;

BOOST_AUTO_TEST_SUITE(Crypt)

const char* message = "Hello World!";
const char* signatureBase64 = "ZLrv3Sy5zh08/+tec0tw2dMJ1JkhX/TcItuo/IuYPkP3muftzYEU3mt+uU9236Hdh2RQlUIw3me/hI06aj9KVAbS8dHSzHbF6GpkhhmwmiLW4v8XfSFb//XujR4nEadWi21mzWTVDEySJA66uotV63r3jvYmVHC+o35dBN0h5Jw=";

#ifndef RBX_PLATFORM_IOS

BOOST_AUTO_TEST_CASE(PositiveTest)
{
	RBX::Crypt crypt;
	BOOST_CHECK_NO_THROW(crypt.verifySignatureBase64(message, signatureBase64));
}

BOOST_AUTO_TEST_CASE(NegativeTest)
{
	RBX::Crypt crypt;
	BOOST_CHECK_THROW(crypt.verifySignatureBase64("Tampered message", signatureBase64), std::exception);
}
#endif

BOOST_AUTO_TEST_SUITE_END()


