file(TO_CMAKE_PATH "${CONTRIB_PATH}/google-breakpad/20MAY2014" BREAKPAD_SRC_ROOT)
include_directories("${BREAKPAD_SRC_ROOT}/src")

if(ANDROID)
  include_directories("${BREAKPAD_SRC_ROOT}/src/common/android/include")
endif(ANDROID)

file(TO_CMAKE_PATH "${CONTRIB_PATH}/android/arm/google-breakpad/20MAY2014/lib" BREAKPAD_LIB_ROOT)
