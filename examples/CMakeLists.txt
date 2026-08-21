
message("[Examples]")

if (MSVC)
    add_compile_options("$<$<CONFIG:Release>:/Ox>")
else ()
    add_compile_options("$<$<CONFIG:Release>:-O3>")
    add_compile_options(-Wall)
endif ()

link_libraries(mango::mango)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(_mango_examples_added 0)

add_subdirectory(misc)
add_subdirectory(image)

set(_mango_example_binary_folders
    "${CMAKE_CURRENT_BINARY_DIR}/misc"
    "${CMAKE_CURRENT_BINARY_DIR}/image"
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

message("    Added ${_mango_examples_added} examples.")
