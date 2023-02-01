cmake_minimum_required(VERSION 3.10)

project(maj2random)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

TRY_RUN(
  cpuident_result
  cpuident_compile_result
  ${CMAKE_CURRENT_BINARY_DIR}/
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cpuident.c
  COMPILE_OUTPUT_VARIABLE test_compile_output
  RUN_OUTPUT_VARIABLE cpuident_output
  ARGS -e
)
foreach(opt_name_value IN ITEMS ${cpuident_output})
  string(REGEX MATCH "^[^=]+" opt_name ${opt_name_value})
  string(REPLACE "${opt_name}=" "" opt_value ${opt_name_value})
  cmake_language(EVAL CODE "SET(${opt_name} ${opt_value})")
endforeach()

message("cpuident: x86_sse4_2:${x86_sse4_2} x86_avx:${x86_avx} x86_avx2:${x86_avx2} x86_avx512f:${x86_avx512f}")

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(cpuident src/cpuident.c)
