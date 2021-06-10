
list(APPEND EXAMPLES
    basic
    imageview
    animation
    mandelbrot
)

foreach(example IN LISTS EXAMPLES)
    add_executable(${example} ${example}.cpp)
    if (APPLE)
        target_link_libraries(${example} mango-opengl "-framework OpenGL -framework Cocoa")
    elseif (UNIX)
        target_link_libraries(${example} mango-opengl X11 GL)
    endif ()
endforeach()

file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
