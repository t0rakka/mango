
list(APPEND EXAMPLES
    threads
    pathtest
    particle
    mathtest
)

foreach(example IN LISTS EXAMPLES)
    add_executable(${example} ${example}.cpp)
endforeach()

file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY pathtest.cpp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
