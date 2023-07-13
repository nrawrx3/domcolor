set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(OpenMP REQUIRED)

if(OpenMP_CXX_FOUND)
  message("OPENMP FOUND")
endif()

add_executable(palette_transfer_serial palette_transfer.cpp)
target_link_libraries(palette_transfer_serial domcolor plog)

add_executable(palette_transfer_parallel palette_transfer.cpp)
target_compile_definitions(palette_transfer_parallel PRIVATE "-DENABLE_OPENMP=1")
target_link_libraries(palette_transfer_parallel PRIVATE OpenMP::OpenMP_CXX domcolor plog)
