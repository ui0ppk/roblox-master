include(Rendering)

include_project_files(GfxCore "*")

include_directories(include)
include_directories(glew)
include_directories(GL)

if(ANDROID)
  list(APPEND SOURCES GL/ContextGLAndroid.cpp)
elseif(WIN32)
  list(APPEND SOURCES GL/ContextGLWin32.cpp)
elseif(APPLE)
  list(APPEND SOURCES GL/ContextGLEAGL.mm)
  list(APPEND SOURCES GL/ContextGLNSGL.mm)
endif(ANDROID)

list(APPEND HEADERS GL/ContextGL.h)
list(APPEND HEADERS GL/DeviceGL.h)
list(APPEND HEADERS GL/FramebufferGL.h)
list(APPEND HEADERS GL/GeometryGL.h)
list(APPEND HEADERS GL/HeadersGL.h)
list(APPEND HEADERS GL/ShaderGL.h)
list(APPEND HEADERS GL/TextureGL.h)
list(APPEND HEADERS glew/GL/glew.h)
list(APPEND HEADERS glew/GL/glxew.h)
list(APPEND HEADERS glew/GL/wglew.h)
list(APPEND HEADERS include/GfxCore/Device.h)
list(APPEND HEADERS include/GfxCore/Framebuffer.h)
list(APPEND HEADERS include/GfxCore/Geometry.h)
list(APPEND HEADERS include/GfxCore/Resource.h)
list(APPEND HEADERS include/GfxCore/Shader.h)
list(APPEND HEADERS include/GfxCore/States.h)
list(APPEND HEADERS include/GfxCore/Texture.h)
list(APPEND HEADERS include/GfxCore/pix.h)

list(APPEND SOURCES Device.cpp)
list(APPEND SOURCES DeviceCreate.cpp)
list(APPEND SOURCES Framebuffer.cpp)
list(APPEND SOURCES GL/CardboardVRGL.cpp)
list(APPEND SOURCES GL/DeviceContextGL.cpp)
list(APPEND SOURCES GL/DeviceGL.cpp)
list(APPEND SOURCES GL/FramebufferGL.cpp)
list(APPEND SOURCES GL/GeometryGL.cpp)
list(APPEND SOURCES GL/ShaderGL.cpp)
list(APPEND SOURCES GL/TextureGL.cpp)
list(APPEND SOURCES Geometry.cpp)
list(APPEND SOURCES Resource.cpp)
list(APPEND SOURCES Shader.cpp)
list(APPEND SOURCES States.cpp)
list(APPEND SOURCES Texture.cpp)
list(APPEND SOURCES glew/glew.cpp)
list(APPEND SOURCES pix.cpp)

add_library(GfxCore OBJECT ${SOURCES} ${HEADERS})
