
list(APPEND EXAMPLES
    basic
    imageview
    animation
    mandelbrot
    srgb
    cubemap
)

if (NOT APPLE)
    list(APPEND EXAMPLES
        compute
    )
endif ()

foreach(example IN LISTS EXAMPLES)
    add_executable(${example} ${example}.cpp)
    if (APPLE)
        target_link_libraries(${example} mango-opengl mango "-framework OpenGL -framework Cocoa")
    elseif (UNIX)
        target_link_libraries(${example} mango-opengl mango X11 GL)
    endif ()
endforeach()

file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
