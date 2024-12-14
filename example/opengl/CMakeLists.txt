
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

find_package(mango-opengl REQUIRED)
link_libraries(mango-opengl)

foreach(example IN LISTS EXAMPLES)
    add_executable(${example} ${example}.cpp)
endforeach()

file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
