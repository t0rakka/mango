# Mango v3 library targets (included from root CMakeLists.txt)

set(MANGO_SOVERSION 3)

set(MANGO_CORE_SOURCES
    ${MAIN_HEADER}
    ${CORE_HEADERS}
    ${CORE_SOURCES}
    ${FILESYSTEM_HEADERS}
    ${FILESYSTEM_SOURCES}
    ${MATH_HEADERS}
    ${MATH_SOURCES}
    ${SIMD_HEADERS}
    ${SIMD_SOURCES}
    ${EXTERNAL_AES}
    ${EXTERNAL_CONCURRENT_QUEUE}
    ${EXTERNAL_UNRAR}
    ${EXTERNAL_INFLATELIB}
)

set(MANGO_IMAGE_SOURCES
    ${IMAGE_HEADERS}
    ${IMAGE_SOURCES}
    ${JPEG_HEADERS}
    ${JPEG_SOURCES}
    ${EXTERNAL_BC}
    ${EXTERNAL_GOOGLE}
    ${EXTERNAL_ZPNG}
)

if (IMAGE_FORMAT_KTX2)
    list(APPEND MANGO_IMAGE_SOURCES ${EXTERNAL_BASISU})
endif ()

if (LIBRARY_ASTC)
    list(APPEND MANGO_IMAGE_SOURCES ${EXTERNAL_ASTC})
endif ()

if (LIBRARY_LZMA)
    list(APPEND MANGO_CORE_SOURCES ${EXTERNAL_LZMA})
endif ()
if (LIBRARY_LZFSE)
    list(APPEND MANGO_CORE_SOURCES ${EXTERNAL_LZFSE})
endif ()

add_library(mango-core ${MANGO_CORE_SOURCES})
add_library(mango::core ALIAS mango-core)

add_library(mango-image ${MANGO_IMAGE_SOURCES})
add_library(mango::image ALIAS mango-image)

set(MANGO_LIBRARY_TARGETS mango-core mango-image)

function(mango_set_library_properties target)
    set_target_properties(${target} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${MANGO_SOVERSION}
    )
    if (APPLE)
        set_target_properties(${target} PROPERTIES OSX_DEPLOYMENT_TARGET "10.15")
    endif ()
endfunction()

function(mango_apply_shared_config target)
    if (BUILD_SHARED_LIBS)
        target_compile_definitions(${target} PUBLIC "MANGO_SHARED")
        target_compile_definitions(${target} PRIVATE "MANGO_API_EXPORT")
    endif ()
    if (WIN32)
        target_compile_definitions(${target} PUBLIC UNICODE)
    endif ()
    target_include_directories(${target}
        PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
        INTERFACE $<INSTALL_INTERFACE:include/>
    )
endfunction()

mango_set_library_properties(mango-core)
mango_apply_shared_config(mango-core)
target_include_directories(mango-core PRIVATE "${EXTERNAL_SOURCE_DIR}/inflatelib")

mango_set_library_properties(mango-image)
mango_apply_shared_config(mango-image)
target_link_libraries(mango-image PUBLIC mango-core)

