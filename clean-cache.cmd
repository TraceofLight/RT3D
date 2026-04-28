@echo off
setlocal
pushd "%~dp0"

echo [clean-cache] Working in: %CD%
echo.

for %%D in (Binaries Intermediate Output Build x64 Debug Release) do (
    if exist "%%~D\" (
        echo  - removing %%~D\
        rmdir /s /q "%%~D"
    )
)

if exist ".vs\" (
    echo  - removing .vs\
    rmdir /s /q ".vs"
)

for %%D in (
    "DerivedDataCache"
    "CrashDumps"
    "Engine\DerivedDataCache"
    "Engine\CrashDumps"
    "Engine\Data\Cooked"
    "Engine\Data\TextureCache"
    "Engine\Tools\Python"
    "Engine\ThirdParty\Python"
    "Mundi\DerivedDataCache"
    "Mundi\CrashDumps"
    "Mundi\Data\Cooked"
    "Mundi\Data\TextureCache"
    "Mundi\Tools\Python"
    "Mundi\ThirdParty\Python"
) do (
    if exist "%%~D\" (
        echo  - removing %%~D\
        rmdir /s /q "%%~D"
    )
)

for /d /r %%D in (__pycache__) do (
    if exist "%%~D" (
        echo  - removing %%~D
        rmdir /s /q "%%~D"
    )
)

for /d /r %%D in (*.tlog) do (
    if exist "%%~D" (
        echo  - removing %%~D
        rmdir /s /q "%%~D"
    )
)

for /r %%F in (*.vcxproj.user) do (
    if exist "%%~F" (
        echo  - removing %%~F
        del /q "%%~F"
    )
)

where git >nul 2>&1
if not errorlevel 1 (
    echo  - cleaning untracked files
    git clean -fd 1>nul 2>&1
    echo  - cleaning untracked baked binaries
    git clean -fx -- "*.objbin" "*.fbxbin" "*.mat.bin" "*.convex.bin" "*.trimesh.bin" "*.anim.bin" 1>nul 2>&1
)

echo.
echo [clean-cache] Done.

popd
endlocal
