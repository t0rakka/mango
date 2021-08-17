
list(APPEND EXAMPLES
    bitmap
    checksum
    compress
    concurrency
    endian
    filesystem
    math
    memory
    aes
    hash
)

foreach(example IN LISTS EXAMPLES)
    add_executable(${example} ${example}.cpp)
endforeach()
