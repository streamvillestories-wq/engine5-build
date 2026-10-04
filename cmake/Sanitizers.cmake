# AddressSanitizer for the standalone (non-Godot) libraries and their tests.
function(e5_enable_sanitizers target)
    if(NOT E5_ENABLE_ASAN)
        return()
    endif()
    if(MSVC)
        target_compile_options(${target} PRIVATE /fsanitize=address /Zi)
        # vcpkg libraries (Catch2) are not ASan-instrumented. The MSVC STL refuses to
        # link annotated and non-annotated containers together (LNK2038), so container
        # annotations are off; heap, stack and global checks remain active.
        target_compile_definitions(${target} PRIVATE _DISABLE_STRING_ANNOTATION _DISABLE_VECTOR_ANNOTATION)
        # /INCREMENTAL is incompatible with ASan.
        target_link_options(${target} PRIVATE /INCREMENTAL:NO)
    else()
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()
