
list(APPEND EXAMPLES
    bitmap
    concurrency
    endian
    filesystem
    math
    memory
)

foreach(example IN LISTS EXAMPLES)
    add_executable(${example} ${example}.cpp)
endforeach()
