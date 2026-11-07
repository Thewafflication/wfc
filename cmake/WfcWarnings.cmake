function(wfc_enable_strict_warnings target)
  if(MSVC)
    target_compile_options(
      "${target}"
      PRIVATE /W4 /WX /permissive- /Zc:__cplusplus /Zc:preprocessor
    )
  else()
    target_compile_options("${target}" PRIVATE -Wall -Wextra -Wpedantic)
    if(NOT WFC_WINELIB)
      target_compile_options("${target}" PRIVATE -Werror)
    endif()
  endif()
endfunction()
