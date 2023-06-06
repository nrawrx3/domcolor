set(CMAKE_VERBOSE_MAKEFILE ON)

if(gcc_or_clang)
  add_compile_options(-Wall -march=native -fmax-errors=1)
else()
  add_compile_options(-Wall)
endif()

set(header_dir ${PROJECT_SOURCE_DIR}/include/domcolor)
set(header_files_relative
	domcolor.hpp
)

set(library_source_files
	domcolor.cpp
)

add_library(domcolor SHARED ${library_source_files})
target_link_libraries(domcolor plog)

target_compile_definitions(domcolor
	PRIVATE DOMCOLOR_API_BEING_BUILT
	PUBLIC DOMCOLOR_API_BEING_IMPORTED)

# Allow automatic discovery of the header directories when our lib is linked
# using target_linked_libraries
target_include_directories(domcolor
    PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)
