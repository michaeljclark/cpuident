# cpuident

cpuident is a portable C program to read x86-64 cpuid processor information.
cpuident has been designed to output variables in a format that can be used
for host processor extension checks using CMake.

cpuid extension values are sourced from the following documents:

- _Intel Architectures Software Developer’s Manual Volume 2A,
  Instruction Set Reference, December 2022, CPUID—CPU Identification._
- _Intel Architecture Instruction Set Extensions and Future Features,
  Programming Reference, December 2022, CPUID—CPU Identification._


## Build Instructions

cpuident has been built and tested with CMake on Windows, macOS and Linux.

```
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

## CMake Integration

cpuident can be added to projects to provide detection of processor features.
While Clang and GCC both support `-march=native`, cpuident allows conditional
CMake logic based on detected processor features.

```
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
```
