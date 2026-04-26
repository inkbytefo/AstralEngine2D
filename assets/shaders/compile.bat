@echo off
REM SDL_GPU Shader Compilation Script
REM SDL_GPU, her platform için farklı shader formatı gerektirir:
REM - Windows: DXIL (DirectX 12)
REM - macOS/iOS: Metal
REM - Linux/Android: SPIRV (Vulkan)

REM DXC (DirectX Shader Compiler) kullanarak DXIL'e compile et
REM DXC'yi indirmek için: https://github.com/microsoft/DirectXShaderCompiler/releases

echo Compiling shaders for SDL_GPU...

REM Vertex Shader
dxc -T vs_6_0 -E main basic_3d.vert.hlsl -Fo basic_3d.vert.dxil
echo Compiled: basic_3d.vert.dxil

REM Fragment Shader
dxc -T ps_6_0 -E main basic_3d.frag.hlsl -Fo basic_3d.frag.dxil
echo Compiled: basic_3d.frag.dxil

echo.
echo Shader compilation complete!
echo.
echo NOTES:
echo - DXIL shaders work on Windows (DirectX 12)
echo - For cross-platform, use SDL_shadercross or compile to SPIRV/Metal
echo - SDL_GPU will automatically select the correct backend
echo.
pause

