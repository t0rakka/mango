
list(APPEND binaries
    snitch
    unsnitch
    image_encoder
)

foreach(binary IN LISTS binaries)
    add_executable(${binary} ${binary}.cpp)
    set_target_properties(${binary} PROPERTIES FOLDER "examples/utils")
    source_group("" FILES ${binary}.cpp)
endforeach()
