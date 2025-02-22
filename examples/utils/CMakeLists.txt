
list(APPEND examples
    snitch
    unsnitch
    image_encoder
)

foreach(example IN LISTS examples)
    add_executable(${example} ${example}.cpp)
    set_target_properties(${example} PROPERTIES FOLDER "examples/utils")
    source_group("" FILES ${example}.cpp)
endforeach()
