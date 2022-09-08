include(Boost)
include(App)

include_project_files(RbxG3D "*")

include_directories(include)
include_directories(../g3d/include)

list(APPEND HEADERS include/RbxG3D/RbxRay.h)
list(APPEND HEADERS include/RbxG3D/RbxTime.h)
list(APPEND HEADERS include/RbxG3D/Frustum.h)
list(APPEND HEADERS include/RbxG3D/RbxCamera.h)

list(APPEND SOURCES RbxCamera.cpp)
list(APPEND SOURCES Frustum.cpp)
list(APPEND SOURCES RbxRay.cpp)

add_library(RbxG3D OBJECT ${SOURCES} ${HEADERS})
