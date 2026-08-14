# ISA / SIMD compile configuration (included from root CMakeLists.txt)

function (mango_isa_compute_targets out_var)
    set(_targets mango-core mango-image)
    if (TARGET mango-import3d)
        list(APPEND _targets mango-import3d)
    endif ()
    set(${out_var} ${_targets} PARENT_SCOPE)
endfunction()

function (mango_apply_isa_to_target target)
    if (EMSCRIPTEN)
        target_compile_options(${target} PUBLIC "-msimd128")
        return ()
    endif ()

    if (MSVC)
        target_compile_options(${target} PUBLIC $<$<CONFIG:Release>:/Ox>)
        target_compile_options(${target} PUBLIC "/utf-8")
        target_compile_options(${target} PUBLIC "/MP")

        if (X86 OR X86_64)
            # Optional post-SSE2 ISA extensions (F16C, POPCNT, ...) require an SSE2+
            # SIMD tier to be enabled. Without that baseline, vendored astcenc can end
            # up in a scalar path while __F16C__ is still defined and the build breaks.
            # On x86-32, ENABLE_SSE2/ENABLE_SSE4 also selects /arch:SSE2 (SSE2 was
            # common late in the 32-bit era; x86-64 always has it in hardware).
            set(_mango_x86_simd_baseline FALSE)
            if (ENABLE_SSE2 OR ENABLE_SSE4 OR ENABLE_AVX OR ENABLE_AVX2 OR ENABLE_AVX512)
                set(_mango_x86_simd_baseline TRUE)
            endif ()

            if (_mango_x86_simd_baseline)
                if (ENABLE_PCLMUL)
                    target_compile_definitions(${target} PUBLIC "__PCLMUL__")
                endif ()

                if (ENABLE_POPCNT)
                    target_compile_definitions(${target} PUBLIC "__POPCNT__")
                endif ()

                if (ENABLE_AES)
                    target_compile_definitions(${target} PUBLIC "__AES__")
                endif ()

                if (ENABLE_F16C)
                    target_compile_definitions(${target} PUBLIC "__F16C__")
                endif ()

                if (ENABLE_LZCNT)
                    target_compile_definitions(${target} PUBLIC "__LZCNT__")
                endif ()

                if (ENABLE_BMI)
                    target_compile_definitions(${target} PUBLIC "__BMI__")
                endif ()

                if (ENABLE_BMI2)
                    target_compile_definitions(${target} PUBLIC "__BMI2__")
                endif ()

                if (ENABLE_FMA)
                    target_compile_definitions(${target} PUBLIC "__FMA__")
                endif ()

                if (ENABLE_SHA)
                    target_compile_definitions(${target} PUBLIC "__SHA__")
                endif ()
            endif ()

            # MSVC does not predefine __SSE2__ / __SSE4_2__ / __AVX2__ the way GCC and
            # Clang do via -msse* / -mavx*. Vendored astcenc (and similar third-party
            # code that never includes mango/configure.hpp) selects its SIMD backend
            # from these macros in astcenc_mathlib.h. Without them, astcenc falls
            # back to the scalar path while optional ISA macros (__F16C__, etc.) may
            # still be set, which breaks the build.
            #
            # mango/configure.hpp uses #ifndef guards for the same names, so defining
            # them here does not conflict with mango headers.
            if (ENABLE_SSE2 OR ENABLE_SSE4 OR ENABLE_AVX OR ENABLE_AVX2 OR ENABLE_AVX512)
                target_compile_definitions(${target} PUBLIC "__SSE2__")
            endif ()
            if (ENABLE_SSE4 OR ENABLE_AVX OR ENABLE_AVX2 OR ENABLE_AVX512)
                target_compile_definitions(${target} PUBLIC "__SSE4_1__" "__SSE4_2__")
            endif ()
            if (ENABLE_AVX OR ENABLE_AVX2 OR ENABLE_AVX512)
                target_compile_definitions(${target} PUBLIC "__AVX__")
            endif ()
            if (ENABLE_AVX2 OR ENABLE_AVX512)
                target_compile_definitions(${target} PUBLIC "__AVX2__")
            endif ()

            # enable only one (the most recent) SIMD extension
            if (ENABLE_AVX512)
                target_compile_options(${target} PUBLIC "/arch:AVX512")
                target_compile_definitions(${target} PUBLIC "__AVX512F__" "__AVX512DQ__")
            elseif (ENABLE_AVX2)
                target_compile_options(${target} PUBLIC "/arch:AVX2")
            elseif (ENABLE_AVX)
                target_compile_options(${target} PUBLIC "/arch:AVX")
            elseif (ENABLE_SSE2 OR ENABLE_SSE4)
                # MSVC does not have SSE4 option; at least enable SSE2
                if (NOT X86_64)
                    # MSVC enables SSE2 by default on X86_64
                    target_compile_options(${target} PUBLIC "/arch:SSE2")
                endif ()
            endif ()
        endif ()

        return ()
    endif ()

    target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:-O3>)

    if (CYGWIN)
        target_compile_options(${target} PUBLIC "-municode")
    endif ()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(${target} PUBLIC "-ftree-vectorize")
    endif ()

    if (X86 OR X86_64)
        set(_mango_x86_simd_baseline FALSE)
        if (ENABLE_SSE2 OR ENABLE_SSE4 OR ENABLE_AVX OR ENABLE_AVX2 OR ENABLE_AVX512)
            set(_mango_x86_simd_baseline TRUE)
        endif ()

        if (_mango_x86_simd_baseline)
            if (ENABLE_PCLMUL)
                target_compile_options(${target} PUBLIC "-mpclmul")
            endif ()

            if (ENABLE_POPCNT)
                target_compile_options(${target} PUBLIC "-mpopcnt")
            endif ()

            if (ENABLE_AES)
                target_compile_options(${target} PUBLIC "-maes")
            endif ()

            if (ENABLE_F16C)
                target_compile_options(${target} PUBLIC "-mf16c")
            endif ()

            if (ENABLE_LZCNT)
                target_compile_options(${target} PUBLIC "-mlzcnt")
            endif ()

            if (ENABLE_BMI)
                target_compile_options(${target} PUBLIC "-mbmi")
            endif ()

            if (ENABLE_BMI2)
                target_compile_options(${target} PUBLIC "-mbmi2")
            endif ()

            if (ENABLE_FMA)
                target_compile_options(${target} PUBLIC "-mfma")
            endif ()

            if (ENABLE_SHA)
                target_compile_options(${target} PUBLIC "-msha")
            endif ()
        endif ()

        # enable only one (the most recent) SIMD extension
        if (ENABLE_AVX512)
            target_compile_options(${target} PUBLIC "-mavx512dq")
            target_compile_options(${target} PUBLIC "-mavx512vl")
            target_compile_options(${target} PUBLIC "-mavx512bw")
        elseif (ENABLE_AVX2)
            target_compile_options(${target} PUBLIC "-mavx2")
        elseif (ENABLE_AVX)
            target_compile_options(${target} PUBLIC "-mavx")
        elseif (ENABLE_SSE4)
            target_compile_options(${target} PUBLIC "-msse4")
        elseif (ENABLE_SSE2)
            target_compile_options(${target} PUBLIC "-msse2")
        endif ()
    endif ()
