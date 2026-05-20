
cmake_minimum_required(VERSION 3.10)
# set -DCMAKE_POLICY_VERSION_MINIMUM=3.5
# set policy version to 3.5 to allow for use of cmake_policy in the debian.cmake file, which is used to set policies that are required for compatibility with older cmake versions used in debian stable

project("GuitarMidi-LV2")
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)

# specify the C++ standard
option(BUILD_ANALYSIS_APP "Build analysis app, usefull for development" OFF)

option(WITH_TRACING_INFO "Build with tracing logs" OFF)
option(WITH_AUBIO "Build aubio for onset detection" OFF)
set(RING_BUFFER_SIZE "8" CACHE STRING "Number of frames to store in the ring buffer")

message("Ring buffer size: ${RING_BUFFER_SIZE}")
option(USE_ELLIPTIC_FILTERS "Build using elliptic filters. These are slower than the default butterworth filter but perform better at lower frequencies" OFF)
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
set(JSON_Install ON CACHE BOOL "Enable installation rules for nlohmann_json dependency")
# add_subdirectory(ext/eigen)
# add_subdirectory(ext/FunctionalPlus)
# # add_subdirectory(ext/json)
# add_subdirectory(ext/frugally-deep)
add_subdirectory(src)
#add_subdirectory(src/tflite_custom_op_example)

if(${BUILD_ANALYSIS_APP})
add_subdirectory(AnalysisApp)
endif()



#Debian packaging
option(BUILD_DEB "Build debian package" ON)
if(BUILD_DEB)
	include(setversionfromgit)

	setversionfromgit()
	include(debian)
endif()


