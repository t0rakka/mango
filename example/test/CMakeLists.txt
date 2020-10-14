
list(APPEND EXAMPLES
    threads
    pathtest
    particle
)

foreach(example IN LISTS EXAMPLES)
    add_executable(${example} ${example}.cpp)
endforeach()