endfunction()

function (mango_resolve_simd_level out_var)
    set(_simd "NONE")

    if ((ARM AND ENABLE_NEON) OR ARM64)
        set(_simd "NEON")
    endif ()

    if (EMSCRIPTEN)
        set(_simd "WASM")
    elseif (MSVC AND (X86 OR X86_64))
        if (ENABLE_AVX512)
            set(_simd "AVX-512 (2015)")
        elseif (ENABLE_AVX2)
            set(_simd "AVX2 (2013)")
        elseif (ENABLE_AVX)
            set(_simd "AVX (2008)")
        elseif (ENABLE_SSE2 OR ENABLE_SSE4)
            set(_simd "SSE2 (2001)")
        endif ()
    elseif (NOT MSVC AND (X86 OR X86_64))
        if (ENABLE_AVX512)
            set(_simd "AVX-512 (2015)")
        elseif (ENABLE_AVX2)
            set(_simd "AVX2 (2013)")
        elseif (ENABLE_AVX)
            set(_simd "AVX (2008)")
        elseif (ENABLE_SSE4)
            set(_simd "SSE4.2 (2006)")
        elseif (ENABLE_SSE2)
            set(_simd "SSE2 (2001)")
        endif ()
    endif ()

    set(${out_var} ${_simd} PARENT_SCOPE)