if (CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(mango-core PUBLIC "${CMAKE_THREAD_LIBS_INIT}")
endif ()

# ------------------------------------------------------------------------------
# mango-window
# ------------------------------------------------------------------------------

if (BUILD_MANGO_WINDOW AND NOT EMSCRIPTEN)

    set(MANGO_WINDOW_SOURCES
        ${WINDOW_HEADERS}
        ${WINDOW_SOURCES}
        ${MANGO_SOURCE_DIR}/window/window_backend.cpp
        ${MANGO_SOURCE_DIR}/window/window_registry.cpp
    )

    add_library(mango-window ${MANGO_WINDOW_SOURCES})
    add_library(mango::window ALIAS mango-window)
    list(APPEND MANGO_LIBRARY_TARGETS mango-window)

    mango_set_library_properties(mango-window)
    mango_apply_shared_config(mango-window)
    target_link_libraries(mango-window PUBLIC mango-core)

    if (WIN32 OR CYGWIN)
        target_link_libraries(mango-window PUBLIC user32 gdi32 shell32 ole32)
    elseif (APPLE)
        target_link_libraries(mango-window PUBLIC "-framework Cocoa" "-framework CoreVideo" "-framework QuartzCore")
    else ()
        if (MANGO_HAS_XLIB_WINDOW)
            target_compile_definitions(mango-window PRIVATE MANGO_HAS_XLIB_WINDOW)
            target_link_libraries(mango-window PUBLIC X11 Xext Xrandr)
        endif ()
        if (MANGO_HAS_XCB_WINDOW)
            target_compile_definitions(mango-window PRIVATE MANGO_HAS_XCB_WINDOW)
            target_link_libraries(mango-window PUBLIC X11-xcb xcb xcb-sync xcb-xkb xcb-keysyms xcb-icccm xcb-randr xcb-glx xkbcommon)
        endif ()
        if (MANGO_HAS_WAYLAND_WINDOW)
            target_compile_definitions(mango-window PRIVATE MANGO_HAS_WAYLAND_WINDOW)
            target_include_directories(mango-window PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
            target_link_libraries(mango-window PUBLIC wayland-client xkbcommon)
        endif ()
    endif ()

endif ()

# ------------------------------------------------------------------------------
# mango-opengl
# ------------------------------------------------------------------------------

if (BUILD_OPENGL)

    add_library(mango-opengl ${OPENGL_HEADERS} ${OPENGL_SOURCES})
    add_library(mango::opengl ALIAS mango-opengl)
    list(APPEND MANGO_LIBRARY_TARGETS mango-opengl)

    mango_set_library_properties(mango-opengl)
    mango_apply_shared_config(mango-opengl)

    if (BUILD_MANGO_WINDOW)
        target_link_libraries(mango-opengl PUBLIC mango-window mango-image)
    else ()
        target_link_libraries(mango-opengl PUBLIC mango-core mango-image)
    endif ()

    if (APPLE)
        target_link_libraries(mango-opengl PUBLIC "-framework OpenGL")
    elseif (WIN32)
        target_link_libraries(mango-opengl PUBLIC ${OPENGL_gl_LIBRARY})
    elseif (NOT EMSCRIPTEN)
        target_link_libraries(mango-opengl PUBLIC GL)
        if (MANGO_HAS_XLIB_WINDOW OR MANGO_HAS_XCB_WINDOW)
            target_link_libraries(mango-opengl PUBLIC ${OPENGL_glx_LIBRARY})
        endif ()
        if (MANGO_HAS_WAYLAND_WINDOW)
            target_link_libraries(mango-opengl PUBLIC wayland-egl)
        endif ()
        if (ENABLE_EGL OR MANGO_HAS_WAYLAND_WINDOW)
            target_compile_definitions(mango-opengl PRIVATE MANGO_OPENGL_CONTEXT_EGL)
            target_link_libraries(mango-opengl PUBLIC EGL)
        endif ()
        if (MANGO_HAS_XLIB_WINDOW)
            target_compile_definitions(mango-opengl PRIVATE MANGO_HAS_XLIB_WINDOW)
        endif ()
        if (MANGO_HAS_XCB_WINDOW)
            target_compile_definitions(mango-opengl PRIVATE MANGO_HAS_XCB_WINDOW)
        endif ()
        if (MANGO_HAS_WAYLAND_WINDOW)
            target_compile_definitions(mango-opengl PRIVATE MANGO_HAS_WAYLAND_WINDOW)
        endif ()
    endif ()

endif ()

# ------------------------------------------------------------------------------
# mango-vulkan
# ------------------------------------------------------------------------------

if (BUILD_VULKAN)

    add_library(mango-vulkan ${VULKAN_HEADERS} ${VULKAN_SOURCES})
    add_library(mango::vulkan ALIAS mango-vulkan)
    list(APPEND MANGO_LIBRARY_TARGETS mango-vulkan)

    mango_set_library_properties(mango-vulkan)
    mango_apply_shared_config(mango-vulkan)
    target_include_directories(mango-vulkan PRIVATE
        "${EXTERNAL_SOURCE_DIR}/vma"
        "${EXTERNAL_SOURCE_DIR}/stb"
    )

    if (APPLE)
        target_sources(mango-vulkan PRIVATE
            "${MANGO_SOURCE_DIR}/vulkan/cocoa/cocoa_vulkan_window.mm"
        )
    elseif (NOT EMSCRIPTEN AND NOT WIN32)
        if (MANGO_HAS_XLIB_WINDOW)
            target_compile_definitions(mango-vulkan PRIVATE MANGO_HAS_XLIB_WINDOW)
        endif ()
        if (MANGO_HAS_XCB_WINDOW)
            target_compile_definitions(mango-vulkan PRIVATE MANGO_HAS_XCB_WINDOW)
        endif ()
        if (MANGO_HAS_WAYLAND_WINDOW)
            target_compile_definitions(mango-vulkan PRIVATE MANGO_HAS_WAYLAND_WINDOW)
        endif ()
    endif ()

    if (BUILD_MANGO_WINDOW)
        target_link_libraries(mango-vulkan PUBLIC mango-window mango-image)
    else ()
        target_link_libraries(mango-vulkan PUBLIC mango-core mango-image)
    endif ()

    find_package(glslang CONFIG REQUIRED)
    target_link_libraries(mango-vulkan PRIVATE
        glslang::glslang
        glslang::SPIRV
    )
    if (TARGET glslang::glslang-default-resource-limits)
        target_link_libraries(mango-vulkan PRIVATE glslang::glslang-default-resource-limits)
    elseif (TARGET glslang-default-resource-limits)
        target_link_libraries(mango-vulkan PRIVATE glslang-default-resource-limits)
    elseif (TARGET glslang::ResourceLimits)
        target_link_libraries(mango-vulkan PRIVATE glslang::ResourceLimits)
    endif ()

    if (UNIX AND NOT APPLE)
        target_link_options(mango-vulkan PRIVATE "LINKER:--exclude-libs,ALL")
        target_link_options(mango-vulkan PRIVATE "$<$<CONFIG:Release>:LINKER:-s>")
    endif ()

    if (WIN32)
        target_include_directories(mango-vulkan PUBLIC ${Vulkan_INCLUDE_DIRS})
        target_link_libraries(mango-vulkan PUBLIC ${Vulkan_LIBRARIES})
    elseif (APPLE)
        target_compile_definitions(mango-vulkan PRIVATE VK_USE_PLATFORM_METAL_EXT)
        target_include_directories(mango-vulkan PUBLIC ${Vulkan_INCLUDE_DIRS})
        target_link_libraries(mango-vulkan PUBLIC ${Vulkan_LIBRARIES})
    else ()
        target_link_libraries(mango-vulkan PUBLIC vulkan)
    endif ()

    if (MANGO_HAS_FREETYPE)
        target_compile_definitions(mango-vulkan PRIVATE MANGO_HAS_FREETYPE=1)
    endif ()
    if (MANGO_HAS_HARFBUZZ)
        target_compile_definitions(mango-vulkan PRIVATE MANGO_HAS_HARFBUZZ=1)
    endif ()

endif ()

# ------------------------------------------------------------------------------
# mango-import3d
# ------------------------------------------------------------------------------

if (BUILD_IMPORT3D)

    add_library(mango-import3d
        ${IMPORT3D_HEADERS}
        ${IMPORT3D_SOURCES}
        ${EXTERNAL_IMPORT3D}
    )
    add_library(mango::import3d ALIAS mango-import3d)
    list(APPEND MANGO_LIBRARY_TARGETS mango-import3d)

    mango_set_library_properties(mango-import3d)
    mango_apply_shared_config(mango-import3d)
    target_link_libraries(mango-import3d PUBLIC mango-core mango-image)
    target_include_directories(mango-import3d PRIVATE "${EXTERNAL_SOURCE_DIR}/fastgltf/include")

endif ()

# ------------------------------------------------------------------------------
# convenience aggregate
# ------------------------------------------------------------------------------

add_library(mango INTERFACE)
add_library(mango::mango ALIAS mango)
target_link_libraries(mango INTERFACE mango-core mango-image)
if (TARGET mango-window)
    target_link_libraries(mango INTERFACE mango-window)
endif ()
if (TARGET mango-opengl)
    target_link_libraries(mango INTERFACE mango-opengl)
endif ()
if (TARGET mango-vulkan)
    target_link_libraries(mango INTERFACE mango-vulkan)
endif ()
if (TARGET mango-import3d)
    target_link_libraries(mango INTERFACE mango-import3d)
endif ()

if (UNIX AND NOT APPLE)
    target_link_options(mango-core PRIVATE "LINKER:--exclude-libs,ALL")
    target_link_options(mango-core PRIVATE "$<$<CONFIG:Release>:LINKER:-s>")
    target_link_options(mango-image PRIVATE "LINKER:--exclude-libs,ALL")
    target_link_options(mango-image PRIVATE "$<$<CONFIG:Release>:LINKER:-s>")
endif ()

if (APPLE)
    target_compile_options(mango-core PUBLIC "-mmacosx-version-min=10.15")
    target_compile_options(mango-image PUBLIC "-mmacosx-version-min=10.15")
endif ()
