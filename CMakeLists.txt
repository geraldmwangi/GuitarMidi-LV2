
cmake_minimum_required(VERSION 3.6)
project("GuitarMidi-LV2")


# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(DSPFiltersDir DSPFilters/shared/DSPFilters)
include_directories( ${DSPFiltersDir}/include/ /usr/include)
message(${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
set(DSPFiltersSRCs ${DSPFiltersDir}/source/Cascade.cpp
    ${DSPFiltersDir}/source/Biquad.cpp
    ${DSPFiltersDir}/source/Elliptic.cpp
    ${DSPFiltersDir}/source/PoleFilter.cpp)
add_library(dsp STATIC ${DSPFiltersSRCs})

add_subdirectory(src)


