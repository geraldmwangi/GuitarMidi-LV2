
cmake_minimum_required(VERSION 3.10)
# set -DCMAKE_POLICY_VERSION_MINIMUM=3.5
# set policy version to 3.5 to allow for use of cmake_policy in the debian.cmake file, which is used to set policies that are required for compatibility with older cmake versions used in debian stable

project("GuitarMidi-LV2")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# specify the C++ standard
option(BUILD_ANALYSIS_APP "Build analysis app, usefull for development" OFF)




set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(TENSORFLOW_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/tensorflow")
message("Tensorflow found at ${TENSORFLOW_SOURCE_DIR}")
add_subdirectory(core)
add_subdirectory(src)

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


