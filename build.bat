"C:\Program Files\Raspberry Pi\Pico SDK v1.5.1\cmake\bin\cmake.EXE" -DCMAKE_BUILD_TYPE:STRING=MinSizeRel -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE "-DCMAKE_C_COMPILER:FILEPATH=C:\Program Files\Raspberry Pi\Pico SDK v1.5.1\gcc-arm-none-eabi\bin\arm-none-eabi-gcc.exe" "-DCMAKE_CXX_COMPILER:FILEPATH=C:\Program Files\Raspberry Pi\Pico SDK v1.5.1\gcc-arm-none-eabi\bin\arm-none-eabi-g++.exe" --no-warn-unused-cli -S . -B ./build -G Ninja
cd build
ninja
cd ..\
