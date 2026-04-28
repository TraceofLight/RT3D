@echo off
REM clean-cache.cmd
REM Remove build outputs, IDE/tool caches, and runtime-baked binary caches that
REM are gitignored but persist across branch switches and may confuse the IDE
REM or msbuild. Safe to run anytime; tracked files are never touched.

setlocal
pushd "%~dp0"

echo [clean-cache] Working in: %CD%
echo.

REM === Top-level build output directories ===
for %%D in (Binaries Intermediate Output x64 Debug Release) do (
    if exist "%%~D\" (
        echo  - removing %%~D\
        rmdir /s /q "%%~D"
    )
)

REM === Visual Studio sidecar ===
if exist ".vs\" (
    echo  - removing .vs\
    rmdir /s /q ".vs"
)

REM === Engine runtime cache directories (any wXX layout) ===
for %%D in (
    "DerivedDataCache"
    "CrashDumps"
    "Engine\DerivedDataCache"
    "Engine\CrashDumps"
    "Engine\Data\Cooked"
    "Engine\Data\TextureCache"
    "Mundi\DerivedDataCache"
    "Mundi\CrashDumps"
    "Mundi\Data\Cooked"
    "Mundi\Data\TextureCache"
) do (
    if exist "%%~D\" (
        echo  - removing %%~D\
        rmdir /s /q "%%~D"
    )
)

REM === Python __pycache__ (recursive) ===
for /d /r %%D in (__pycache__) do (
    if exist "%%~D" (
        echo  - removing %%~D
        rmdir /s /q "%%~D"
    )
)

REM === MSBuild log databases (.tlog directories, recursive) ===
for /d /r %%D in (*.tlog) do (
    if exist "%%~D" (
        echo  - removing %%~D
        rmdir /s /q "%%~D"
    )
)

REM === User-specific vcxproj overrides (recursive) ===
for /r %%F in (*.vcxproj.user) do (
    if exist "%%~F" (
        echo  - removing %%~F
        del /q "%%~F"
    )
)

REM === Runtime-baked binary caches (.objbin/.fbxbin/.X.bin) ===
REM Only untracked files are removed; tracked ones are preserved by git clean.
REM Using -fx with pathspec catches orphans regardless of branch gitignore state.
where git >nul 2>&1
if not errorlevel 1 (
    echo  - cleaning untracked baked binaries ^(objbin/fbxbin/mat.bin/convex.bin/trimesh.bin/anim.bin^)
    git clean -fx -- "*.objbin" "*.fbxbin" "*.mat.bin" "*.convex.bin" "*.trimesh.bin" "*.anim.bin" 1>nul 2>&1
) else (
    echo  - skipping baked-binary clean ^(git not on PATH^)
)

echo.
echo [clean-cache] Done.

popd
endlocal
