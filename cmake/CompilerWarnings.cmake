# Applies the project warning policy to a first-party target.
# Third-party code is never passed through this function.
function(e5_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /utf-8
            /w14242 # conversion, possible loss of data
            /w14254 # larger bit field assigned to smaller
            /w14263 # member function does not override base virtual
            /w14265 # class has virtual functions but non-virtual destructor
            /w14287 # unsigned/negative constant mismatch
            /w14296 # expression is always true/false
            /w14311 # pointer truncation
            /w14545 /w14546 /w14547 /w14549 /w14555 # suspicious comma/expression statements
            /w14619 # unknown pragma warning number
            /w14640 # thread-unsafe static member initialization
            /w14826 # sign-extended conversion
            /w14905 /w14906 # wide/narrow string cast
            /w14928 # illegal copy-initialization
        )
        if(E5_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
            -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Woverloaded-virtual
            -Wnull-dereference -Wdouble-promotion -Wformat=2
        )
        if(E5_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
