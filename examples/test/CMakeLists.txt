
list(APPEND EXAMPLES
    aes
    checksum
    hash
    compress
    threads
    pathtest
    particle
    mathtest
    container
)

foreach(example IN LISTS EXAMPLES)
    add_executable(${example} ${example}.cpp)
    set_target_properties(${example} PROPERTIES FOLDER "examples/test")
    source_group("" FILES ${example}.cpp)
endforeach()

file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
