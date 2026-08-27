cmake_minimum_required(VERSION 3.10)
# set -DCMAKE_POLICY_VERSION_MINIMUM=3.5
# set policy version to 3.5 to allow for use of cmake_policy in the debian.cmake file, which is used to set policies that are required for compatibility with older cmake versions used in debian stable

project("GuitarMidi-Core")

option(WITH_TRACING_INFO "Build with tracing logs" OFF)
option(WITH_AUDIO_OUTPUT "Build with audio output support" OFF)
set(RING_BUFFER_SIZE "8" CACHE STRING "Number of frames to store in the ring buffer") 
set(NUM_INFERENCE_THREADS "4" CACHE STRING "Number of threads to use for inference")

message("Ring buffer size: ${RING_BUFFER_SIZE}")
message("Number of inference threads: ${NUM_INFERENCE_THREADS}")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
configure_file(include/guitarmidi/config.hpp.in include/guitarmidi/config.hpp)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

include_directories(/usr/include)
include_directories(include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
find_package(PkgConfig REQUIRED)
find_library(ZITA_RESAMPLER_LIB zita-resampler)
message("Found zita-resampler library: ${ZITA_RESAMPLER_LIB}")
#find_package(OpenMP REQUIRED)
# pkg_check_modules(Pango REQUIRED pango)

# add_subdirectory(tflite_custom_op_example)

# Read TF_VERSION file to get TensorFlow version
file(READ "${TENSORFLOW_SOURCE_DIR}/tensorflow/tf_version.bzl" TF_VERSION_BZL)
#message ("TF_VERSION_BZL contents: ${TF_VERSION_BZL}")
string(REGEX MATCH "TF_VERSION\\s* = \\s*\"([0-9]+)\\.([0-9]+)\\.([0-9]+)\"" _ "${TF_VERSION_BZL}")

if(NOT CMAKE_MATCH_1)
  message(FATAL_ERROR "Could not determine Tensorflow version from tf_version.bzl contents: ${TF_VERSION_BZL}")
endif()

set(TF_MAJOR ${CMAKE_MATCH_1})
set(TF_MINOR ${CMAKE_MATCH_2})
set(TF_PATCH ${CMAKE_MATCH_3})
set(TF_VERSION_SUFFIX "")

message("Building with Tensorflow version: ${TF_MAJOR}.${TF_MINOR}.${TF_PATCH}${TF_VERSION_SUFFIX}")

set(TF_CXX_FLAGS "-DTF_MAJOR_VERSION=${TF_MAJOR} -DTF_MINOR_VERSION=${TF_MINOR} -DTF_PATCH_VERSION=${TF_PATCH} -DTF_VERSION_SUFFIX=\"${TF_VERSION_SUFFIX}\"")
message("Setting TensorFlow compile flags: ${TF_CXX_FLAGS}")
add_definitions(-DTF_MAJOR_VERSION=${TF_MAJOR} -DTF_MINOR_VERSION=${TF_MINOR} -DTF_PATCH_VERSION=${TF_PATCH} -DTF_VERSION_SUFFIX=\"${TF_VERSION_SUFFIX}\")

# Provide compile-time definitions in the recommended CMake form (no leading -D).
# This makes macros available globally; for target-specific definitions prefer
# `target_compile_definitions(<target> PRIVATE ...)` below.
## compile-time definitions will be added to the `guitarmidi` target below


option(LINK_TFLITE_FLEX "Enable tensorflowlite_flex library linkage" OFF)

set(TENSORFLOW_SOURCE_DIR "${CMAKE_SOURCE_DIR}/ext/tensorflow" CACHE PATH
  "Directory that contains the TensorFlow project"
)

message("TensorFlow source dir: ${TENSORFLOW_SOURCE_DIR}")
set(TFLITE_ENABLE_XNNPACK ON CACHE BOOL "Enable XNNPACK" FORCE)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps" CACHE PATH "" FORCE)

include_directories(${TENSORFLOW_SOURCE_DIR}  ${TENSORFLOW_SOURCE_DIR}/tensorflow/lite/delegates/xnnpack)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(
  "${TENSORFLOW_SOURCE_DIR}/tensorflow/lite"
  "${CMAKE_CURRENT_BINARY_DIR}/tensorflow-lite"
  EXCLUDE_FROM_ALL
)




set(SRC    src/fretboard.cpp src/fretboardrepresentation.cpp src/guitarnote.cpp src/logging.cpp src/filterbank.cpp src/guitarstring.cpp src/noteinferencer.cpp src/modelinferencer.cpp)

add_library(guitarmidi_core STATIC ${SRC})
set_target_properties(guitarmidi_core PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries(guitarmidi_core PUBLIC tensorflow-lite  ${ZITA_RESAMPLER_LIB} )
