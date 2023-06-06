set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(OpenMP REQUIRED)

if(OpenMP_CXX_FOUND)
  message("OPENMP FOUND")
endif()

add_executable(domcolor_example_serial domcolor_example.cpp)
target_link_libraries(domcolor_example_serial domcolor plog)

add_executable(domcolor_example_parallel domcolor_example.cpp)
target_compile_definitions(domcolor_example_parallel PRIVATE "-DENABLE_OPENMP=1")
target_link_libraries(domcolor_example_parallel PRIVATE OpenMP::OpenMP_CXX domcolor plog)
