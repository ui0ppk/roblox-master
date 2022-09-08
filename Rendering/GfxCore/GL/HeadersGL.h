#pragma once

#ifdef RBX_PLATFORM_IOS
    #include <OpenGLES/ES2/gl.h>
    #include <OpenGLES/ES2/glext.h>
    #ifdef __OBJC__
        #include <OpenGLES/EAGL.h>
    #endif

    #define GLES
#elif defined(__ANDROID__)
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>

    #define GLES
#else
    #include <GL/glew.h>

    void glewInitRBX();
#endif

#ifdef __ANDROID__
typedef struct __GLsync *GLsync;
typedef uint64_t GLuint64;
#endif

#ifdef GLES
    // Core GLES
    #define glClearDepth glClearDepthf

    // OES_vertex_array_object
    GLvoid glBindVertexArray(GLuint array);
    GLvoid glDeleteVertexArrays(GLsizei n, const GLuint *arrays);
    GLvoid glGenVertexArrays(GLsizei n, GLuint *arrays);

    // OES_mapbuffer
    #define GL_WRITE_ONLY 0x88B9
    GLvoid *glMapBuffer(GLenum target, GLenum access);
    GLboolean glUnmapBuffer(GLenum target);

    // EXT_map_buffer_range
    GLvoid *glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
    #define GL_MAP_READ_BIT 0x0001
    #define GL_MAP_WRITE_BIT 0x0002
    #define GL_MAP_INVALIDATE_RANGE_BIT 0x0004
    #define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
    #define GL_MAP_FLUSH_EXPLICIT_BIT 0x0010
    #define GL_MAP_UNSYNCHRONIZED_BIT 0x0020

    // EXT_texture_storage
    GLvoid glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
    #define GL_LUMINANCE8 0x8040
    #define GL_LUMINANCE8_ALPHA8 0x8045

    // GL_OES_packed_depth_stencil
    #define GL_DEPTH_STENCIL 0x84F9
    #define GL_UNSIGNED_INT_24_8 0x84FA
    #define GL_DEPTH24_STENCIL8 0x88F0

    // OES_rgb8_rgba8
    #define GL_RGBA8 0x8058

    // OES_texture_half_float
    #define GL_HALF_FLOAT 0x8D61

    // EXT_color_buffer_half_float
    #define GL_RGBA16F 0x881A

    // OES_texture_3D
    #define GL_TEXTURE_3D 0x806F
    GLvoid glTexImage3D(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    GLvoid glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
    GLvoid glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

	// GLES 3.0 formats
	#define GL_RED 0x1903
	#define GL_RG 0x8227
	#define GL_R8 0x8229
	#define GL_RG8 0x822B
	#define GL_RG16 0x822C

    // GLES 3.0 buffer objects
    #define GL_DYNAMIC_COPY 0x88EA
    #define GL_PIXEL_UNPACK_BUFFER 0x88EC

    // GLES 3.0 sync objects
    #define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
    #define GL_ALREADY_SIGNALED 0x911A
    #define GL_TIMEOUT_EXPIRED 0x911B
    #define GL_CONDITION_SATISFIED 0x911C
    #define GL_WAIT_FAILED 0x911D
    #define GL_SYNC_FLUSH_COMMANDS_BIT 0x00000001
    GLsync glFenceSync(GLenum condition, GLbitfield flags);
    void glDeleteSync(GLsync sync);
    GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout);
    void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout);

	// GLES 3.0 MSAA
	#define GL_READ_FRAMEBUFFER 0x8CA8
	#define GL_DRAW_FRAMEBUFFER 0x8CA9
	#define GL_MAX_SAMPLES 0x8D57
	GLvoid glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
	GLvoid glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

	// GLES 3.0 invalidate FBO
	GLvoid glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments);
#endif

// EXT_texture_compression_s3tc
#ifndef GL_EXT_texture_compression_s3tc
#define GL_EXT_texture_compression_s3tc 1

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

// IMG_texture_compression_pvrtc
#ifndef GL_IMG_texture_compression_pvrtc
#define GL_IMG_texture_compression_pvrtc 1

#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03
#endif

#ifndef GL_ETC1_RGB8_OES
#define GL_ETC1_RGB8_OES           (0x8D64)
#endif

// APPLE_texture_max_level, GLES 3.0
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif
