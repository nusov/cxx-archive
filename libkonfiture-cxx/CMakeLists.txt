cmake_minimum_required(VERSION 2.8)
project(konfiture CXX)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()


find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

bison_target(parser konfiture.yy ${CMAKE_CURRENT_BINARY_DIR}/konfiture.tab.cc)
flex_target(scanner konfiture.ll ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cc)

add_flex_bison_dependency(scanner parser)

set(libkonfiture_sources
  ${BISON_parser_OUTPUTS}
  ${FLEX_scanner_OUTPUTS})

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

add_library(konfiture-cxx STATIC ${libkonfiture_sources})

add_executable(konfiture-lint main.cc)
target_link_libraries(konfiture-lint konfiture-cxx)
