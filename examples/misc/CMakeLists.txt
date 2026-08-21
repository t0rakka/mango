
list(APPEND EXAMPLES
    bitmap
    concurrency
    compress
    container
    endian
    filesystem
    memory
    particle
)

foreach(example IN LISTS EXAMPLES)
    add_executable(${example} ${example}.cpp)
    set_target_properties(${example} PROPERTIES FOLDER "examples/misc")
    source_group("" FILES ${example}.cpp)
endforeach()

list(LENGTH EXAMPLES _n)
math(EXPR _mango_examples_added "${_mango_examples_added} + ${_n}")
set(_mango_examples_added ${_mango_examples_added} PARENT_SCOPE)
