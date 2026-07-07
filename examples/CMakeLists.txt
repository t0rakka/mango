
message("[Examples]")

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

set(_mango_example_binary_folders
    "${CMAKE_CURRENT_BINARY_DIR}/test"
    "${CMAKE_CURRENT_BINARY_DIR}/misc"
    "${CMAKE_CURRENT_BINARY_DIR}/image"
    "${CMAKE_CURRENT_BINARY_DIR}/utils"
)

if (BUILD_OPENGL)
    add_subdirectory(opengl)
    list(APPEND _mango_example_binary_folders
        "${CMAKE_CURRENT_BINARY_DIR}/opengl"
    )
endif ()

if (BUILD_VULKAN)
    add_subdirectory(vulkan)
    list(APPEND _mango_example_binary_folders
        "${CMAKE_CURRENT_BINARY_DIR}/vulkan"
    )
endif ()

if (NOT EXAMPLE_BINARY_FOLDERS STREQUAL "${_mango_example_binary_folders}")
    set(EXAMPLE_BINARY_FOLDERS "${_mango_example_binary_folders}"
        CACHE STRING "Binary Directories for examples" FORCE
    )
endif ()
