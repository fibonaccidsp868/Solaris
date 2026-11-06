@echo off
REM Windows: run from an ADMINISTRATOR "x64 Native Tools / Developer" prompt (needs CMake + Visual Studio 2022 with C++)
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
echo Done. Installed to C:\Program Files\Common Files\VST3 - rescan plugins in your DAW.
pause
