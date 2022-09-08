include(App)

include_directories(include)
include_directories(../g3d/include)
include_directories(../RbxG3D/include)
include_directories(../GfxBase/include)

list(APPEND HEADERS include/appdraw/Draw.h)
list(APPEND HEADERS include/appdraw/HitTest.h)
list(APPEND HEADERS include/appdraw/DrawAdorn.h)
list(APPEND HEADERS include/appdraw/HandleType.h)
list(APPEND HEADERS include/appdraw/DrawPrimitives.h)

list(APPEND SOURCES Draw.cpp)
list(APPEND SOURCES DrawAdorn.cpp)
list(APPEND SOURCES HitTest.cpp)

add_library(AppDraw OBJECT ${SOURCES} ${HEADERS})
