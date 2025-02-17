
list(APPEND EXAMPLES
    basic
    imageview
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
    add_executable(${example} ${example}.cpp)
    set_target_properties(${example} PROPERTIES FOLDER "examples/opengl")
    source_group("" FILES ${example}.cpp)

    if (APPLE)
        target_link_libraries(${example} PRIVATE "-framework OpenGL -framework Cocoa")
    elseif (UNIX)
        target_link_libraries(${example} PRIVATE X11 GL)
    elseif (WIN32)
        target_link_libraries(${example} PRIVATE opengl32)
    endif ()

endforeach()

file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
