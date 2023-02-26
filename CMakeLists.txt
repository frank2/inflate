cmake_minimum_required(VERSION 3.24)

project(inflate CXX)

option(INFLATE_TEST "Enable testing for inflate." OFF)
option(INFLATE_BUILD_SHARED "Compile inflate as a shared library." OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE HEADER_FILES FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/include/inflate/*.h ${PROJECT_SOURCE_DIR}/include/inflate/*.hpp)
file(GLOB_RECURSE SRC_FILES FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/src/*.c ${PROJECT_SOURCE_DIR}/src/*.cpp)

source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Header Files" FILES ${HEADER_FILES} ${PROJECT_SOURCE_DIR}/include/inflate.hpp)
source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SRC_FILES})

if (INFLATE_BUILD_SHARED)
  add_compile_definitions(INFLATE_SHARED)
  add_compile_definitions(INFLATE_EXPORT)
  add_library(inflate SHARED ${HEADER_FILES} ${PROJECT_SOURCE_DIR}/include/inflate.hpp ${SRC_FILES})
else()
  add_compile_definitions(INFLATE_STATIC)
  add_library(inflate STATIC ${HEADER_FILES} ${PROJECT_SOURCE_DIR}/include/inflate.hpp ${SRC_FILES})
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

set_target_properties(inflate PROPERTIES LINKER_LANGUAGE CXX)

if (UNIX)
  install(TARGETS inflate DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
  install(FILES ${HEADER_FILES} DESTINATION "${CMAKE_INSTALL_PREFIX}/include/inflate")
  install(FILES ${PROJECT_SOURCE_DIR}/include/inflate.hpp DESTINATION "${CMAKE_INSTALL_PREFIX}/include")
endif()

if (INFLATE_TEST)
  enable_testing()

  add_executable(testinflate ${PROJECT_SOURCE_DIR}/test/main.cpp ${PROJECT_SOURCE_DIR}/test/framework.hpp)
  target_link_libraries(testinflate PUBLIC inflate)
  target_include_directories(testinflate PUBLIC
    "${PROJECT_SOURCE_DIR}/test"
  )
  add_test(NAME testinflate COMMAND testinflate)
endif()
