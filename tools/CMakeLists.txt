
message("[Tools]")

if (MSVC)
    add_compile_options("$<$<CONFIG:Release>:/Ox>")
else ()
    add_compile_options("$<$<CONFIG:Release>:-O3>")
    add_compile_options(-Wall)
endif ()

link_libraries(mango::mango)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_executable(hcompress hcompress.cpp)
add_executable(hdecompress hdecompress.cpp)
add_executable(image_encoder image_encoder.cpp)
add_executable(image_inspect image_inspect.cpp)

foreach(tool IN ITEMS hcompress hdecompress image_encoder image_inspect)
    set_target_properties(${tool} PROPERTIES FOLDER "tools")
endforeach()

source_group("" FILES
    hcompress.cpp
    hdecompress.cpp
    image_encoder.cpp
    image_inspect.cpp
)

if (NOT TOOL_BINARY_FOLDERS STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    set(TOOL_BINARY_FOLDERS "${CMAKE_CURRENT_BINARY_DIR}"
        CACHE STRING "Binary directories for tools" FORCE
    )
endif ()
