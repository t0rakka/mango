
list(APPEND EXAMPLES
    vkbasic
)

foreach(example IN LISTS EXAMPLES)
    add_executable(${example} WIN32 ${example}.cpp)
    set_target_properties(${example} PROPERTIES FOLDER "examples/vulkan")
    source_group("" FILES ${example}.cpp)
endforeach()
