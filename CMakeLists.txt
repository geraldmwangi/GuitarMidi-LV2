
cmake_minimum_required(VERSION 3.6)
project("GuitarMidi-LV2")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(setversionfromgit)

setversionfromgit()
# specify the C++ standard
option(BUILD_ANALYSIS_APP "Build analysis app, usefull for development" OFF)

option(WITH_TRACING_INFO "Build with tracing logs" OFF)
option(USE_ELLIPTIC_FILTERS "Build using elliptic filters. These are slower than the default butterworth filter but perform better at lower frequencies" ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(DSPFiltersDir DSPFilters/shared/DSPFilters)
include_directories( ${DSPFiltersDir}/include/ /usr/include)
message(${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
set(DSPFiltersSRCs ${DSPFiltersDir}/source/Cascade.cpp
    ${DSPFiltersDir}/source/Biquad.cpp
    ${DSPFiltersDir}/source/Elliptic.cpp
    ${DSPFiltersDir}/source/Butterworth.cpp
    ${DSPFiltersDir}/source/PoleFilter.cpp)
add_library(dsp STATIC ${DSPFiltersSRCs})

add_subdirectory(src)

if(${BUILD_ANALYSIS_APP})
add_subdirectory(AnalysisApp)
endif()


include(debian)


