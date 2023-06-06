set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(OpenMP REQUIRED)

if(OpenMP_CXX_FOUND)
  message("OPENMP FOUND")
endif()


add_executable(domcolor_benchmark_serial domcolor_benchmark.cpp nanobench.h)
target_link_libraries(domcolor_benchmark_serial PRIVATE domcolor PRIVATE plog PRIVATE)

add_executable(domcolor_benchmark_parallel domcolor_benchmark.cpp nanobench.h)
target_compile_definitions(domcolor_benchmark_parallel PRIVATE "-DENABLE_OPENMP=1")
target_link_libraries(domcolor_benchmark_parallel PRIVATE OpenMP::OpenMP_CXX domcolor plog)
