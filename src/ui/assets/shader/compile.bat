@echo off
chcp 65001 >nul
REM 编译 HLSL 着色器到 SPIR-V 和 DXIL
REM 需要安装 DirectXShaderCompiler (dxc)

echo Compiling shaders...

REM ==================== Vulkan (SPIR-V) ====================
echo.
echo [1/4] Compiling vertex shader for Vulkan (SPIR-V)...
dxc -spirv -T vs_6_0 -E main_vs "-fspv-target-env=vulkan1.3" -Fo vert.spv vert.hlsl
if %errorlevel% neq 0 (
    echo Failed to compile vertex shader for Vulkan
    pause
    exit /b 1
)

echo [2/4] Compiling fragment shader for Vulkan (SPIR-V)...
dxc -spirv -T ps_6_0 -E main_ps "-fspv-target-env=vulkan1.3" -Fo frag.spv frag.hlsl
if %errorlevel% neq 0 (
    echo Failed to compile fragment shader for Vulkan
    pause
    exit /b 1
)

REM ==================== DirectX 12 (DXIL) ====================
echo.
echo [3/4] Compiling vertex shader for DX12 (DXIL)...
dxc -T vs_6_6 -E main_vs -Fo vert.dxil vert.hlsl
if %errorlevel% neq 0 (
    echo Failed to compile vertex shader for DX12
    pause
    exit /b 1
)

echo [4/4] Compiling fragment shader for DX12 (DXIL)...
dxc -T ps_6_6 -E main_ps -Fo frag.dxil frag.hlsl
if %errorlevel% neq 0 (
    echo Failed to compile fragment shader for DX12
    pause
    exit /b 1
)

echo.
echo ========================================
echo All shaders compiled successfully!
echo   - Vulkan: vert.spv, frag.spv
echo   - DX12:   vert.dxil, frag.dxil
echo ========================================
pause
