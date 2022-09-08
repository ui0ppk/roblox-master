#include "FastLog.h"

#pragma comment(lib, "opengl32.lib")

FASTFLAG(GraphicsGL3)

#include "glew.c"

template <typename T, typename U> static void patchUnsafe(T& t, U& u)
{
    if (!t && u)
		t = reinterpret_cast<T>(u);
}

template <typename T> static void patch(T& t, T& u)
{
    if (!t && u)
		t = u;
}

void patchLegacyFunctions()
{
#define P_APPLE(name) patch(name, name##APPLE)
#define P_EXT(name) patch(name, name##EXT)
#define P_ARB(name) patch(name, name##ARB)

    // The rendering code generally relies on OpenGL 2.1+ APIs (and sometimes OpenGL 3.0+).
    // Some of these APIs are available as extensions with different entrypoints but matching semantics.
    // To work around this we reroute core entrypoints to extension-specific entrypoints if core one is unavailable.

    if (!FFlag::GraphicsGL3)
    {
    	// APPLE_vertex_array_object
    	P_APPLE(glBindVertexArray);
    	P_APPLE(glDeleteVertexArrays);
    	patchUnsafe(glGenVertexArrays, glGenVertexArraysAPPLE); // glGenVertexArraysAPPLE has a const-correctness bug
    	P_APPLE(glIsVertexArray);
    }

	// EXT_framebuffer_object
    P_EXT(glBindFramebuffer);
    P_EXT(glBindRenderbuffer);
    P_EXT(glCheckFramebufferStatus);
    P_EXT(glDeleteFramebuffers);
    P_EXT(glDeleteRenderbuffers);
    P_EXT(glFramebufferRenderbuffer);
    P_EXT(glFramebufferTexture1D);
    P_EXT(glFramebufferTexture2D);
    P_EXT(glFramebufferTexture3D);
    P_EXT(glGenFramebuffers);
    P_EXT(glGenRenderbuffers);
    P_EXT(glGenerateMipmap);
    P_EXT(glGetFramebufferAttachmentParameteriv);
    P_EXT(glGetRenderbufferParameteriv);
    P_EXT(glIsFramebuffer);
    P_EXT(glIsRenderbuffer);
    P_EXT(glRenderbufferStorage);

    // ARB_draw_buffers
    P_ARB(glDrawBuffers);

    // EXT_texture3D
	patchUnsafe(glTexImage3D, glTexImage3DEXT); // glTexImage3DEXT uses GLenum internalFormat instead of GLint
	P_EXT(glTexSubImage3D);

    // ARB_texture_compression
    P_ARB(glCompressedTexImage1D);
    P_ARB(glCompressedTexImage2D);
    P_ARB(glCompressedTexImage3D);
    P_ARB(glCompressedTexSubImage1D);
    P_ARB(glCompressedTexSubImage2D);
    P_ARB(glCompressedTexSubImage3D);
    P_ARB(glGetCompressedTexImage);

    // ARB_shader_objects
    patch(glAttachShader, glAttachObjectARB);
    P_ARB(glCompileShader);
    patch(glCreateProgram, glCreateProgramObjectARB);
    patch(glCreateShader, glCreateShaderObjectARB);
    patch(glDeleteProgram, glDeleteObjectARB);
    patch(glDeleteShader, glDeleteObjectARB);
	patch(glDetachShader, glDetachObjectARB);
    P_ARB(glGetActiveUniform);
	patch(glGetAttachedShaders, glGetAttachedObjectsARB);
	patch(glGetShaderInfoLog, glGetInfoLogARB);
	patch(glGetProgramInfoLog, glGetInfoLogARB);
	patch(glGetShaderiv, glGetObjectParameterivARB);
	patch(glGetProgramiv, glGetObjectParameterivARB);
    P_ARB(glGetShaderSource);
    P_ARB(glGetUniformLocation);
    P_ARB(glGetUniformfv);
    P_ARB(glGetUniformiv);
    P_ARB(glLinkProgram);
    patchUnsafe(glShaderSource, glShaderSourceARB); // glShaderSourceARB has a const-correctness bug
    P_ARB(glUniform1f);
    P_ARB(glUniform1fv);
    P_ARB(glUniform1i);
    P_ARB(glUniform1iv);
    P_ARB(glUniform2f);
    P_ARB(glUniform2fv);
    P_ARB(glUniform2i);
    P_ARB(glUniform2iv);
    P_ARB(glUniform3f);
    P_ARB(glUniform3fv);
    P_ARB(glUniform3i);
    P_ARB(glUniform3iv);
    P_ARB(glUniform4f);
    P_ARB(glUniform4fv);
    P_ARB(glUniform4i);
    P_ARB(glUniform4iv);
    P_ARB(glUniformMatrix2fv);
    P_ARB(glUniformMatrix3fv);
    P_ARB(glUniformMatrix4fv);
	patch(glUseProgram, glUseProgramObjectARB);
    P_ARB(glValidateProgram);

    // ARB_vertex_buffer_object
    P_ARB(glBindBuffer);
    P_ARB(glBufferData);
    P_ARB(glBufferSubData);
    P_ARB(glDeleteBuffers);
    P_ARB(glGenBuffers);
    P_ARB(glGetBufferParameteriv);
    P_ARB(glGetBufferPointerv);
    P_ARB(glGetBufferSubData);
    P_ARB(glIsBuffer);
    P_ARB(glMapBuffer);
    P_ARB(glUnmapBuffer);

    // ARB_vertex_shader
    P_ARB(glVertexAttribPointer);
    P_ARB(glEnableVertexAttribArray);
    P_ARB(glDisableVertexAttribArray);
    P_ARB(glBindAttribLocation);
    P_ARB(glGetActiveAttrib);
    P_ARB(glGetAttribLocation);
    P_ARB(glGetVertexAttribPointerv);

    // EXT_framebuffer_multisample
    P_EXT(glRenderbufferStorageMultisample);

#undef P_ARB
#undef P_EXT
#undef P_APPLE
}

void glewInitRBX()
{
    glewInit();
    
    if (FFlag::GraphicsGL3)
    {
        // GLEW does not load core functions automatically on core contexts.
        // We could set glewExperimental but that causes GLEW to try every single entrypoint which is not very safe.
        _glewInit_GL_ARB_framebuffer_object();
        _glewInit_GL_ARB_map_buffer_range();
        _glewInit_GL_ARB_texture_storage();
        _glewInit_GL_ARB_timer_query();
        _glewInit_GL_ARB_vertex_array_object();
        _glewInit_GL_ARB_sync();
    }

    patchLegacyFunctions();
}
