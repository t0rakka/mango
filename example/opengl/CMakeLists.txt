
list(APPEND EXAMPLES
    gltest
    glfb
)

foreach(example IN LISTS EXAMPLES)
    add_executable(${example} ${example}.cpp)
    if (APPLE)
        target_link_libraries(${example} mango-opengl "-framework OpenGL -framework Cocoa")
    elseif (UNIX)
        target_link_libraries(${example} mango-opengl X11 GL)
    endif ()
endforeach()
