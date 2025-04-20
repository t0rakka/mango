
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

set(EXAMPLE_BINARY_FOLDERS
    "${CMAKE_CURRENT_BINARY_DIR}/test"
    "${CMAKE_CURRENT_BINARY_DIR}/misc"
    "${CMAKE_CURRENT_BINARY_DIR}/image"
    "${CMAKE_CURRENT_BINARY_DIR}/utils"
)

if (BUILD_OPENGL)
    add_subdirectory(opengl)
    list(APPEND EXAMPLE_BINARY_FOLDERS
        "${CMAKE_CURRENT_BINARY_DIR}/opengl"
    )
endif ()

set(EXAMPLE_BINARY_FOLDERS ${EXAMPLE_BINARY_FOLDERS}
    CACHE STRING "Binary Directories for examples" FORCE
)
