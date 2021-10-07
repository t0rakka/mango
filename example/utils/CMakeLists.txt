
list(APPEND binaries
    snitch
    unsnitch
)

foreach(x IN LISTS binaries)
    add_executable(${x} ${x}.cpp)
endforeach()
