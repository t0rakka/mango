
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("[ examples ]")

if (MSVC)
    add_compile_options("$<$<CONFIG:Release>:/Ox>")
else ()
    add_compile_options("$<$<CONFIG:Release>:-O3>")
    add_compile_options(-Wall)
endif ()

if (APPLE)
    # fix wrapped compiler dropping /usr/local
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/local/include")
    #link_directories("/usr/local/lib")
endif ()

link_libraries(mango)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(test)
add_subdirectory(misc)
add_subdirectory(image)
add_subdirectory(utils)

if (BUILD_OPENGL)
    add_subdirectory(opengl)
endif ()
