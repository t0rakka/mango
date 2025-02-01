
list(APPEND binaries
    snitch
    unsnitch
    image_encoder
)

foreach(x IN LISTS binaries)
    add_executable(${x} ${x}.cpp)
endforeach()
