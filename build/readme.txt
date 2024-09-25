------------------------------------------------------------------------------------------------

MANGO! Compiling instructions.
The library does come with many ways to compile itself; hopefully one of them works for you.

------------------------------------------------------------------------------------------------
* MESON!
------------------------------------------------------------------------------------------------

0. You should be in the build/ folder

1. meson setup temp
2. cd temp
3. ninja
4. sudo ninja install

------------------------------------------------------------------------------------------------
* CMAKE!
------------------------------------------------------------------------------------------------

0. You should be in the build/ folder

1. mkdir temp
2. cd temp
3. cmake ..
4. make -j20
5. sudo make install

The library will be installed typically in /usr/local/include and /usr/local/lib 
The cmake build script will compile everything into one (.so) library

Pro tip! "cmake -DENABLE_AVX512=ON .." to enable Intel AVX-512 SIMD instructions.
         "cmake -DBUILD_SHARED_LIBS=OFF .." to compile .a/.lib instead of .so/.dll/.dylib

Select compiler:
    "cmake .. -DCMAKE_C_COMPILER=gcc-mp-7 -DCMAKE_CXX_COMPILER=g++-mp-7" (example for gcc-7 mp)    

------------------------------------------------------------------------------------------------
* CMAKE + Emscripten
------------------------------------------------------------------------------------------------

0. You should be in the build/ folder

1. mkdir temp
2. cd temp
3. emcmake cmake ..
4. make -j20
5. sudo make install

------------------------------------------------------------------------------------------------
* XCODE!!!
------------------------------------------------------------------------------------------------

Load the xcode project and build for release; mango.framework will be generated.

------------------------------------------------------------------------------------------------
* VISUAL STUDIO!!!
------------------------------------------------------------------------------------------------

1. Load the solution file.
2. Select configuration (the Release / x64 is a very nice one).
3. Build!

------------------------------------------------------------------------------------------------
* ANDROID!!?
------------------------------------------------------------------------------------------------

1. cd build/android/
2. ndk-build -j20

Then use the resulting library files in your cool/awesome Android projects.

------------------------------------------------------------------------------------------------
How To Configure A Fresh Linux (Ubuntu/Mint) For Development With MANGO
------------------------------------------------------------------------------------------------

Install the required tools and libraries:
  sudo apt install g++ git cmake libpng-dev libx11-dev libgl-dev libjpeg-turbo8-dev

------------------------------------------------------------------------------------------------
Troubleshooting!
------------------------------------------------------------------------------------------------

Error:
  "In file included from /root/temp/mango/source/mango/image/image_ktx.cpp:9:0:
  /root/temp/mango/build/../include/mango/opengl/opengl.hpp:80:23: fatal error: GL/gl.h: No such file or directory
     #include <GL/gl.h>"

Solution:
  Install OpenGL headers. On Linux distributions you get these with "mesa":
  apt-get install mesa-common-dev

-------------

Error:
  /usr/bin/ld: cannot find -lGL

Solution:
  apt install libgl1-mesa-dev

------------------------------------------------------------------------------------------------
