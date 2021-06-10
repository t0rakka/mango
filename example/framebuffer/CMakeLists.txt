
list(APPEND EXAMPLES
    framebuffer
    mandelbrot
    animation
)

foreach(example IN LISTS EXAMPLES)
    add_executable(${example} ${example}.cpp)
    if (APPLE)
        target_link_libraries(${example} mango-opengl)
    elseif (UNIX)
        target_link_libraries(${example} mango-opengl X11)
    endif ()
endforeach()

file(COPY images DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