endfunction()

function (mango_report_isa_extensions simd_level)
    message("[ISA Extensions]")

    if (X86 OR X86_64)
        set(_mango_x86_simd_baseline FALSE)
        if (ENABLE_SSE2 OR ENABLE_SSE4 OR ENABLE_AVX OR ENABLE_AVX2 OR ENABLE_AVX512)
            set(_mango_x86_simd_baseline TRUE)
        endif ()

        if (_mango_x86_simd_baseline)
            if (ENABLE_PCLMUL)
                message("    PCLMUL (2008)")
            endif ()

            if (ENABLE_POPCNT)
                message("    POPCNT (2008)")
            endif ()

            if (ENABLE_AES)
                message("    AES (2010)")
            endif ()

            if (ENABLE_F16C)
                message("    F16C (2012)")
            endif ()

            if (ENABLE_LZCNT)
                message("    LZCNT (2013)")
            endif ()

            if (ENABLE_BMI)
                message("    BMI (2013)")
            endif ()

            if (ENABLE_BMI2)
                message("    BMI2 (2013)")
            endif ()

            if (ENABLE_FMA)
                message("    FMA (2013)")
            endif ()

            if (ENABLE_SHA)
                message("    SHA (2013)")
            endif ()
        elseif (ENABLE_PCLMUL OR ENABLE_POPCNT OR ENABLE_AES OR ENABLE_F16C OR ENABLE_LZCNT
                OR ENABLE_BMI OR ENABLE_BMI2 OR ENABLE_FMA OR ENABLE_SHA)
            message("    (optional ISA extensions ignored: enable SSE2 or higher)")
        endif ()

        if (ENABLE_SSE2)
            message("    SSE2 (2001)")
        endif ()

        if (ENABLE_SSE4)
            message("    SSE4 (2006)")
        endif ()

        if (ENABLE_AVX)
            message("    AVX (2008)")
        endif ()

        if (ENABLE_AVX2)
            message("    AVX2 (2013)")
        endif ()

        if (ENABLE_AVX512)
            message("    AVX-512 (2015)")
        endif ()
    endif ()

    message("[SIMD]")
    message("    " ${simd_level})
endfunction()

function (mango_configure_isa)
    mango_isa_compute_targets(_isa_targets)

    if (NOT EMSCRIPTEN AND NOT MSVC)
        add_compile_options(-Wall)

        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            set(CMAKE_CXX_FLAGS "-Wno-psabi" PARENT_SCOPE)
        endif ()

        if (ARM AND (NOT APPLE))
            # The compiler for Apple Silicon does not recognize these
            add_definitions(-mfpu=neon -mfloat-abi=hard)
        endif ()
    endif ()

    foreach (_target IN LISTS _isa_targets)
        mango_apply_isa_to_target(${_target})
    endforeach ()

    mango_resolve_simd_level(_simd)
    mango_report_isa_extensions(${_simd})
    set(SIMD ${_simd} PARENT_SCOPE)
endfunction()
