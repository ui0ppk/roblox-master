file(TO_CMAKE_PATH "${CONTRIB_PATH}/android/arm/openssl/openssl-1.0.2c" SSL_ROOT)
include_directories("${SSL_ROOT}/include")
link_directories("${SSL_ROOT}/lib")
