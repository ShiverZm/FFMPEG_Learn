@echo off

set PATH=%PATH%;../build/cmake_3.20/windows/bin;
cd ..
mkdir build_2019
cd build_2019
cmake .. -G"Visual Studio 16 2019" -A Win32 -DCMAKE_GENERATOR_TOOLSET=v142 -DCMAKE_CONFIGURATION_TYPES=Debug;Release
pause
@echo on
