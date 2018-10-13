------------------------------------------------------------------------------------------------

MANGO! Compiling instructions.
The library does come with many ways to compile itself; hopefully one of them works for you.

------------------------------------------------------------------------------------------------

* CMAKE!

This is probably the most generic build script we have; it has been tested on Linux, Windows and macOS.

0. You should be in the build/ folder

1. mkdir cmake
2. cd cmake
3. cmake ..
4. make -j20
5. sudo make install

The library will be installed typically in /usr/local/include and /usr/local/lib 
The cmake build script will compile everything into one (.a) library

Pro tip! "cmake -DENABLE_AVX512=ON .." to enable Intel AVX-512 SIMD instructions.
         "cmake -DBUILD_SHARED_LIBS=ON .." to compile .so/.dll/.dylib instead of .a/.lib

------------------------------------------------------------------------------------------------

* MAKE!

This is a hand-written, custom makefile which needs some manual tinkering on different platforms 
for example when building with cross-compilation toolchains. Should work out of the box on Linux and macOS.

1. cd build/unix
2. make -j20
3. make install

The library will be compiled as shared object (.so) files: 
  - libmango.so
  - libmango-opengl.so
  - libmango-vulkan.so

The separation is done so that when not using OpenGL or Vulkan don't have to pull in X11 libraries.

Pro tip! "make simd=avx2" to enable Intel AVX2 SIMD instructions.

------------------------------------------------------------------------------------------------

* XCODE!!!

Load the xcode project and build for release.
Drag and drop or link with the mango.framework to get access to the headers and binaries.

------------------------------------------------------------------------------------------------

* VISUAL STUDIO!!!

Load the solution file. Select configuration (the Release / x64 is very nice one). Build!

When using the library; add the include/ into your projects INCLUDE path and
link with the appropriate library. Each configuration generates own static .lib file to it's own folder.

We only support the latest Visual Studio (2017 at the time of writing this); apologies for the inconvenience.

------------------------------------------------------------------------------------------------

* ANDROID!!?

1. cd build/android/
2. ndk-build -j20

Then use the resulting library files in your cool/awesome Android projects.

------------------------------------------------------------------------------------------------

Apologies for poor quality build scripts and instructions. Typically you configure the library to be part of your build system
just once and there are so many ways organizations and individuals do these things it's super duper hard to please everyone so
our main focus was pleasing ourselves foremost and fix problems as they are reported. Have a nice day! :)

