
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
    set_target_properties(${example} PROPERTIES FOLDER "examples/misc")
    source_group("" FILES ${example}.cpp)
endforeach()
