include(Rendering)

include_directories(../GfxAdapters)
include_directories(../../ClientShared)

#add_subdirectory(freetype)


add_definitions(-DFT2_BUILD_LIBRARY)

include_directories(freetype/include)
include_directories(freetype/src)

file(GLOB PUBLIC_HEADERS "freetype/include/ft2build.h" "include/freetype/*.h")
file(GLOB PUBLIC_CONFIG_HEADERS "freetype/include/freetype/config/*.h")
file(GLOB PRIVATE_HEADERS "freetype/include/freetype/internal/*.h")

set(BASE_SRCS
  freetype/src/autofit/autofit.c
  freetype/src/base/ftbase.c
  freetype/src/base/ftbbox.c
  freetype/src/base/ftbdf.c
  freetype/src/base/ftbitmap.c
  freetype/src/base/ftcid.c
  freetype/src/base/ftfntfmt.c
  freetype/src/base/ftfstype.c
  freetype/src/base/ftgasp.c
  freetype/src/base/ftglyph.c
  freetype/src/base/ftgxval.c
  freetype/src/base/ftinit.c
  freetype/src/base/ftlcdfil.c
  freetype/src/base/ftmm.c
  freetype/src/base/ftotval.c
  freetype/src/base/ftpatent.c
  freetype/src/base/ftpfr.c
  freetype/src/base/ftstroke.c
  freetype/src/base/ftsynth.c
  freetype/src/base/ftsystem.c
  freetype/src/base/fttype1.c
  freetype/src/base/ftwinfnt.c
  freetype/src/bdf/bdf.c
  freetype/src/bzip2/ftbzip2.c
  freetype/src/cache/ftcache.c
  freetype/src/cff/cff.c
  freetype/src/cid/type1cid.c
  freetype/src/gzip/ftgzip.c
  freetype/src/lzw/ftlzw.c
  freetype/src/pcf/pcf.c
  freetype/src/pfr/pfr.c
  freetype/src/psaux/psaux.c
  freetype/src/pshinter/pshinter.c
  freetype/src/psnames/psnames.c
  freetype/src/raster/raster.c
  freetype/src/sfnt/sfnt.c
  freetype/src/smooth/smooth.c
  freetype/src/truetype/truetype.c
  freetype/src/type1/type1.c
  freetype/src/type42/type42.c
  freetype/src/winfonts/winfnt.c
)

list(APPEND HEADERS LightObject.h)
list(APPEND HEADERS VertexStreamer.h)
list(APPEND HEADERS ShaderManager.h)
list(APPEND HEADERS ParticleEmitter.h)
list(APPEND HEADERS Emitter.h)
list(APPEND HEADERS TextureRef.h)
list(APPEND HEADERS SceneManager.h)
list(APPEND HEADERS RenderNode.h)
list(APPEND HEADERS SSAO.h)
list(APPEND HEADERS MegaCluster.h)
list(APPEND HEADERS MaterialGenerator.h)
list(APPEND HEADERS TextureManager.h)
list(APPEND HEADERS RenderQueue.h)
list(APPEND HEADERS SpatialGrid.h)
list(APPEND HEADERS GlobalShaderData.h)
list(APPEND HEADERS TextureCompositor.h)
list(APPEND HEADERS RenderView.h)
list(APPEND HEADERS Sky.h)
list(APPEND HEADERS Util.h)
list(APPEND HEADERS Material.h)
list(APPEND HEADERS LightGrid.h)
list(APPEND HEADERS TypesetterBitmap.h)
list(APPEND HEADERS SpatialHashedScene.h)
list(APPEND HEADERS RenderCamera.h)
list(APPEND HEADERS Water.h)
list(APPEND HEADERS EmitterShared.h)
list(APPEND HEADERS GeometryGenerator.h)
list(APPEND HEADERS VisualEngine.h)
list(APPEND HEADERS SceneUpdater.h)
list(APPEND HEADERS ScreenSpaceEffect.h)
list(APPEND HEADERS Image.h)
list(APPEND HEADERS FastCluster.h)
list(APPEND HEADERS CullableSceneNode.h)
list(APPEND HEADERS AdornRender.h)
list(APPEND HEADERS SuperCluster.h)
list(APPEND HEADERS ObjectExporter.h)
list(APPEND HEADERS EnvMap.h)
list(APPEND HEADERS ExplosionEmitter.h)
list(APPEND HEADERS CustomEmitter.h)
list(APPEND HEADERS SmoothCluster.h)
list(APPEND HEADERS TextureAtlas.h)
list(APPEND HEADERS TypesetterDynamic.h)

list(APPEND SOURCES RenderQueue.cpp)
list(APPEND SOURCES RenderView.cpp)
list(APPEND SOURCES SpatialHashedScene.cpp)
list(APPEND SOURCES ParticleEmitter.cpp)
list(APPEND SOURCES TextureRef.cpp)
list(APPEND SOURCES GlobalShaderData.cpp)
list(APPEND SOURCES SSAO.cpp)
list(APPEND SOURCES RenderNode.cpp)
list(APPEND SOURCES Material.cpp)
list(APPEND SOURCES Image.cpp)
list(APPEND SOURCES CullableSceneNode.cpp)
list(APPEND SOURCES FastCluster.cpp)
list(APPEND SOURCES RenderCamera.cpp)
list(APPEND SOURCES TextureManager.cpp)
list(APPEND SOURCES LightGrid.cpp)
list(APPEND SOURCES MaterialGenerator.cpp)
list(APPEND SOURCES MegaCluster.cpp)
list(APPEND SOURCES TextureCompositor.cpp)
list(APPEND SOURCES GeometryGenerator.cpp)
list(APPEND SOURCES Sky.cpp)
list(APPEND SOURCES VertexStreamer.cpp)
list(APPEND SOURCES AdornRender.cpp)
list(APPEND SOURCES Water.cpp)
list(APPEND SOURCES Emitter.cpp)
list(APPEND SOURCES stdafx.cpp)
list(APPEND SOURCES LightObject.cpp)
list(APPEND SOURCES TypesetterBitmap.cpp)
list(APPEND SOURCES SceneManager.cpp)
list(APPEND SOURCES ScreenSpaceEffect.cpp)
list(APPEND SOURCES VisualEngine.cpp)
list(APPEND SOURCES ShaderManager.cpp)
list(APPEND SOURCES SceneUpdater.cpp)
list(APPEND SOURCES SuperCluster.cpp)
list(APPEND SOURCES ObjectExporter.cpp)
list(APPEND SOURCES EnvMap.cpp)
list(APPEND SOURCES ExplosionEmitter.cpp)
list(APPEND SOURCES CustomEmitter.cpp)
list(APPEND SOURCES SmoothCluster.cpp)
list(APPEND HEADERS TextureAtlas.cpp)
list(APPEND HEADERS TypesetterDynamic.cpp)

add_library(GfxRender OBJECT ${PUBLIC_HEADERS} ${PUBLIC_CONFIG_HEADERS} ${PRIVATE_HEADERS} ${BASE_SRCS} ${SOURCES} ${HEADERS})
