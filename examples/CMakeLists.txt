
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("[ examples ]")

if (MSVC)
    add_compile_options("$<$<CONFIG:Release>:/Ox>")
else ()
    add_compile_options("$<$<CONFIG:Release>:-O3>")
    add_compile_options(-Wall)
endif ()

link_libraries(mango::mango)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(test)
add_subdirectory(misc)
add_subdirectory(image)
add_subdirectory(utils)

if (BUILD_OPENGL)
    add_subdirectory(opengl)
endif ()
