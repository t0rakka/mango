
list(APPEND EXAMPLES
    basic
    imageview
    bicubic
    animation
    mandelbrot
    srgb
    cubemap
)

if (NOT APPLE)
    # OpenGL on macOS is 4.1 and does not support compute shaders
    list(APPEND EXAMPLES compute)
endif ()

foreach(example IN LISTS EXAMPLES)
    add_executable(${example} WIN32 ${example}.cpp)
    set_target_properties(${example} PROPERTIES FOLDER "examples/opengl")
    source_group("" FILES ${example}.cpp)
endforeach()

file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
