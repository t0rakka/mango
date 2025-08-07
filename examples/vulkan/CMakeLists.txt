
list(APPEND EXAMPLES
    basic
)

foreach(example IN LISTS EXAMPLES)
    add_executable(${example} WIN32 ${example}.cpp)
    set_target_properties(${example} PROPERTIES FOLDER "examples/vulkan")
    source_group("" FILES ${example}.cpp)
endforeach()

#file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
