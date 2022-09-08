@echo off
"%~dp0\Rendering\ShaderCompiler\ShaderCompiler.exe" /P "%~dp0\shaders" d3d9 d3d11 glsl glsl3 glsles glsles3 && ^
"%~dp0\Rendering\ShaderCompiler\ShaderCompiler_x64.exe" /P "%~dp0\shaders" d3d11_durango
