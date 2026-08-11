
message("[Tests]")

if (MSVC)
    add_compile_options("$<$<CONFIG:Release>:/Ox>")
else ()
    add_compile_options("$<$<CONFIG:Release>:-O3>")
    add_compile_options(-Wall)
endif ()

set(MANGO_TESTS
    cache
    threads
    pathtest
)

foreach(test IN LISTS MANGO_TESTS)
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} PRIVATE mango::mango)
    set_target_properties(${test} PROPERTIES FOLDER "tests")
    source_group("" FILES ${test}.cpp)
endforeach()

file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
