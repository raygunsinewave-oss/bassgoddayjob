@echo off
rem One-command build for Windows x64: VST3 and Standalone, plus tests. Needs Visual Studio 2022 and CMake 3.22+.
cd /d "%~dp0\.."
cmake -B build -G "Visual Studio 17 2022" -A x64 %* || exit /b 1
cmake --build build --config Release --parallel || exit /b 1
ctest --test-dir build -C Release --output-on-failure || exit /b 1
echo Artefacts: build\BASSGODSDAYJOB_artefacts\Release\VST3 and Standalone
