set(CMAKE_VERBOSE_MAKEFILE ON)

if(gcc_or_clang)
  add_compile_options(-Wall -march=native -fmax-errors=1)
else()
  add_compile_options(-Wall)
endif()

add_subdirectory(domcolor_example)
add_subdirectory(domcolor_benchmark)