
message("[Tools]")

if (MSVC)
    add_compile_options("$<$<CONFIG:Release>:/Ox>")
else ()
    add_compile_options("$<$<CONFIG:Release>:-O3>")
    add_compile_options(-Wall)
endif ()

link_libraries(mango::mango)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(MANGO_TOOLS
    hcompress
    hdecompress
    image_encoder
    image_inspect
)

foreach(tool IN LISTS MANGO_TOOLS)
    add_executable(${tool} ${tool}.cpp)
    set_target_properties(${tool} PROPERTIES FOLDER "tools")
    source_group("" FILES ${tool}.cpp)
endforeach()

list(LENGTH MANGO_TOOLS _mango_tools_count)
message("    Added ${_mango_tools_count} tools.")

if (NOT TOOL_BINARY_FOLDERS STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    set(TOOL_BINARY_FOLDERS "${CMAKE_CURRENT_BINARY_DIR}"
        CACHE STRING "Binary directories for tools" FORCE
    )
endif ()
