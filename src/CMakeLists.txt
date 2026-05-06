set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
configure_file(include/config.hpp.in include/config.hpp)
include_directories(${PROJECT_BINARY_DIR}/src/include)
include_directories(/usr/include)
include_directories(include)
include_directories(${PROJECT_SOURCE_DIR}/src/include)
find_package(PkgConfig REQUIRED)

find_package(OpenMP REQUIRED)
pkg_check_modules(Pango REQUIRED pango)

# add_subdirectory(tflite_custom_op_example)

# Read TF_VERSION file to get TensorFlow version
file(READ "${CMAKE_SOURCE_DIR}/ext/tensorflow/tensorflow/tf_version.bzl" TF_VERSION_BZL)
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
include_directories(${TENSORFLOW_SOURCE_DIR}  ${TENSORFLOW_SOURCE_DIR}/tensorflow/lite/delegates/xnnpack)
add_subdirectory(
  "${TENSORFLOW_SOURCE_DIR}/tensorflow/lite"
  "${CMAKE_CURRENT_BINARY_DIR}/tensorflow-lite"
  EXCLUDE_FROM_ALL
)




set(SRC guitarmidi.cpp  noteclassifier.cpp midioutput.cpp fretboard.cpp fretboardrepresentation.cpp guitarnote.cpp logging.cpp filterbank.cpp filter.cpp guitarstring.cpp noteinferencer.cpp modelinferencer.cpp)

add_library(guitarmidi SHARED ${SRC})


if(WITH_AUBIO=On)
message("Building with onset support")
target_link_libraries(guitarmidi dsp aubio ${Pango_LIBRARIES})
else()
message("Building without onset support")
target_link_libraries(guitarmidi PUBLIC dsp  ${Pango_LIBRARIES} tensorflow-lite  )
target_link_libraries(guitarmidi PUBLIC OpenMP::OpenMP_CXX )
endif()





install(TARGETS guitarmidi
        DESTINATION guitarmidi.lv2)

install(FILES ../manifest.ttl ../guitarmidi.ttl ../python/neuralnetmodelling/guitarmidi.tflite
        DESTINATION guitarmidi.lv2)