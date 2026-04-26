@echo off
REM SDL_GPU SPIRV Shader Compilation Script
REM glslc (Vulkan SDK'dan) kullanarak GLSL'i SPIRV'ye compile eder

REM Önce shader klasörüne geç
cd /d "%~dp0"

echo Compiling GLSL shaders to SPIRV for SDL_GPU...
echo Working directory: %CD%
echo.

REM Vertex Shader
if exist basic_3d.vert.glsl (
    glslc -fshader-stage=vertex basic_3d.vert.glsl -o basic_3d.vert.spv
    if %ERRORLEVEL% EQU 0 (
        echo [OK] basic_3d.vert.spv
    ) else (
        echo [FAIL] Vertex shader compilation failed!
    )
) else (
    echo [FAIL] basic_3d.vert.glsl not found!
)

REM Fragment Shader
if exist basic_3d.frag.glsl (
    glslc -fshader-stage=fragment basic_3d.frag.glsl -o basic_3d.frag.spv
    if %ERRORLEVEL% EQU 0 (
        echo [OK] basic_3d.frag.spv
    ) else (
        echo [FAIL] Fragment shader compilation failed!
    )
) else (
    echo [FAIL] basic_3d.frag.glsl not found!
)

echo.
echo Shader compilation complete!
echo.
echo NOTES:
echo - SPIRV shaders work on Vulkan backend
echo - glslc is part of Vulkan SDK
echo - Install Vulkan SDK from: https://vulkan.lunarg.com/
echo.
pause
